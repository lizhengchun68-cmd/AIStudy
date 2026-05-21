# AIStudy：从早期 Agent 调度到成熟 Agent + Skill 架构

> 文档版本：1.0  
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
  ├─ 直接调用 adapter::rainflow::rainflow_adapter(payload_json)
  └─ scheduler::Dispatcher::execute(envelope_json)
        └─ task_type == "rainflow"
              └─ rainflow_adapter(payload_json)
                    └─ kernel::rainflow::rainflowCounting(...)
```

信封示例（调度层）：

```json
{
  "task_type": "rainflow",
  "payload": {
    "load_history": [1.0, 2.0, 3.0, 2.0, 1.0],
    "method": "ThreePoint",
    "params": { "threshold": 0.2, "grads": 50 }
  }
}
```

适配器响应示例：

```json
{
  "success": true,
  "result": {
    "items": [ { "amplitude": 1.0, "mean": 2.0, "count": 1 } ],
    "num_cycles": 1
  }
}
```

---

## 3. 现状评估：为什么说 rainflow 是「早期形态」

### 3.1 已经具备的能力（雏形）

| 能力 | 实现位置 | 说明 |
|------|----------|------|
| Skill 标识 | `task_type: "rainflow"` | 类似 `skill_id` |
| 统一入口 | `Dispatcher::execute` | JSON 信封入、JSON 字符串出 |
| 能力描述 | `rainflow_schema()` | JSON Schema 风格 input/output |
| 发现接口（预留） | `listTaskTypes()` / `getSchema()` | `main` 中 help 已提及 `--list` / `--describe` |
| 统一成败字段 | `success` / `error` / `result` | 便于 Agent 解析 |

### 3.2 与成熟 Agent + Skill 的差距

| 维度 | 当前实现 | 成熟目标 |
|------|----------|----------|
| Skill 定义 | C++ 函数 `rainflow_schema()` + 手工 `registerAdapter` | 独立 **Skill Manifest**（id、version、schema、示例） |
| 注册方式 | `std::string(*)(const std::string&)` 函数指针 | 符合规范：句柄 / 配置结构体 / 进程间 JSON，**跨模块禁函数指针** |
| 信封协议 | 仅 `task_type` + `payload` | `request_id`、`skill_version`、`context`、`timeout` 等 |
| 参数校验 | 无调度层校验，失败在 adapter 内 | 执行前 JSON Schema 校验 |
| 错误模型 | 字符串 `error` | 与 `common/status` 的 `code` / `category` / `message` 统一 |
| 发现 | 代码内写死注册 | `capabilities`：自动扫描 manifest / 插件目录 |
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

与当前 `Dispatcher::listTaskTypes()` / `getSchema()` 对齐演进即可。

---

## 5. 与项目强制规范的对齐

### 5.1 禁止跨模块函数指针

**现状：**

```cpp
// src/scheduler/Dispatcher.h
using AdapterFunc = std::string(*)(const std::string&);
void registerAdapter(const std::string& task_type, AdapterFunc func, ...);
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

### 阶段 A：Skill 契约扎实化（优先级最高）

**目标：** Agent 只依赖契约，不依赖 C++ 头文件。

| 任务 | 交付物 |
|------|--------|
| 定义协议版本 | `protocol: "1"` 请求/响应规范文档 |
| 引入 Skill Manifest | 每 Skill 目录 + `manifest.json` |
| 统一错误 JSON | 映射 `ErrorCodeWrapper` → `error.code/category/message` |
| 调度前校验 | 集成 `common/io/json` 或 schema 校验库 |
| 实现 describe | CLI：`--list` / `--describe rainflow` 读 manifest |

**验收：** 仅给 Agent manifest + 宿主 EXE，即可完成 rainflow 调用与错误处理。

---

### 阶段 B：调度层去函数指针、可扩展注册

**目标：** 符合架构规则，支持多 Skill 接入。

| 任务 | 交付物 |
|------|--------|
| 替换 `AdapterFunc` 注册 | 静态表 / 生成 `skills_registry.cpp` / 子进程 |
| 自动发现 | 启动时扫描 `skills/` 或 CMake 生成清单 |
| 版本路由 | `rainflow` + `rainflow@2` 或 `skill_version` 字段 |

**验收：** 新增 Skill 无需修改 `main.cpp` 手工 `registerAdapter`。

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

已有 `rainflow_schema()` 可迁移为 manifest；缺口在 **稳定协议、capabilities 端点、无跨模块函数指针的宿主**。

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
| `src/scheduler/Dispatcher.h` | 调度器接口、函数指针注册 |
| `src/scheduler/Dispatcher.cpp` | 信封解析、`task_type` 路由 |
| `src/adapter/rainflow_adapter.h` | rainflow 适配器声明 |
| `src/adapter/rainflow_adapter.cpp` | JSON ↔ kernel、schema、响应封装 |
| `src/kernel/rainflow/` | 雨流算法内核 |
| `src/main.cpp` | 注册 rainflow、演示直接调用与信封调用 |
| `src/common/status/` | 错误码与 `StatusOr` |
| `src/common/io/` | JSON / HDF5 |
| `src/common/logger/` | Poco 日志与错误联动 |
| `.cursor/rules/fem-simulation-architecture.mdc` | 三层架构与句柄、禁指针规则 |

---

## 12. 修订记录

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0 | 2026-05-21 | 初稿：基于 rainflow + Dispatcher 现状与成熟 Agent+Skill 差距分析 |

---

*本文档由架构讨论整理而成，实施时若与 `.cursor/rules/fem-simulation-architecture.mdc` 冲突，以规则文件为准并先与负责人确认。*
