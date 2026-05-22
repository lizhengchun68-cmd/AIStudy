# 新增 Skill 检查清单

> 对应 M4：在 `rainflow` / `host_echo` 样板基础上扩展。个人开发者按序勾选即可。

## 0. 命名与目录

- [ ] 选定 `skill_id`（小写+下划线，如 `mesh_import`）
- [ ] 创建 `skills/<skill_id>/manifest.json`
- [ ] 在 `src/scheduler/skill_registry_bindings.txt` 增加一行 `skill_id`
- [ ] 在 `src/scheduler/skill_registry.cpp` 的 `kBindings[]` 增加 `{ "skill_id", adapter::...::execute }`

**CMake 配置时会校验**：`bindings.txt` 与 `skills/*/manifest.json` 目录一一对应，不一致则 `cmake` 失败。

## 1. manifest（唯一契约源）

- [ ] `id` / `version` / `title` / `description`
- [ ] `input_schema`：建议根对象 `additionalProperties: false`
- [ ] `required`、类型、`enum`、`minItems` 等仅使用 [aistudy-schema-v1 子集](skill-protocol-v1.md)
- [ ] `output_schema` 描述 `result` 结构
- [ ] `examples` 至少 1 条合法输入

## 2. 内核层（有算法/领域逻辑时）

- [ ] `src/kernel/<module>/` 实现核心逻辑
- [ ] 对外经 adapter 调用；内核内部可用指针，**禁止**向 scheduler 暴露指针/函数指针

## 3. 适配层

- [ ] `src/adapter/<skill>_adapter.h/.cpp`
- [ ] 签名：`StatusOr<Poco::JSON::Object::Ptr> xxx_execute(const std::string& payload_json_str)`
- [ ] 解析 payload、调用 kernel、组装 `result` 对象（不含 API 信封）
- [ ] 在 `src/adapter/CMakeLists.txt` 加入源文件

## 4. 注册与构建

- [ ] `kBindings` 与 `skill_registry_bindings.txt` 同步
- [ ] `cmake --build build --config Release` 通过
- [ ] `AIstudy --list` 出现新 Skill且无 `load_errors`
- [ ] `AIstudy --describe <skill_id>` 输出 manifest

## 5. 契约测试（本地门禁，无需 CI）

- [ ] `skills/<skill_id>/tests/request_ok.json`
- [ ] `skills/<skill_id>/tests/expected_ok.json`
- [ ] 可选负例：`request_invalid_*.json`
- [ ] `tests/<skill>_contract_test.cpp` 或扩展现有 GTest
- [ ] `cd build && ctest -C Release` 全绿

## 6. 文档

- [ ] 若 Agent 暴露为 MCP Tool：更新 `dosc/mcp-tool-alignment.md` 映射表
- [ ] 复杂 payload 在 manifest `description` / `examples` 中写清

## 参考样板

| Skill | 类型 | 路径 |
|-------|------|------|
| rainflow | kernel + adapter + 算法 | `skills/rainflow/` |
| host_echo | 仅 adapter（Host 烟测） | `skills/host_echo/` |
