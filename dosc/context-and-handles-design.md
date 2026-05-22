# Context 与会话句柄设计（M5a）

> 版本：1.0  
> 日期：2026-05-21  
> 状态：**设计基线**（M5a）；实现见 M5b 任务表  
> 约束：符合 `.cursor/rules/fem-simulation-architecture.mdc`（对外仅字符串句柄 ID，禁止指针/函数指针出进程）

---

## 1. 目标与范围

### 1.1 要解决的问题

FEM 多步工作流（导入网格 → 求解 → 导出）中，大对象不宜在每次 `execute` 的 `payload` 里重复传输。需要：

- **会话**：同一 Agent 任务内多次 `execute` 共享状态；
- **句柄**：用 `handle_id` 引用网格/结果/文件，而非裸指针或大块 JSON；
- **与 rainflow 共存**：无状态 Skill 不传 `context` 时行为与 M4 完全一致。

### 1.2 M5a / M5b 划分

| 阶段 | 内容 | 验收 |
|------|------|------|
| **M5a**（本文 + 约定代码） | 设计文档、句柄 ID 规范、`ArtifactMeta` 类型、句柄校验函数、协议 JSON 形状、Store **接口声明** | 设计评审；GTest 校验规则；**无**运行时 Store |
| **M5b** | 解析信封 `context`、`ContextStore` 内存实现、Dispatcher 注入、首个 FEM 向 Skill 骨架 | 同一 `context_id` 两次调用可注册/查询句柄 |

---

## 2. 协议 v1：`context` 信封字段

### 2.1 位置

与 `skill-protocol-v1.md` 一致，在 **调度信封**（非 `payload`）上可选携带：

```json
{
  "protocol": "1",
  "request_id": "...",
  "skill_id": "mesh_import",
  "context": {
    "context_id": "550e8400-e29b-41d4-a716-446655440001",
    "handles": {
      "mesh_001": { "kind": "mesh", "uri": "artifact://session/mesh_001.h5" },
      "file_cfg": { "kind": "file", "uri": "file://D:/data/model.inp" }
    }
  },
  "payload": { }
}
```

### 2.2 字段说明

| 字段 | 必填 | 说明 |
|------|------|------|
| `context.context_id` | 若带 `context` 则必填 | 会话 ID；建议 UUID。空串视为无效。 |
| `context.handles` | 否 | 调用方已知的句柄表（入站）；Skill 可在 `result` 中返回新句柄，由 Host 合并进 Store（M5b）。 |
| `handles[<id>].kind` | 建议 | `mesh` / `result` / `file`（与 ID 前缀一致，见 §3） |
| `handles[<id>].uri` | 建议 | 资源定位：`artifact://`（HDF5 等）、`file://`（路径）；M5b 起 Host 可校验 |

### 2.3 未带 `context` 时

- `SkillEnvelope::context_id` 为空，`handles` 为空；
- Dispatcher **不**访问 ContextStore；
- rainflow / host_echo 零改动路径。

### 2.4 `options` 与 `context` 关系

- `options.timeout_ms`：M3 已解析，仅日志；
- `context`：M5b 解析；二者独立。

---

## 3. 句柄 ID 规范（对外字符串）

### 3.1 前缀与类型

| 前缀 | `HandleKind` | 典型用途 |
|------|--------------|----------|
| `mesh_` | `mesh` | 网格、节点/单元拓扑（大对象） |
| `result_` | `result` | 求解结果、场数据 |
| `file_` | `file` | 输入文件、配置、日志路径 |

规则（由 `context_handle_rules` 强制执行）：

1. `handle_id` 非空，长度 ≤ 128；
2. 必须匹配 `^(mesh_|result_|file_)[A-Za-z0-9._-]+$`；
3. 若 JSON 提供 `kind`，须与前缀推导一致。

`context_id`：非空，长度 ≤ 64，字符集 `[A-Za-z0-9._-]`（UUID 兼容）。

### 3.2 禁止

- 在 API / adapter 对外接口传递 `void*`、内核对象指针；
- 使用 `0x` 地址或整数伪装句柄；
- 跨进程传递 Store 内存指针（M7 子进程 Skill 仅传 `handle_id` + `uri`）。

---

## 4. `ArtifactMeta`（进程内元数据）

```cpp
struct ArtifactMeta {
    std::string handle_id;
    HandleKind kind;
    std::string uri;           // artifact:// / file://
    std::string skill_id;      // 创建该句柄的 Skill（可选）
    std::string created_at;    // ISO8601 或 steady 计数（M5b）
};
```

- **uri** 为逻辑路径，由 adapter/kernel 解释；HDF5 文件由 `common/io/hdf5` 读写。
- 大数组不进 `payload`；响应可返回 `{ "mesh_handle": "mesh_001" }` 等字符串字段。

---

## 5. Context Store（M5b 实现）

### 5.1 职责

- 进程内单例（或 `Dispatcher` 成员）：`context_id → (handle_id → ArtifactMeta)`；
- 生命周期：默认 **进程存活期**；M7 可加 TTL / 显式 `context_close` Skill；
- 线程安全：M5b 先 `std::mutex` 保护 map。

### 5.2 接口（`skill_context_store.h`，M5b 实现）

| 方法 | 说明 |
|------|------|
| `ensureContext(context_id)` | 无则创建空会话 |
| `getArtifact(context_id, handle_id)` | 查询；失败返回 `VALIDATION` / `UNKNOWN` |
| `putArtifact(context_id, meta)` | 注册或覆盖句柄 |
| `listHandles(context_id)` | 调试 / `--describe` 扩展 |
| `mergeInboundHandles(context_id, handles_json)` | 将信封 `context.handles` 合并入 Store |

### 5.3 Dispatcher 集成（M5b）

```
parseSkillEnvelope
  → validate context_id / inbound handles (handle rules)
  → ensureContext + mergeInboundHandles
  → validatePayloadAgainstManifest
  → adapter(payload_json, ContextView)  // M5b: 第二参数为只读视图，仍无指针
```

M5a **不**修改 `SkillExecuteFunc` 签名；M5b 通过 `thread_local` 或 `execute` 栈上 `ContextView` 传入 adapter（设计预留，见 M5b 实现说明）。

---

## 6. 与 HDF5 artifact 的关系

| 场景 | 做法 |
|------|------|
| 网格导入 | kernel 写 `artifact://<context_id>/mesh_xxx.h5`，Store 登记 `mesh_xxx` |
| 结果导出 | 读已有 `mesh_` 句柄 uri，写 `result_` 句柄 |
| rainflow | 不使用 Store；载荷历史仍在 `payload` |

目录建议（M5b）：`${AISTUDY_PROJECT_ROOT}/.aistudy/artifacts/<context_id>/`（已在 M5b 实现时创建）。

---

## 7. 首个 FEM Skill（M5b 选型）

建议 **`mesh_import` 骨架**（或 `artifact_register` 极简版）：

1. `payload`：`file_path` + 可选 `format`；
2. `result`：返回 `mesh_<id>` 句柄字符串；
3. kernel：可先 stub（仅登记 uri），再接 HDF5。

第二 Skill 示例：`result_export` 读 `mesh_` 句柄写 `result_`。

---

## 8. 错误码与日志

| 场景 | category | 说明 |
|------|----------|------|
| 非法 `handle_id` | `validation` | `error.message` 含 `context.handles.<id>` |
| 未知句柄 | `validation` | `handle not found: mesh_xxx` |
| `context_id` 缺失 | `validation` | 带了 `context` 但无 `context_id` |

日志：`AIstudy.Dispatcher` 增加 `context_id=`（M5b），与 `request_id` 同 grep。

---

## 9. 测试策略

| 阶段 | 测试 |
|------|------|
| M5a | `context_handle_rules_test`：合法/非法 handle、kind 一致性 |
| M5b | `context_store_test`：put/get 同 context；`mesh_import` 契约 golden |

---

## 10. 参考

- `dosc/skill-protocol-v1.md`
- `src/scheduler/skill_context_types.h`
- `src/scheduler/context_handle_rules.*`
