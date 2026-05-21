# common/common_status 错误码机制说明

`AIstudy` 命名空间下的状态与错误处理模块，供内核层、适配层等返回「成功值或错误码」，并在需要时转为异常或统一错误信息。

## 目录结构

| 路径 | 职责 |
|------|------|
| `status_or.h` | 值或错误容器 `StatusOr<T>` |
| `exception/error_category.h` | 错误分类常量 |
| `exception/error_codes.h` | 预定义错误码枚举与码段基值 |
| `exception/error_code_wrapper.h` | 错误码包装 `ErrorCodeWrapper` |
| `exception/error_code_registry.h` | 错误码注册表（线程安全） |
| `exception/error_info.h` | 统一错误信息 `ErrorInfo`、上下文、提取函数 |
| `exception/exception.h` | 分层异常类型 |
| `exception/error_handler.h` | 全局错误处理策略 |
| `exception/error_macros.h` | 检查/抛异常宏 |
| `exception/error_messages.h` | 预置错误文案注册入口 |

---

## 1. StatusOr\<T\>（`status_or.h`）

- 互斥持有：**成功值 `T`** 或 **`ErrorCodeWrapper`**
- 无默认构造；支持拷贝/移动/赋值（值与错误状态可切换）
- 状态判断：`ok()` / `failed()` / `operator bool`
- 取值：`value()`（失败时抛 `SimUtilsException`）、`value_or_default()`
- 错误侧：`status()`、`code()`、`message()`
- 运算符：`*` / `->`（语义同 `value()`）
- 隐式转为 `ErrorCodeWrapper`（成功时为“成功哨兵”）
- 工厂：`StatusOr::Ok` / `Fail`（支持 `ErrorCodeWrapper` 或 `code + category`）
- 流输出：`operator<<` → `[OK]` 或 `[FAIL] code message`

---

## 2. 错误分类（`error_category.h`）

- 分类字符串常量：`system`、`network`、`filesystem`、`memory`、`simulation`、`convergence`、`boundary_condition`、`mesh`、`solver`、`config`、`parameter`、`validation`、`thread`、`device`、`license`、`json`、`hdf5`
- `getCategoryDescription(category)`：分类的可读说明

---

## 3. 错误码定义（`error_codes.h`）

- **码段基值** `ErrorCodeBase`：按领域分段（从 10000 起，避免与系统 errno 冲突）
- **预定义枚举**（均 `int32_t`）：
  - `SystemError`
  - `NetworkError`
  - `FileSystemError`
  - `MemoryError`
  - `SimulationError`
  - `ConvergenceError`
  - `BoundaryConditionError`
  - `MeshError`
  - `SolverError`
  - `ConfigError`
  - `ParameterError`
  - `ValidationError`
  - `ThreadError`
  - `DeviceError`
  - `LicenseError`
  - `JsonError`
  - `Hdf5Error`
- Windows 下对冲突宏名的预处理清理

---

## 4. ErrorCodeWrapper（`error_code_wrapper.h`）

- 构造：默认（成功）、`(code)`、`(code, category)`
- 查询：`code()`、`category()`、`message()`（优先注册表文案，否则生成默认描述）
- 状态：`isSuccess()` / `isFailure()` / `explicit operator bool`（失败为 true）
- 比较：`==` / `!=` / `<`
- 与系统错误互转：`fromSystemError` / `toSystemError`
- 格式化：`toString()` → `category:code - message`
- 转换：`toErrorInfo()`
- 抛异常：`toException(message)`；带 `file/line/function` 的重载（按 **category** 映射到对应 `*Exception` 子类）

---

## 5. 错误码注册表（`error_code_registry.h`）

- 单例：`ErrorCodeRegistry::getInstance()`（线程安全）
- 注册：`registerErrorCode(code, category, description)`
- 查询：`getDescription`、`isRegistered`、`findErrorCode`
- 列举：`getErrorCodesByCategory`
- 统计：`getErrorCodeCount()`（全局/按分类）、`getErrorCodeStatistics()`
- 清空：`clear()`
- 便捷函数：`isValidErrorCode`、`validateErrorCode`

---

## 6. 预置文案（`error_messages.h` / `error_messages.cpp`）

- `initializeDefaultErrorMessages()`：将 `error_codes.h` 中各枚举的默认英文描述批量注册到 `ErrorCodeRegistry`（建议在进程启动时调用一次）

---

## 7. ErrorInfo 与上下文（`error_info.h`）

- **ExceptionContext**：`file`、`line`、`function`；`empty()` 判断
- **ErrorInfo**：`errorCode`、`message`、`category`、`context`、`timestamp`
  - `toString()` / `toFormattedString()`
  - `toJson()` / `fromJson()`（Poco JSON）
- **组装**：`getErrorInfo(error)` / `getErrorInfo(error, ctx)` / `getErrorInfo(error, ctx, messageOverride)`
- **格式化上下文**：`getFormattedMessage(ctx)`
- **从异常提取**：`extractErrorCode` / `extractErrorInfo`（支持 `SimUtilsException`、`std::exception`、`exception_ptr`）

---

## 8. 异常体系（`exception.h`）

- 基类：**SimUtilsException**（含 `errorCode()`、`toErrorInfo()`、异常链 `setCause` / `getCause` / `getExceptionChain`）
- 按领域派生（构造时绑定 `ErrorCodeWrapper` + 可选 `ExceptionContext`）：
  - `SimulationException`
  - `ConfigurationException`
  - `NetworkException`
  - `FileSystemException`
  - `MemoryException`
  - `ConvergenceException`
  - `SolverException`
  - `MeshException`
  - `ParameterException`
  - `ValidationException`
- `ErrorCodeWrapper::toException` 按 category 自动选择上述类型

---

## 9. 统一错误处理（`error_handler.h`）

- 策略 **ErrorHandlingStrategy**：`THROW_EXCEPTION`、`RETURN_ERROR_CODE`、`LOG_ONLY`（当前为占位无操作）
- 全局实例：**ErrorHandler::getGlobal()**
- `handleError(error, message)`：按策略抛错、返回错误或仅回调
- `handleException(e)`：提取 `ErrorInfo` 后按策略处理
- `setStrategy` / `strategy`
- `setCallback(fn)`：处理时可选 `ErrorInfo` 回调（如日志）

---

## 10. 便捷宏（`error_macros.h`）

- `SIMUTILS_THROW_IF_ERROR(ec, msg)`：失败则带源码位置抛异常
- `SIMUTILS_THROW_ON_ERROR`：同上（别名）
- `SIMUTILS_THROW_EXCEPTION(ec, msg)`：无条件抛异常（带位置）
- `SIMUTILS_CHECK_ERROR(ec)`：失败则用 `ec.message()` 抛异常（带位置）

---

## 典型用法（能力组合）

| 场景 | 使用能力 |
|------|----------|
| API 返回成功/失败 | `StatusOr<T>` + `ErrorCodeWrapper` |
| 使用预定义错误 | `error_codes.h` 枚举 + `ErrorCategory::*` |
| 可读错误信息 | `initializeDefaultErrorMessages()` + `message()` / 注册表 |
| 自定义错误码 | `ErrorCodeRegistry::registerErrorCode` |
| 日志/上报 JSON | `ErrorInfo::toJson` |
| 强制失败即异常 | `SIMUTILS_*` 宏或 `StatusOr::value()` |
| 统一兜底处理 | `ErrorHandler::getGlobal()` |
