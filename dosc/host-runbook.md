# AIstudy Skill Host 运行手册

> 对应 M3（可运维）+ CI：本地运行、探活、日志与 manifest 路径。里程碑状态见 [next-development-plan.md](next-development-plan.md)。

## 1. 可执行文件与工作目录

- 构建产物：`build/src/Release/AIstudy.exe`（或 Debug 配置对应目录）。
- **推荐**：在仓库根目录 `AIStudy/` 下运行，或确保编译时注入了 `AISTUDY_PROJECT_ROOT`（CMake 已设为源码根）。
- Host 通过 `AISTUDY_PROJECT_ROOT/skills/<skill_id>/manifest.json` 加载契约；路径错误时 `--list` 的 `load_errors` 会列出失败项。

## 2. CLI 命令

| 命令 | 说明 |
|------|------|
| `AIstudy --list` | 已注册 Skill 列表（JSON），含 `load_errors` |
| `AIstudy --describe <skill_id>` | 输出 manifest / schema（如 `rainflow`、`host_echo`） |
| `AIstudy --health` | 探活 JSON：`skills_loaded`、`checks.poco`、`checks.hdf5` |
| `AIstudy`（无参数） | 从 **stdin** 读入一条协议 v1 信封 JSON，向 **stdout** 输出一条响应 JSON |

帮助：`AIstudy --help`

## 3. stdin 执行示例（PowerShell）

```powershell
Get-Content skills\rainflow\tests\request_ok.json -Raw | .\build\src\Release\AIstudy.exe
```

或在仓库根：

```powershell
cd D:\code\AIStudy
Get-Content skills\rainflow\tests\request_ok.json -Raw | .\build\src\Release\AIstudy.exe
```

## 4. 请求级日志（grep `request_id`）

模块名：`AIstudy.Dispatcher`。

- 入口：`execute begin request_id=... skill_id=...`
- 若信封含 `options.timeout_ms`：`options.timeout_ms=N (reserved: not enforced; logged only)`
- 出口：`execute end request_id=... skill_id=... duration_ms=... ok=... error_code=... category=...`

**说明**：`options.timeout_ms` 已解析并写日志，**尚未**实现取消/超时中断（预留 M6+）。

## 5. 回归测试（本地与 CI）

```powershell
cmake -S . -B build -DAISTUDY_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

GitHub Actions：仓库根 [`.github/workflows/ci.yml`](../.github/workflows/ci.yml)。**仅**在以下 PR 时自动执行（push 到 `zcli/fea_dev` 不触发）：

- `zcli/fea_dev` → `group/fea_dev`
- `group/fea_dev` → `dev/fea_dev`

**如何查看 CI 进度：** GitHub → Actions → 当前 Run。步骤顺序为：运行信息 → 依赖检查 → Configure → Build → 枚举测试 → ctest。日志使用 `::group::` 折叠长输出；测试失败会附带 `LastTest.log` 尾部并上传 **ctest-logs** 构件。Run 页 **Summary** 有结果表格与本地复现命令。也可在 Actions 页手动 **Run workflow**（`workflow_dispatch`）。

## 6. 常见问题

| 现象 | 原因 | 处理 |
|------|------|------|
| `--list` 中 `load_errors` 含 rainflow | manifest 路径不对或 JSON 无效 | 确认 `AISTUDY_PROJECT_ROOT` 指向含 `skills/` 的目录 |
| `--health` 中 `ok: false` | 无 Skill 注册成功 | 先修 manifest，再看 stderr WARNING |
| `checks.hdf5: false` | 构建未启用 `common_io_hdf5` | 检查 `thirdparty/hdf5` 是否完整；不影响 rainflow stdin 调用 |

## 7. 协议字段（M3 相关）

信封可选 `options`：

```json
"options": { "timeout_ms": 30000 }
```

与 `skill-protocol-v1.md` 一致；Host 仅记录日志，不强制超时。
