# AIstudy Skill Host 运行时（M6）

> `AIstudy.exe` 的定位：**Skill Host**（计算网关），不是 Agent Runtime。  
> Agent / Cursor / 脚本负责组信封；Host 负责发现、校验、调度、返回协议 v1 JSON。

---

## 1. Host 边界

| 属于 Host | 不属于 Host |
|-----------|-------------|
| `--list` / `--describe` / `--health` / `--drop-context` | LLM 推理、多轮对话记忆 |
| stdin **一行** JSON `execute` | 内置 Workflow DAG |
| `--serve` HTTP JSON API（本地） | 业务算法（在 kernel/adapter） |
| 加载 `skills/*/manifest.json` | 将 `logger` 注册为 Skill |

依赖库：`common/status`、`common/logger`、`common/io` 仅进程内使用。

---

## 2. 传输方式

### 2.1 stdio（默认，推荐脚本/子进程）

- **输入：** stdin 一整行 JSON 信封（可含尾部换行，Host 读取全部 stdin）。
- **输出：** stdout **一行** JSON 响应 + 换行。
- **日志：** stderr（manifest WARNING、HTTP 监听提示、Poco 日志）。

与 [`skill-protocol-v1.md`](skill-protocol-v1.md) 完全一致。

### 2.2 HTTP（`--serve`，本地集成）

```powershell
.\build\src\Release\AIstudy.exe --serve 8765
```

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/v1/health` | 同 `--health` |
| GET | `/v1/skills` | 同 `--list` |
| GET | `/v1/skills/<skill_id>` | 同 `--describe` |
| POST | `/v1/execute` | body = 信封 JSON；响应体 = 执行结果 |

**curl 示例（rainflow）：**

```bash
curl -s http://127.0.0.1:8765/v1/health
curl -s -X POST http://127.0.0.1:8765/v1/execute ^
  -H "Content-Type: application/json" ^
  -d "{\"protocol\":\"1\",\"skill_id\":\"rainflow\",\"payload\":{\"load_history\":[1,2,3,2,1],\"method\":\"ThreePoint\"}}"
```

HTTP 状态码：`200` + `ok:true`；`422` + `ok:false`（业务/校验失败）；`500` 响应无法解析。

仅绑定 **127.0.0.1**；M6 不提供 TLS/认证。

### 2.3 MCP

完整 MCP Server 未内置。Cursor 侧映射与推荐流程见 [`mcp-tool-alignment.md`](mcp-tool-alignment.md)。  
可行方式：将 `AIstudy.exe` 作为 **stdio 子进程**，由外部 MCP 桥接工具转发 `execute`（与 §2.1 相同）。

---

## 3. 进程退出码（stdio / CLI）

| 码 | 常量 | 含义 |
|----|------|------|
| 0 | Success | 命令成功；或 `execute` 响应 `ok: true` |
| 1 | UsageError | 参数错误、空 stdin、`--help` 以外非法参数 |
| 2 | ExecuteFailed | `execute` 响应 `ok: false` |
| 3 | HostError | 响应非 JSON、缺少 `ok` 字段等 |

实现：`src/host/skill_host.cpp` → `SkillHost::exitCodeFromExecuteResponse`。

**说明：** `--list` / `--describe` / `--health` 成功时均为 **0**（即使 describe 未知 skill 返回 JSON 错误对象，仍视为 CLI 成功）。

---

## 4. 稳定性验收

```powershell
cmake --build build --config Release
.\scripts\stdio_stress.ps1 -Count 100
```

默认 100 次 rainflow stdin 调用，要求全部 `exit 0` 且 `ok: true`。可调 `-Count`、`-Exe`。

---

## 5. 工作目录与 manifest

与 [`host-runbook.md`](host-runbook.md) 相同：在仓库根运行，或依赖编译期 `AISTUDY_PROJECT_ROOT`。

---

## 6. 与里程碑关系

| M6 任务 | 交付 |
|---------|------|
| Host 身份 | 本文 + README |
| stdio 固化 | 退出码 + `stdio_stress.ps1` |
| HTTP | `--serve` + `/v1/*` |
| MCP | 文档对齐（`mcp-tool-alignment.md`），无独立 MCP 二进制 |

后续 **M7**（见 [`next-development-plan.md`](next-development-plan.md)）：**M7a** 会话收尾 → **M7b** 大结果 handle → **M7c** 异步 job / 超时取消 → **M7d** 子进程（可选）。

---

## 7. 参考

- [`skill-protocol-v1.md`](skill-protocol-v1.md)
- [`host-runbook.md`](host-runbook.md)
- [`next-development-plan.md`](next-development-plan.md) § M6
