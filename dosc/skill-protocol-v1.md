# Skill 调度协议 v1

> 适用宿主：`AIstudy`（`Dispatcher` + `skills/<skill_id>/manifest.json`�? 
> 传输：进�?stdin 一�?JSON / 未来可扩�?HTTP、MCP

本文档定�?Agent �?Skill Host 之间�?*唯一**调度契约，不包含其它信封格式�?

---

## 1. 约定

| �?| 说明 |
|----|------|
| 协议版本 | 固定字符�?`"1"`，字段名 `protocol` |
| Skill 标识 | `skill_id`，与 manifest �?`id` 一�?|
| 契约来源 | `skills/<skill_id>/manifest.json`（`input_schema` / `output_schema`�?|
| 业务载荷 | `payload` 对象，须满足 manifest �?`input_schema.required` |
| 追踪 ID | `request_id`；调用方可省略，Host �?`protocol == "1"` 时自动生�?UUID |

---

## 2. 执行请求（`execute`�?

### 2.1 信封结构

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
| `protocol` | �?| string | 必须�?`"1"` |
| `skill_id` | �?| string | 已注�?Skill �?id |
| `payload` | �?| object | 业务参数；调度层�?manifest 校验 `required` 字段 |
| `request_id` | �?| string | 用于日志与响应回传；省略时由 Host 生成 |
| `skill_version` | �?| string | 若提供且�?manifest `version` 不一致，返回校验错误 |
| `context` | �?| object | **预留**：会话与资源句柄（当前实现未解析�?|
| `options` | �?| object | **预留**：如 `timeout_ms`（当前实现未解析�?|

### 2.3 校验分层

1. **调度�?*：JSON 可解析、必填信封字段、`payload` 顶层 `required` 存在性（�?`skill_payload_validator`）�?
2. **适配�?*：字段类型、枚举、算法约束（�?`rainflow_execute` 内解析）�?
3. **内核�?*：算法与数值错误，�?`StatusOr` 映射为统一 `error`�?

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
| `meta.skill_id` | 实际执行�?Skill |
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
    "message": "..."
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
| `error.code` | 整数错误码（�?`common/status/exception/error_codes.h`�?|
| `error.category` | 错误类别字符�?|
| `error.message` | 人类可读说明 |

信封解析失败且无法确�?`request_id` 时，`request_id` 可为空字符串；`meta` �?`skill_id` / `skill_version` 在未�?Skill 时可能省略或仅部分填充�?

---

## 4. Skill 发现（CLI�?

不经�?`execute` 信封，由宿主命令行提供：

| 命令 | 输出 |
|------|------|
| `AIstudy --list` | `{ "skills": [ { "id", "version", "title", "description", "deprecated" }, ... ] }` |
| `AIstudy --describe <skill_id>` | 完整 manifest 摘要（含 `input_schema`、`output_schema`、`tags` 等） |

Agent 编排前应优先 `--describe` 获取契约，再构�?`payload`�?

---

## 5. 注册与目�?

| 路径 | 作用 |
|------|------|
| `skills/<skill_id>/manifest.json` | Skill 契约 |
| `src/scheduler/skill_registry.cpp` | 编译�?`skill_id` �?`execute` 函数绑定 |

新增 Skill：增�?manifest + �?`kBindings` 注册执行函数；契约以 manifest 为准�?

---

## 6. 示例：stdin 调用 rainflow

**请求（单�?JSON）：**

```json
{"protocol":"1","skill_id":"rainflow","payload":{"load_history":[1,2,3,2,1],"method":"ThreePoint"}}
```

**PowerShell�?*

```powershell
'{"protocol":"1","skill_id":"rainflow","payload":{"load_history":[1,2,3,2,1],"method":"ThreePoint"}}' |
  .\build\src\Release\AIstudy.exe
```

---

## 7. 与架构文档关�?

- 分层与句柄规则：`.cursor/rules/fem-simulation-architecture.mdc`
- 演进路线：`dosc/agent-skill-architecture-roadmap.md`
