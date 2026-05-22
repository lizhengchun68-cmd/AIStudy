# MCP / Cursor Agent 与 Skill Host 对齐说明

> M4 交付：便于在 Cursor 中将 `AIstudy` Skill 配置为 MCP Tool 或脚本工具。

## 1. 概念映射

| Skill Host | MCP / Agent 常见概念 |
|------------|----------------------|
| `skill_id` | Tool **name**（建议与 manifest `id` 一致，如 `rainflow`） |
| `manifest.input_schema` | Tool **parameters** / JSON Schema（Agent 填 `payload`） |
| `manifest.output_schema` | 结果形状说明（对应响应 `result`） |
| `AIstudy --describe <skill_id>` | 拉取完整契约（等同 Tool schema 文档） |
| `AIstudy --list` | 枚举可用 Tool |
| stdin `execute` 信封 | Tool **invoke** 请求体 |
| 响应 `ok` / `error` / `meta` | Tool 返回；`request_id` 用于日志串联 |

## 2. 调用信封（与 `skill-protocol-v1.md` 一致）

```json
{
  "protocol": "1",
  "request_id": "optional-uuid",
  "skill_id": "host_echo",
  "payload": { "message": "hello" }
}
```

Agent 生成 payload 前应阅读 `input_schema`（通过 `--describe` 或仓库内 `skills/<id>/manifest.json`）。

## 3. 当前内置 Skill 映射表

| skill_id | Tool 建议名 | 必填 payload | 说明 |
|----------|-------------|--------------|------|
| `rainflow` | `rainflow` | `load_history`, `method` | 雨流计数 |
| `host_echo` | `host_echo` | `message` | 回显烟测 |
| `mesh_import` | `mesh_import` | `source_path`；信封需 `context.context_id` | 网格导入骨架（M5b） |

## 4. 推荐 Agent 工作流

1. `AIstudy --health` — 确认 Host 与 Skill 已加载  
2. `AIstudy --describe <skill_id>` — 获取 schema  
3. 构造信封 JSON，`payload` 满足 schema  
4. 管道 stdin 执行，解析 stdout JSON  
5. 若 `ok: false`，读 `error.message`（含字段路径）自动改参  

## 5. 运行方式（脚本 / MCP stdio）

见 [host-runbook.md](host-runbook.md)。工作目录或 `AISTUDY_PROJECT_ROOT` 须指向含 `skills/` 的仓库根。

## 6. 信封扩展字段

| 信封字段 | 状态 |
|----------|------|
| `context` | ✅ M5b：`context_id` + 可选 `handles`；`mesh_import` 等需会话的 Skill 必填 |
| `options.timeout_ms` | 已解析并写日志，**未**强制超时（M6+） |

详见 [`skill-protocol-v1.md`](skill-protocol-v1.md) §2.4 与 [`context-and-handles-design.md`](context-and-handles-design.md)。
