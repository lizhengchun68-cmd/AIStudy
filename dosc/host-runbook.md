# AIstudy Skill Host 运行手册

> 对应里程碑 M3：本地运行、探活、日志与 manifest 路径说明。

## 1. 可执行文件与工作目录

- 构建产物：`build/src/Release/AIstudy.exe`（或 Debug 配置对应目录）。
- **推荐**：在仓库根目录 `AIStudy/` 下运行，或确保编译时注入了 `AISTUDY_PROJECT_ROOT`（CMake 已设为源码根）。
- Host 通过 `AISTUDY_PROJECT_ROOT/skills/<skill_id>/manifest.json` 加载契约；路径错误时 `--list` 的 `load_errors` 会列出失败项。

## 2. CLI 命令

| 命令 | 说明 |
|------|------|
| `AIstudy --list` | 已注册 Skill 列表（JSON），含 `load_errors` |
| `AIstudy --describe rainflow` | 输出该 Skill 的 manifest / schema |
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

## 5. 本地回归（无 CI）

```powershell
cmake --build build --config Release
cd build
ctest -C Release
```

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
