# AIStudy

[![CI](https://github.com/lizhengchun68-cmd/AIStudy/actions/workflows/ci.yml/badge.svg)](https://github.com/lizhengchun68-cmd/AIStudy/actions/workflows/ci.yml)

面向 Agent 调度的 **Skill Host** 与有限元仿真分层代码库：内核（`kernel`）→ 适配（`adapter`）→ 调度（`scheduler`），对外仅通过 JSON 协议与字符串句柄交互，无裸指针出进程。

当前内置 Skill：`rainflow`（雨流计数）、`host_echo`（探活样板）、`mesh_import`（M5 会话/句柄样板，非真实网格导入）。

---

## 外部 Agent 最小阅读集

仅依赖以下三项即可完成 `rainflow` 成功/失败调用（迭代 1 验收路径）：

| 资源 | 路径 |
|------|------|
| 调度协议 | [`dosc/skill-protocol-v1.md`](dosc/skill-protocol-v1.md) |
| rainflow 契约 | [`skills/rainflow/manifest.json`](skills/rainflow/manifest.json) |
| Host 可执行文件 | 构建后的 `build/src/Release/AIstudy.exe` |

可选：`skills/rainflow/tests/request_ok.json` 作为 stdin 请求样例。

多步 FEM 会话与句柄：另读 [`dosc/context-and-handles-design.md`](dosc/context-and-handles-design.md)。

---

## 构建

要求：Windows + Visual Studio 2022、CMake 3.16+。

```powershell
cmake -S . -B build
cmake --build build --config Release
```

产物：`build/src/Release/AIstudy.exe`。CMake 会注入 `AISTUDY_PROJECT_ROOT` 指向源码根，用于加载 `skills/<skill_id>/manifest.json`。

---

## 运行 Host

**推荐在仓库根目录执行**，或保证 `AISTUDY_PROJECT_ROOT` 指向含 `skills/` 的目录。

| 命令 | 说明 |
|------|------|
| `AIstudy --help` | 帮助 |
| `AIstudy --list` | 已注册 Skill（JSON），含 `load_errors` |
| `AIstudy --describe <skill_id>` | manifest / input_schema / output_schema |
| `AIstudy --health` | 探活：`skills_loaded`、`checks.poco`、`checks.hdf5` |
| `AIstudy --serve [port]` | 本地 HTTP API（默认 `8765`，仅 `127.0.0.1`） |
| `AIstudy`（无参数） | stdin 一行 JSON 信封 → stdout 一行 JSON；退出码见下 |

**stdio 退出码：** `0` 成功 / `1` 用法错误 / `2` 业务 `ok:false` / `3` Host 异常。详见 [`dosc/host-runtime.md`](dosc/host-runtime.md)。

**rainflow 示例（PowerShell）：**

```powershell
Get-Content skills\rainflow\tests\request_ok.json -Raw | .\build\src\Release\AIstudy.exe
```

manifest 加载失败时，stderr 会输出 `[AIstudy] WARNING: failed to load skill ...`，`--list` 的 `load_errors` 也会列出详情。

更完整的排错与日志说明见 [`dosc/host-runbook.md`](dosc/host-runbook.md)。

---

## 本地测试

```powershell
cmake -S . -B build -DAISTUDY_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

契约测试覆盖 rainflow golden、payload 校验负例、registry 加载失败、`--health`、context/句柄与 `mesh_import` 等。

**GitHub Actions（开发阶段分支策略）：**

| 流向 | 是否跑 CI |
|------|-----------|
| `zcli/fea_dev` → `group/fea_dev`（PR） | ✅ |
| `group/fea_dev` → `dev/fea_dev`（PR） | ✅ |
| 直接 push 到 `zcli/fea_dev` 或其它分支 | ❌ |

配置见 [`.github/workflows/ci.yml`](.github/workflows/ci.yml)；也可在 Actions 页 **Run workflow** 手动触发。需将 `thirdparty/` 预编译依赖一并提交到仓库。

查看运行状态：仓库 **Actions** → 选中 Run → 各步骤有中文标题；日志开头会打印 `PR: 源分支 -> 目标分支`。

---

## 目录结构（摘要）

```
skills/<skill_id>/manifest.json   # Skill 契约（Agent 真源）
src/kernel/                       # 算法与领域实现
src/adapter/                      # JSON ↔ kernel，无指针对外
src/scheduler/                    # Dispatcher、协议解析、注册表、Context Store
dosc/                             # 协议、路线图、开发计划、运行手册
tests/                            # GTest（契约与调度行为）
```

新增 Skill 流程：[`dosc/add-skill-checklist.md`](dosc/add-skill-checklist.md)。

---

## 文档索引

完整列表见 [`dosc/README.md`](dosc/README.md)。

| 文档 | 说明 |
|------|------|
| [`dosc/skill-protocol-v1.md`](dosc/skill-protocol-v1.md) | 信封与响应格式（**协议真源**） |
| [`dosc/next-development-plan.md`](dosc/next-development-plan.md) | 里程碑 M1–M7、迭代状态（当前：**M7**） |
| [`dosc/host-runtime.md`](dosc/host-runtime.md) | Host 边界、stdio/HTTP、退出码（M6） |
| [`dosc/host-runbook.md`](dosc/host-runbook.md) | 工作目录、日志 grep `request_id`、常见问题 |
| [`dosc/context-and-handles-design.md`](dosc/context-and-handles-design.md) | `context_id`、句柄、`artifact://` |
| [`dosc/next-development-plan.md`](dosc/next-development-plan.md) | M1–M7 里程碑与迭代计划 |
| [`dosc/agent-skill-architecture-roadmap.md`](dosc/agent-skill-architecture-roadmap.md) | 架构演进路线图 |
| [`dosc/mcp-tool-alignment.md`](dosc/mcp-tool-alignment.md) | MCP Tool 与 `skill_id` 对齐说明 |
| [`.cursor/rules/fem-simulation-architecture.mdc`](.cursor/rules/fem-simulation-architecture.mdc) | 分层与句柄强制规则 |

---

## 架构原则（简）

- 三层：`kernel` / `adapter` / `scheduler`；模块间禁无序强依赖。
- 对外 API：句柄 ID、配置结构体；**禁止**裸指针与函数指针出模块边界。
- 平台库（`common/status`、`common/logger`、`common/io`）不作为 Skill 注册。

---

## 许可证

见仓库内各组件说明；第三方依赖见 `thirdparty/`。
