#ifndef COMMON_LOGGER_LOGGER_CONFIG_H
#define COMMON_LOGGER_LOGGER_CONFIG_H

#include "logger.h"
#include <cstddef>
#include <string>

namespace AIstudy {
namespace common {
namespace logger {

/** @brief Logger configuration (JSON/INI/Properties, runtime). */
struct LoggerConfig {
    LogLevel level = LogLevel::INFO;
    bool consoleOutput = true;
    bool fileOutput = false;
    std::string filePath;
    std::size_t maxFileSize = 100 * 1024 * 1024;  // 100 MiB
    int maxFileCount = 10;
    std::string rotateType = "size";  // "size" | "daily" | "weekly" | "monthly"
    std::string timeRotateUnit = "day";
    bool enableColor = true;
    bool enableTrace = true;   // file/line/function in format
    bool async = false;
    std::size_t asyncQueueSize = 0;   // 0 = unlimited
};

/** @brief Load LoggerConfig from Properties file (logging.* keys). */
LoggerConfig loadLoggerConfigFromProperties(const std::string& path);

/** @brief Load LoggerConfig from JSON (logger.* keys). Returns default if parse fails. */
LoggerConfig loadLoggerConfigFromJson(const std::string& path);

/** @brief Apply config to root logger (channels, formatter, level). */
void applyLoggerConfig(const LoggerConfig& cfg);

/** @brief Configure from JSON file; stores path for RefreshConfig. */
void configureLoggerFromJson(const std::string& path);

/** @brief Configure from Properties file (logging.loggers.root.*); stores path for RefreshConfig. */
void configureLoggerFromProperties(const std::string& path);

/** @brief Reload config from last file (JSON or Properties) and apply. No-op if none. */
void refreshLoggerConfig();

/** @brief Runtime: set root level. */
void setLoggerLevel(LogLevel level);

/** @brief Runtime: set file path, enable file output, re-apply. */
void setLoggerFilePath(const std::string& path);

/** @brief Runtime: enable/disable file output, re-apply. */
void setLoggerFileOutput(bool enable);

/** @brief Runtime: enable/disable console output, re-apply. */
void setLoggerConsoleOutput(bool enable);

} // namespace logger
} // namespace common
} // namespace AIstudy

#endif // COMMON_LOGGER_LOGGER_CONFIG_H
