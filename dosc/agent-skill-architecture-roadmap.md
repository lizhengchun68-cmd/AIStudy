# AIStudy：从早期 Agent 调度到成熟 Agent + Skill 架构

> 文档版本：1.4  
> 适用仓库：`AIStudy`（`src/kernel`、`src/adapter`、`src/scheduler`、`src/common`）  
> 关联规范：`.cursor/rules/fem-simulation-architecture.mdc`

---

## 1. 文档目的

本文档基于当前代码实现（以 `rainflow` 为首个可调度任务），说明：

1. 现有架构处于 **Agent + Skill 的早期形态** 的具体表现；
2. 与 **成熟 Agent + Skill 平台** 之间的差距；
3. 分阶段改进路线图、优先级与 `common` 层定位；
4. 与项目三层架构规则（内核 / 适配 / 调度、句柄、禁止跨模块函数指针）的对齐方式。

**本文档为设计与演进指南，不替代接口规范的最终定稿。**

---

## 2. 项目架构回顾

### 2.1 三层分层（强制规范）

| 层级 | 目录 | 职责 |
|------|------|------|
| 原生内核层 | `src/kernel/` | 算法与仿真核心实现，可含内部指针 |
| 适配封装层 | `src/adapter/` | JSON（未来可扩展配置结构体）↔ 内核类型，无裸指针外传 |
| 流程调度层 | `src/scheduler/` | 任务路由、信封解析、Skill 发现与执行入口 |

**横切基础库（非 Skill）**：`src/common/status`、`src/common/io`、`src/common/logger`  
—— 进程内 C++ API，供各层内部使用，通常 **不** 注册为 Agent 可调度的 `task_type`。

### 2.2 当前主程序调用链（rainflow）

```
main.cpp
  ├─ registerBuiltinSkills(Dispatcher)   // skill_registry 读 skills/<id>/manifest.json
  ├─ CLI: --list / --describe <skill_id>
  └─ stdin → Dispatcher::execute(envelope_json)
        ├─ parseSkillEnvelope（protocol v1：`protocol`、`skill_id`、`payload`）
        ├─ validatePayloadAgainstManifest
        ├─ rainflow_execute(payload_json)   // adapter → StatusOr<result>
        └─ makeProtocolV1* / makeSkillApiResponse（scheduler 统一 `ok` / `error` / `meta`）
              └─ kernel::rainflow::rainflowCounting(...)
```

信封示例（协议 v1，详见 `dosc/skill-protocol-v1.md`）：

```json
{
  "protocol": "1",
  "skill_id": "rainflow",
  "payload": {
    "load_history": [1.0, 2.0, 3.0, 2.0, 1.0],
    "method": "ThreePoint",
    "params": { "threshold": 0.2, "grads": 50 }
  }
}
```

调度层响应示例：

```json
{
  "protocol": "1",
  "request_id": "...",
  "ok": true,
  "result": {
    "items": [ { "amplitude": 1.0, "mean": 2.0, "count": 1 } ],
    "num_cycles": 1
  },
  "meta": {
    "skill_id": "rainflow",
    "skill_version": "1.0.0",
    "duration_ms": 12
  }
}
```

---

## 3. 现状评估：为什么说 rainflow 是「早期形态」

### 3.1 已经具备的能力（雏形）

| 能力 | 实现位置 | 说明 |
|------|----------|------|
| Skill 标识 | 信封 `skill_id` + manifest `id` | 如 `"rainflow"` |
| 统一入口 | `Dispatcher::execute` | JSON 信封入、JSON 字符串出 |
| 能力描述 | `skills/<id>/manifest.json` | `input_schema` / `output_schema` |
| 发现接口 | `--list` / `--describe` | `listSkillsJson` / `describeSkillJson` |
| 统一成败字段 | `ok` / `error` / `result` / `meta` | 协议 v1，见 `api_response` |

### 3.2 与成熟 Agent + Skill 的差距

| 维度 | 当前实现 | 成熟目标 |
|------|----------|----------|
| Skill 定义 | manifest 已落地，adapter 无独立 schema 函数 | 示例/标签/版本策略继续完善 |
| 注册方式 | `SkillExecuteFunc` 静态表（进程内） | 句柄 / 子进程 JSON / 代码生成注册表 |
| 信封协议 | v1 文档已定；`context`/`options` 未解析 | 解析并实现 `context`、超时等 |
| 参数校验 | 调度层 `required` 字段 | 完整 JSON Schema（类型、enum） |
| 错误模型 | `code` / `category` / `message` 已统一 | 日志与 Agent 重试策略联动 |
| 发现 | CLI + manifest；`kBindings` 手工 | 自动扫描 / CMake 生成注册表 |
| 部署 | 单 EXE 全链接 | 可选 **Skill Host** 或每 Skill 独立 EXE |
| 状态 | 无会话 | `context_id` + 资源 **句柄 ID**（网格、结果文件） |
| 编排 | 单次调用 | 多步工作流或外部 Agent 多轮调用 + 异步 job |
| 可观测性 | logger 未绑定请求 | `request_id` + `skill_id` 结构化日志与耗时 |

### 3.3 `common/logger` 为何不走 adapter + 调度器

**结论：这是职责分层，不是架构落后。**

| 类型 | 模块 | 集成方式 |
|------|------|----------|
| 领域 Skill | rainflow、未来网格/求解等 | kernel → adapter → Dispatcher |
| 平台基础设施 | status、io、logger | 静态库，供 Skill 与调度器 **内部调用** |

Agent 在执行 `rainflow` 时 **内部** 使用 logger 记录 `request_id` 与耗时，一般 **不需要** `task_type: "write_log"` 这类 Skill。

---

## 4. 成熟 Agent + Skill 参考模型

### 4.1 角色划分

```
┌─────────────────────────────────────────────────────────┐
│  Agent Runtime（LLM / 规则 / Cursor Agent）              │
│  - 读 manifest，选 skill，组 envelope，重试与多步编排     │
└──────────────────────────┬──────────────────────────────┘
                           │ 协议（stdio / HTTP / MCP）
┌──────────────────────────▼──────────────────────────────┐
│  Skill Host / 调度网关（scheduler 演进）                  │
│  - list / describe / execute / health                   │
│  - 校验、追踪、超时、统一错误                             │
└──────────────────────────┬──────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        ▼                  ▼                  ▼
   adapter/rainflow   adapter/...        adapter/...
        │                  │                  │
        ▼                  ▼                  ▼
   kernel/rainflow     kernel/...        kernel/...
```

### 4.2 Skill 一等公民：Manifest 示例（目标态）

建议路径：`skills/rainflow/manifest.json`（或 `share/aistudy/skills/`）

```json
{
  "id": "rainflow",
  "version": "1.0.0",
  "title": "RainflowCounting",
  "description": "雨流计数：载荷谱应力幅与均值分布",
  "tags": ["fatigue", "signal"],
  "input_schema": { "$ref": "schemas/input.json" },
  "output_schema": { "$ref": "schemas/output.json" },
  "examples": [
    {
      "name": "three_point_default",
      "input": { "load_history": [1, 2, 3, 2, 1], "method": "ThreePoint" }
    }
  ],
  "timeout_ms": 30000,
  "deprecated": false
}
```

实现代码（adapter/kernel）与 manifest **解耦**：Agent 与文档只依赖 manifest。

### 4.3 信封协议 v1（建议）

**请求：**

```json
{
  "protocol": "1",
  "request_id": "550e8400-e29b-41d4-a716-446655440000",
  "skill_id": "rainflow",
  "skill_version": "1.0.0",
  "payload": { },
  "context": {
    "context_id": "session-abc",
    "handles": {
      "mesh": "mesh_handle_001"
    }
  },
  "options": {
    "timeout_ms": 30000
  }
}
```

**响应（与 `common/status` 对齐）：**

```json
{
  "protocol": "1",
  "request_id": "550e8400-e29b-41d4-a716-446655440000",
  "ok": true,
  "result": { },
  "error": {
    "code": 0,
    "category": "json",
    "message": "human readable"
  },
  "meta": {
    "skill_id": "rainflow",
    "skill_version": "1.0.0",
    "duration_ms": 12
  }
}
```

字段说明：

| 字段 | 用途 |
|------|------|
| `request_id` | 日志、追踪、幂等、Agent 多轮对话关联 |
| `skill_version` | 破坏性变更时并存多版本 |
| `context` | 多步仿真共享句柄与会话状态 |
| `options.timeout_ms` | 调度层强制超时 |
| `error.code/category` | 机器可处理，供 Agent 决策重试或换参 |

### 4.4 Capabilities API（发现）

| 方法 | 说明 |
|------|------|
| `list_skills` | 返回所有 `skill_id`、版本、简短描述 |
| `describe_skill` | 返回完整 manifest + input/output schema |
| `execute` | 执行信封 |
| `health`（可选） | 宿主与依赖（Poco、HDF5）就绪状态 |

与当前 `Dispatcher::listSkillsJson()` / `describeSkillJson()` 对齐演进即可。

---

## 5. 与项目强制规范的对齐

### 5.1 禁止跨模块函数指针

**现状：**

```cpp
// src/scheduler/Dispatcher.h（进程内静态表，不对外暴露函数指针）
using SkillExecuteFunc = StatusOr<Poco::JSON::Object::Ptr>(*)(const std::string& payload_json);
void registerSkill(const SkillManifest& manifest, SkillExecuteFunc func);
```

**演进选项（择一或组合）：**

| 方案 | 做法 | 优点 | 缺点 |
|------|------|------|------|
| A. 静态分发表 | `skill_id` → 编译期注册的 `execute_json` 符号 | 简单、无指针外传 | Skill 增多时需代码生成 |
| B. 子进程 Skill | 每 Skill 独立 EXE，stdio JSON | 隔离最好，符合「独立 EXE」 | 进程开销 |
| C. 插件 ABI 单入口 | `extern "C" int skill_execute(const char* in, char* out, size_t cap)` | 动态加载 | 需严格 ABI 与版本管理 |

**原则：** 函数指针若存在，仅限制在 **Skill Host 进程内部** 或 **插件边界**，不进入 adapter/kernel 的公开 C++ API。

### 5.2 资源句柄 ID

大对象（网格、场结果、HDF5 路径）对外：

- 响应返回 `artifact_handle` / `mesh_handle`，而非指针或裸路径随意传播；
- `context.handles` 在多次 `execute` 间传递；
- 内核内部仍可用裸指针，对外仅句柄字符串。

rainflow 当前为 **无状态单次计算**，可作为 Skill 样板；**网格/求解** 上线前必须完成句柄与会话存储设计。

### 5.3 模块新增流程（规范要求）

新增领域 Skill 时顺序：

1. 定义模块职责 + manifest + input/output schema  
2. `kernel` 原生实现  
3. `adapter`：JSON ↔ 内核，统一 `ok/result/error`  
4. 注册到调度层（或 manifest 自动发现）  
5. 契约测试（golden JSON）+ schema 回归  

---

## 6. 分阶段改进路线图

### 阶段 A：Skill 契约扎实化（优先级最高）— **已基本完成**

**目标：** Agent 只依赖契约，不依赖 C++ 头文件。

| 任务 | 交付物 | 状态 |
|------|--------|------|
| 定义协议版本 | `dosc/skill-protocol-v1.md` | ✅ |
| 引入 Skill Manifest | `skills/rainflow/manifest.json` + `skill_manifest.*` | ✅ |
| 统一错误 JSON | `common/status/api_response.h`（v1 `ok`/`error`/`meta`） | ✅ |
| 调度前校验 | `skill_payload_validator`（required 字段） | ✅ 基础版 |
| 实现 describe | CLI：`--list` / `--describe rainflow` | ✅ |

**验收：** 仅给 Agent manifest + 宿主 EXE，即可完成 rainflow 调用与错误处理。

---

### 阶段 B：调度层去函数指针、可扩展注册 — **部分完成**

**目标：** 符合架构规则，支持多 Skill 接入。

| 任务 | 交付物 | 状态 |
|------|--------|------|
| 静态 Skill 分发表 | `skill_registry.cpp`（`kBindings` + manifest 加载） | ✅ |
| 去掉 main 手工注册 | `registerBuiltinSkills(dispatcher)` | ✅ |
| 函数指针边界 | `SkillExecuteFunc` 仅 scheduler 内部静态表 | ✅ 进程内 |
| 版本路由 | 信封 `skill_version` 与 manifest 比对 | ✅ 基础版 |
| 子进程 / 动态插件 | 独立 EXE 或 `extern "C"` 加载 | ⏳ 未做 |

**验收：** 新增 Skill 在 `kBindings` 加一行 + `skills/<id>/manifest.json`，无需改 `main.cpp`。

---

### 阶段 C：Agent Runtime 与 Skill Host 分离

**目标：** 「会选 Skill 的 Agent」与「会算的内核」解耦。

| 组件 | 职责 |
|------|------|
| `aistudy-skill-host`（或由 `AIstudy` 演进） | 仅 `execute` / `describe` / `health` |
| Agent（外部或独立工具） | 读 manifest、组信封、重试、多步编排 |
| 传输 | stdio（批处理）、HTTP（服务化）、MCP（与 Cursor 对齐） |

**验收：** Cursor / 脚本通过 stdin 一行 JSON 调用 rainflow，无需链接业务库。

---

### 阶段 D：编排、会话、句柄（仿真向成熟）

**目标：** 支撑 FEM 多步流程。

| 任务 | 说明 |
|------|------|
| Context Store | `context_id` → 句柄表（内存或 HDF5 索引） |
| 异步任务 | `execute_async` → `job_id` → `poll` / `get_result` |
| 大结果外置 | 结果写 artifact，响应只返回 handle |
| Workflow（可选） | 内置 DAG 或文档约定由 Agent 多轮 `execute` |

**验收：** 「导入网格 → 求解 → 导出结果」三步通过句柄串联，无需在 payload 重复传大对象。

---

### 阶段 E：工程化与运维（生产级）

| 任务 | 说明 |
|------|------|
| 结构化日志 | `common/logger` 打 `request_id`、`skill_id`、`duration_ms` |
| 指标 | 成功率、P99 延迟、payload 大小 |
| 限流与配额 | 最大 payload、并发 job 上限 |
| 版本策略 | deprecated、最低 host 版本 |
| 契约测试 | 每 Skill 目录 `tests/*.json` in/out |

---

## 7. `src/common` 在成熟架构中的定位

| 模块 | 角色 | 是否做成 Skill |
|------|------|----------------|
| `common/status` | 统一错误码、`StatusOr`、异常 | 否 |
| `common/io/json` | manifest 加载、schema、payload 解析 | 否 |
| `common/io/hdf5` | artifact 持久化、大结果 | 否 |
| `common/logger` | 请求级追踪与错误日志 | 否 |

**改进方向：** 强化平台能力，而非把 logger 注册为 `task_type`。

示例日志字段（目标）：

```
[INFO] request_id=... skill_id=rainflow duration_ms=12 ok=true
[ERROR] request_id=... skill_id=rainflow code=1001 category=json message=...
```

---

## 8. 与 Cursor / MCP Skill 的对齐（可选）

| MCP / Cursor 概念 | AIStudy 映射 |
|-------------------|--------------|
| Tool name | `skill_id` |
| Tool description | manifest `description` |
| Parameters JSON Schema | manifest `input_schema` |
| Tool call | `execute` 信封 |
| Tool result | `ok` + `result` 或 `error` |

rainflow 契约已以 manifest 为准；缺口在 **协议与实现完全收敛、capabilities 完善（如 health）、可观测性与 FEM 句柄**。

---

## 9. 预编译 common 库（补充说明）

若将 `status` / `io` / `logger` 预编译为三方式静态库：

- **有利于** 缩短主工程编译、多 EXE 复用、接口冻结后的 SDK 化；
- **不利于** common 频繁改动时的调试、Debug/配置矩阵维护、与 Poco/HDF5 版本绑定。

**建议：** 在阶段 A/B 协议稳定后再考虑；开发期保持 `add_subdirectory` 源码构建。

详见团队内对「预编译 common」的可行性评估（与本文档独立）。

---

## 10. 优先级总览

| 顺序 | 内容 | 投入 | 收益 |
|------|------|------|------|
| 1 | 统一协议 + manifest + status 错误对齐 | 中 | 高 |
| 2 | capabilities + payload 校验 | 中 | 高 |
| 3 | 去掉跨模块函数指针、自动注册 | 中 | 高（合规） |
| 4 | request_id + 结构化日志 | 低 | 中高 |
| 5 | context + 句柄 + session | 高 | 高（FEM 必备） |
| 6 | 异步 job / 独立 Skill EXE | 高 | 中（规模大了再做） |
| 7 | 内置 workflow | 高 | 视 Agent 外置与否 |

---

## 11. 附录：当前关键文件索引

| 文件 | 作用 |
|------|------|
| `dosc/skill-protocol-v1.md` | 协议 v1 请求/响应说明 |
| `skills/rainflow/manifest.json` | rainflow Skill 契约 |
| `src/scheduler/Dispatcher.*` | 信封解析、校验、执行、`makeSkillApiResponse` |
| `src/scheduler/skill_registry.*` | 静态 Skill 表 + manifest 注册 |
| `src/scheduler/skill_manifest.*` | 加载 manifest.json |
| `src/scheduler/skill_protocol.*` | 信封解析（protocol v1） |
| `src/scheduler/skill_payload_validator.*` | payload required 校验 |
| `src/adapter/rainflow_adapter.*` | `rainflow_execute`：JSON ↔ kernel |
| `src/kernel/rainflow/` | 雨流算法内核 |
| `src/main.cpp` | CLI：`--list` / `--describe` / stdin execute |
| `src/common/status/api_response.*` | 协议 v1 响应（`ok` / `error` / `meta`） |
| `src/common/status/` | 错误码与 `StatusOr` |
| `src/common/io/` | JSON / HDF5 |
| `src/common/logger/` | Poco 日志与错误联动 |
| `.cursor/rules/fem-simulation-architecture.mdc` | 三层架构与句柄、禁指针规则 |

---

## 12. 修订记录

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0 | 2026-05-21 | 初稿：基于 rainflow + Dispatcher 现状与成熟 Agent+Skill 差距分析 |
| 1.1 | 2026-05-21 | 实现统一错误 JSON：`api_response.h`，adapter/scheduler 已接入 |
| 1.2 | 2026-05-21 | API 信封由 scheduler 集中：`SkillExecuteFunc` + `makeApiResponse`；adapter 仅返回 `StatusOr<result JSON>` |
| 1.3 | 2026-05-21 | 阶段 A/B 落地：manifest、protocol v1、`skill_registry`、CLI、payload 校验；见 `skills/` 与 `scheduler/skill_*` |
| 1.4 | 2026-05-21 | §2.2/§3.1 与 `skill-protocol-v1.md` 对齐：移除 `task_type`/`success` 等遗留描述 |

---

*本文档由架构讨论整理而成，实施时若与 `.cursor/rules/fem-simulation-architecture.mdc` 冲突，以规则文件为准并先与负责人确认。*
