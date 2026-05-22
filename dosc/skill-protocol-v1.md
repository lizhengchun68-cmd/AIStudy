# Skill 调度协议 v1

> 适用宿主：`AIstudy`（`Dispatcher` + `skills/<skill_id>/manifest.json`）  
> 传输：进程 stdin 一行 JSON；可选本地 HTTP（`AIstudy --serve`，见 [`host-runtime.md`](host-runtime.md)）；MCP 见 [`mcp-tool-alignment.md`](mcp-tool-alignment.md)

本文档定义 Agent 与 Skill Host 之间的**唯一**调度契约，不包含其它信封格式。

**与 M5 会话句柄：** 信封可选字段 `context` 自 **M5b** 起由 Host 解析；字段形状、句柄 ID 规则、Store 行为见 [`context-and-handles-design.md`](context-and-handles-design.md)。下文 §2.4 为协议侧摘要。

---

## 1. 约定

| 项 | 说明 |
|----|------|
| 协议版本 | 固定字符串 `"1"`，字段名 `protocol` |
| Skill 标识 | `skill_id`，与 manifest 的 `id` 一致 |
| 契约来源 | `skills/<skill_id>/manifest.json`（`input_schema` / `output_schema`） |
| 业务载荷 | `payload` 对象，须满足 manifest 的 `input_schema` |
| 追踪 ID | `request_id`；调用方可省略，Host 在 `protocol == "1"` 时自动生成 UUID |

---

## 2. 执行请求（`execute`）

### 2.1 信封结构（无状态 Skill）

```json
{
  "protocol": "1",
  "request_id": "550e8400-e29b-41d4-a716-446655440000",
  "skill_id": "rainflow",
  "skill_version": "1.0.0",
  "payload": {
    "load_history": [1.0, 2.0, 3.0, 2.0, 1.0],
    "method": "ThreePoint",
    "params": { "threshold": 0.2, "grads": 50 }
  }
}
```

### 2.2 字段说明

| 字段 | 必填 | 类型 | 说明 |
|------|------|------|------|
| `protocol` | 是 | string | 必须为 `"1"` |
| `skill_id` | 是 | string | 已注册 Skill 的 id |
| `payload` | 是 | object | 业务参数；调度层按 manifest 校验 `required` 等 |
| `request_id` | 否 | string | 用于日志与响应回传；省略时由 Host 生成 |
| `skill_version` | 否 | string | 若提供且与 manifest `version` 不一致，返回校验错误 |
| `context` | 否 | object | **M5b+**：会话 ID 与入站句柄（见 §2.4）；省略时无状态路径 |
| `options` | 否 | object | 如 `timeout_ms`：已解析，**当前仅写日志、不强制超时** |

### 2.3 校验分层

1. **调度层**：JSON 可解析、必填信封字段、`context`（若存在）句柄规则、`payload` 与 manifest（`skill_payload_validator`）。
2. **适配层**：字段类型、枚举、算法约束（各 Skill `*_execute` 内解析）。
3. **内核层**：算法与数值错误，经 `StatusOr` 映射为统一 `error`。

校验失败时，`error.message` 尽量包含**字段路径**（与 payload 校验一致），例如：

- `context.context_id: must not be empty`
- `context.handles.mesh_main: kind does not match handle_id prefix`
- `context.handles.mesh_step: not registered (context_id=wf-m5b)`
- `payload.method: value not in enum`

### 2.4 `context`（M5b+，可选）

与 [`context-and-handles-design.md`](context-and-handles-design.md) §2 一致。

```json
{
  "protocol": "1",
  "skill_id": "mesh_import",
  "context": {
    "context_id": "550e8400-e29b-41d4-a716-446655440001",
    "handles": {
      "mesh_001": { "kind": "mesh", "uri": "artifact://550e8400-e29b-41d4-a716-446655440001/mesh_001.h5" },
      "file_cfg": { "kind": "file", "uri": "file://D:/data/model.inp" }
    }
  },
  "payload": { "source_path": "model.inp", "mesh_handle": "mesh_001" }
}
```

| 字段 | 必填 | 说明 |
|------|------|------|
| `context.context_id` | 若带 `context` 则必填 | 会话 ID；建议 UUID；须满足 `isValidContextId`（长度 ≤64，字符集见设计文档） |
| `context.handles` | 否 | 入站句柄表；键为 `handle_id`（前缀 `mesh_` / `result_` / `file_`） |
| `handles[<id>].kind` | 建议 | `mesh` / `result` / `file`，须与 `handle_id` 前缀一致 |
| `handles[<id>].uri` | 建议 | `artifact://<context_id>/...` 或 `file://...` |
| `context.close` | 否 | 布尔；`true` 时在本请求 Skill **执行结束后**调用 `closeContext`（M7a） |

**行为：**

- 省略 `context`：`rainflow`、`host_echo` 等与 M4 相同，不访问 Context Store。
- 提供 `context`：Host `ensureContext`、合并 `handles` 入 Store，执行期 adapter 可通过 `skill_execution_context` 读写句柄；artifact 目录为 `<project>/.aistudy/artifacts/<context_id>/`。
- 需要会话的 Skill（如 `mesh_import`）若未带 `context`，返回 `context: no active context (envelope context required)`。
- **收尾（M7a）：** Skill `context_close`（`payload: {}`）或 `context.close: true`；Host 亦支持 `AIstudy --drop-context <id>`（仅内存，不删 artifact）。

**多步示例（同一 `context_id`）：** 先 `mesh_import` 注册 `mesh_*`，下一步可在 `context.handles` 中传入该句柄，无需在 `payload` 重复传大对象；结束时 `context_close` 或 `close: true`。详见设计文档 §5.4 与 `tests/context_close_contract_test.cpp`。

---

## 3. 执行响应

### 3.1 成功

```json
{
  "protocol": "1",
  "request_id": "550e8400-e29b-41d4-a716-446655440000",
  "ok": true,
  "result": {
    "items": [],
    "num_cycles": 0
  },
  "meta": {
    "skill_id": "rainflow",
    "skill_version": "1.0.0",
    "duration_ms": 12
  }
}
```

| 字段 | 说明 |
|------|------|
| `ok` | 固定 `true` |
| `result` | 业务结果，形状见 manifest `output_schema` |
| `meta.skill_id` | 实际执行的 Skill |
| `meta.skill_version` | manifest 中的版本 |
| `meta.duration_ms` | Host 侧执行耗时（毫秒） |

### 3.2 失败

```json
{
  "protocol": "1",
  "request_id": "550e8400-e29b-41d4-a716-446655440000",
  "ok": false,
  "error": {
    "code": 1001,
    "category": "validation",
    "message": "context.handles.bad_handle: invalid handle_id prefix (expected mesh_/result_/file_)"
  },
  "meta": {
    "skill_id": "rainflow",
    "skill_version": "1.0.0",
    "duration_ms": 0
  }
}
```

| 字段 | 说明 |
|------|------|
| `ok` | 固定 `false` |
| `error.code` | 整数错误码（见 `common/status/exception/error_codes.h`） |
| `error.category` | 错误类别字符串 |
| `error.message` | 人类可读说明；信封/context/payload 校验与 Store 失败均优先带字段路径 |

信封解析失败且无法确定 `request_id` 时，`request_id` 可为空字符串；`meta` 的 `skill_id` / `skill_version` 在未解析 Skill 时可能省略或仅部分填充。

---

## 4. Skill 发现（CLI）

不经过 `execute` 信封，由宿主命令行提供：

| 命令 | 输出 |
|------|------|
| `AIstudy --list` | `{ "skills": [ ... ], "load_errors": [ ... ] }`（可选） |
| `AIstudy --describe <skill_id>` | 完整 manifest 摘要（含 `input_schema`、`output_schema`、`tags` 等） |
| `AIstudy --health` | `{ "protocol":"1", "ok", "skills_loaded", "checks": { "poco", "hdf5" }, ... }` |

Agent 编排前应优先 `--describe` 获取契约，再构造 `payload`；多步 FEM 流程另需稳定 `context.context_id`。

---

## 5. 注册与目录

| 路径 | 作用 |
|------|------|
| `skills/<skill_id>/manifest.json` | Skill 契约 |
| `src/scheduler/skill_registry.cpp` | 编译期 `skill_id` → `execute` 函数绑定 |
| `src/scheduler/skill_registry_bindings.txt` | CMake 校验用 manifest 列表（与 `kBindings` 应对齐） |

新增 Skill：增加 manifest + 在 `kBindings` 注册执行函数；契约以 manifest 为准。若 Skill 使用会话句柄，manifest 或文档中应声明需要 `context`。

---

## 6. 示例：stdin 调用 rainflow

**请求（单行 JSON）：**

```json
{"protocol":"1","skill_id":"rainflow","payload":{"load_history":[1,2,3,2,1],"method":"ThreePoint"}}
```

**PowerShell：**

```powershell
'{"protocol":"1","skill_id":"rainflow","payload":{"load_history":[1,2,3,2,1],"method":"ThreePoint"}}' |
  .\build\src\Release\AIstudy.exe
```

---

## 7. 与架构文档关系

- 分层与句柄规则：`.cursor/rules/fem-simulation-architecture.mdc`
- Context / 句柄设计（实现细节）：[`context-and-handles-design.md`](context-and-handles-design.md)
- 演进路线：`dosc/agent-skill-architecture-roadmap.md`
