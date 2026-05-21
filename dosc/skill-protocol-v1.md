# Skill 调度协议 v1

## 请求信封

兼容字段：`task_type` 与 `skill_id` 等价（遗留写法保留）。

```json
{
  "protocol": "1",
  "request_id": "optional-uuid",
  "skill_id": "rainflow",
  "skill_version": "1.0.0",
  "payload": { },
  "context": {
    "context_id": "session-abc",
    "handles": {}
  },
  "options": {
    "timeout_ms": 30000
  }
}
```

未指定 `protocol` 时按遗留模式处理（仅 `task_type` + `payload`，响应使用 `success` 字段）。

## 响应

协议 v1：

```json
{
  "protocol": "1",
  "request_id": "...",
  "ok": true,
  "result": { },
  "meta": {
    "skill_id": "rainflow",
    "skill_version": "1.0.0",
    "duration_ms": 12
  }
}
```

失败时 `ok: false`，`error` 为 `{ "code", "category", "message" }`。

## Capabilities

- `list_skills`：`Dispatcher::listSkillsJson()`
- `describe_skill`：`Dispatcher::describeSkillJson(skill_id)`
