# AIStudy：从早期 Agent 调度到成熟 Agent + Skill 架构

> 文档版本：1.5  
> 日期：2026-05-21  
> 适用仓库：`AIStudy`（`src/kernel`、`src/adapter`、`src/scheduler`、`src/common`）  
> 关联规范：`.cursor/rules/fem-simulation-architecture.mdc`  
> 协议真源：[`skill-protocol-v1.md`](skill-protocol-v1.md)（优于本文 §4.3 示例）

---

## 1. 文档目的

本文档说明：

1. 当前架构在 **Agent + Skill** 方向上的实现程度；
2. 与成熟平台相比仍缺的项（M6/M7、真实 FEM、schema 高级特性等）；
3. 分阶段路线图与 `common` 层定位；
4. 与三层架构规则（内核 / 适配 / 调度、句柄、禁止跨模块函数指针）的对齐方式。

**实施状态快照（2026-05-21）：** M1–M5b 已落地；迭代 1–3（见 [`next-development-plan.md`](next-development-plan.md) §6）主体完成；下一步为 **M6**（stdio 固化 / HTTP / MCP）。

---

## 2. 项目架构回顾

### 2.1 三层分层（强制规范）

| 层级 | 目录 | 职责 |
|------|------|------|
| 原生内核层 | `src/kernel/` | 算法与仿真核心，可含内部指针 |
| 适配封装层 | `src/adapter/` | JSON ↔ 内核，无裸指针对外 |
| 流程调度层 | `src/scheduler/` | 路由、信封解析、Skill 注册、Context Store |

**横切基础库（非 Skill）：** `src/common/status`、`src/common/io`、`src/common/logger` — 进程内 C++ API，**不**注册为 `skill_id`。

### 2.2 当前主程序调用链

```
main.cpp
  ├─ registerBuiltinSkills(Dispatcher)   // skill_registry + skills/<id>/manifest.json
  ├─ CLI: --list / --describe / --health
  └─ stdin → Dispatcher::execute(envelope_json)
        ├─ parseSkillEnvelope（protocol v1；可选 context / options）
        ├─ ContextStore（若带 context_id）
        ├─ validatePayloadAgainstManifest
        ├─ <skill>_execute(payload_json)   // adapter → StatusOr<result>
        └─ makeSkillApiResponse / makeSkillApiFailureResponse
```

内置 Skill：`rainflow`、`host_echo`、`mesh_import`（会话/句柄样板，非真实网格导入）。

---

## 3. 现状评估

### 3.1 已具备（相对 v1.4 路线图）

| 能力 | 实现位置 | 状态 |
|------|----------|------|
| 协议 v1 | `skill_protocol` + `dosc/skill-protocol-v1.md` | ✅ |
| Skill 标识与 manifest | `skills/<id>/manifest.json` | ✅ |
| 发现 | `--list` / `--describe` | ✅ |
| 探活 | `--health` | ✅ |
| payload 校验 | `skill_payload_validator`（aistudy-schema-v1 子集 + 字段路径） | ✅ |
| 契约测试 + CI | GTest + `.github/workflows/ci.yml` | ✅ |
| 请求级日志 | `AIstudy.Dispatcher` + `request_id` | ✅ |
| 注册校验 | `skill_registry_bindings.txt` + `AistudySkills.cmake` | ✅ |
| 会话句柄 | `ContextStore`、`context` 信封、`mesh_import` 样板 | ✅ M5b |
| 错误 message | 信封 / payload / Store 均带字段路径 | ✅ |

### 3.2 与成熟 Agent + Skill 的差距

| 维度 | 当前 | 目标 |
|------|------|------|
| 注册 | `kBindings` 手工 + CMake 校验 | 代码生成 / 子进程 Skill（M7） |
| 传输 | stdin 单行 JSON | stdio 固化 + HTTP/MCP（M6） |
| 超时 | `options.timeout_ms` 仅日志 | 调度层强制取消 |
| FEM 领域 | `mesh_import` stub | 真实导入/求解/导出 Skill |
| 会话生命周期 | 进程内存 Store | TTL、`context_close`（M7） |
| Schema | aistudy-schema-v1 子集 | `oneOf`/`pattern` 等扩展 |
| 异步 | 同步 `execute` | `execute_async` + `job_id`（M7） |

### 3.3 `common/logger` 为何不是 Skill？

平台基础设施（status / io / logger）供 Skill 与 Dispatcher **内部**使用；Agent 通过 `execute` 间接获得可观测性（日志中的 `request_id`），无需 `write_log` 类 Skill。

---

## 4. 成熟 Agent + Skill 参考模型

### 4.1 角色划分

```
┌─────────────────────────────────────────────────────────┐
│ Agent Runtime（LLM / Cursor Agent）                      │
│ 读 manifest，组 envelope，多轮编排                       │
└──────────────────────────┬──────────────────────────────┘
                           │ 协议（stdio / HTTP / MCP）
┌──────────────────────────▼──────────────────────────────┐
│ Skill Host（AIstudy / scheduler）                        │
│ list / describe / execute / health                       │
└──────────────────────────┬──────────────────────────────┘
                           │
        adapter/*  →  kernel/*
```

### 4.2 Manifest

契约真源：`skills/<skill_id>/manifest.json`。Agent 与文档不依赖 C++ 头文件。

### 4.3 信封协议

**以 [`skill-protocol-v1.md`](skill-protocol-v1.md) 为准**（含 §2.4 `context`、§2.3 错误路径示例）。本文不再重复完整 JSON，避免与实现漂移。

### 4.4 Capabilities

| 能力 | 当前 CLI / API |
|------|----------------|
| `list_skills` | `AIstudy --list` |
| `describe_skill` | `AIstudy --describe <skill_id>` |
| `execute` | stdin 信封 |
| `health` | `AIstudy --health` |

MCP 映射见 [`mcp-tool-alignment.md`](mcp-tool-alignment.md)。

---

## 5. 与项目强制规范的对齐

### 5.1 函数指针

`SkillExecuteFunc` 仅存在于 **scheduler 进程内**静态表（`skill_registry.cpp`），不进入 adapter/kernel 公开 API。演进见 M7 子进程方案。

### 5.2 资源句柄 ID

M5b 已实现：`context_id` + `mesh_`/`result_`/`file_` 前缀句柄、`artifact://` URI、内存 ContextStore。rainflow 仍无状态；真实网格 I/O 待后续 Skill。

### 5.3 新增 Skill 流程

见 [`add-skill-checklist.md`](add-skill-checklist.md)。

---

## 6. 分阶段路线图（实现状态）

### 阶段 A：Skill 契约扎实化 — ✅ 已完成

协议 v1、manifest、统一 `ok`/`error`/`meta`、payload 校验、describe、rainflow 契约测试。

### 阶段 B：可扩展注册 — ✅ 主体完成

`skill_registry`、`registerBuiltinSkills`、CMake 与 `bindings.txt` 校验、`host_echo` 第二样板。未做：子进程 / 动态插件。

### 阶段 C：Agent Runtime 与 Host 分离 — ⏳ 未开始（M6）

目标：stdio 行为固化、可选 HTTP/MCP；身份说明见 README / 计划中的 `host-runtime.md`。

### 阶段 D：编排、会话、句柄 — ✅ 最小实现（M5b）

Context Store、`mesh_import` 样板、两步 `context_id` 测试。未做：异步 job、真实 `file_`→`mesh_` 业务链文档、HDF5 网格持久化。

### 阶段 E：工程化与运维 — ✅ 主体完成

结构化日志、`health`、多 Skill 契约测试、GitHub Actions CI。未做：指标、限流、全 Skill golden 覆盖。

---

## 7. `src/common` 定位

| 模块 | 角色 | 做成 Skill？ |
|------|------|-------------|
| `common/status` | `StatusOr`、错误码、API 响应 | 否 |
| `common/io/json` | manifest、payload | 否 |
| `common/io/hdf5` | artifact（已接入，FEM 未用满） | 否 |
| `common/logger` | `request_id` 追踪 | 否 |

---

## 8. 与 Cursor / MCP 对齐

见 [`mcp-tool-alignment.md`](mcp-tool-alignment.md)。`context` 与 `options.timeout_ms` 已实现解析；超时**未**强制中断。

---

## 9. 预编译 common 库

开发期保持源码 `add_subdirectory`；协议稳定后再评估预编译 SDK（见 [`next-development-plan.md`](next-development-plan.md) §8）。

---

## 10. 优先级总览（更新后）

| 顺序 | 内容 | 状态 |
|------|------|------|
| 1–4 | 协议、manifest、校验、日志、CI | ✅ |
| 5 | context + 句柄（最小） | ✅ M5b |
| 6 | M6 stdio/HTTP/MCP | 下一步 |
| 7 | M7 异步 / 子进程 Skill | 待定 |

---

## 11. 附录：关键文件索引

| 文件 | 作用 |
|------|------|
| `dosc/skill-protocol-v1.md` | 协议 v1（含 context） |
| `dosc/next-development-plan.md` | M1–M7 与迭代计划 |
| `dosc/context-and-handles-design.md` | 句柄与会话设计 |
| `dosc/host-runbook.md` | 运行、日志、CI 查看 |
| `dosc/add-skill-checklist.md` | 新增 Skill 步骤 |
| `.github/workflows/ci.yml` | Windows MSVC + ctest |
| `src/scheduler/skill_context_store.*` | Context Store |
| `src/scheduler/context_handle_rules.*` | 句柄校验与解析 |
| `skills/mesh_import/` | FEM 样板 Skill |

---

## 12. 修订记录

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0–1.4 | 2026-05-21 | 初稿至协议 v1 / 阶段 A/B 落地 |
| 1.5 | 2026-05-21 | 全文 UTF-8 修复；同步 M3–M5b、CI、三 Skill；阶段 C–E 状态更新 |

---

*与 `.cursor/rules/fem-simulation-architecture.mdc` 冲突时以规则文件为准。*
