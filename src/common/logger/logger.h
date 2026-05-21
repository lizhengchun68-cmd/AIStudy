#ifndef COMMON_LOGGER_LOGGER_H
#define COMMON_LOGGER_LOGGER_H

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  undef ERROR
#  undef INFO
#endif

#include <string>

namespace AIstudy {
namespace common {
namespace logger {

/**
 * @brief Log level (maps to Poco::Message::Priority)
 */
enum class LogLevel {
    TRACE = 0,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

/**
 * @brief Logger encapsulation over Poco::Logger
 */
class Logger {
public:
    /**
     * @brief Get logger by name (hierarchical, e.g. "SimUtils", "SimUtils.IO")
     */
    static Logger get(const std::string& name);

    /**
     * @brief Default logger for macros (name "SimUtils")
     */
    static Logger defaultLogger();

    /**
     * @brief Initialize logging. Call setupDefaultConfiguration() if not yet configured.
     */
    static void initialize();

    /**
     * @brief Shutdown logging framework.
     */
    static void shutdown();

    /**
     * @brief Configure from Properties file (logging.* keys). See LOGGER checklist.
     */
    static void configureFromFile(const std::string& path);

    /**
     * @brief Setup default: ConsoleChannel + PatternFormatter, root level INFO.
     */
    static void setupDefaultConfiguration();

    void setLevel(LogLevel level);
    LogLevel getLevel() const;
    bool isLevelEnabled(LogLevel level) const;

    void trace(const std::string& msg);
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warning(const std::string& msg);
    void error(const std::string& msg);
    void fatal(const std::string& msg);

    void trace(const std::string& msg, const char* file, int line);
    void debug(const std::string& msg, const char* file, int line);
    void info(const std::string& msg, const char* file, int line);
    void warning(const std::string& msg, const char* file, int line);
    void error(const std::string& msg, const char* file, int line);
    void fatal(const std::string& msg, const char* file, int line);

    template <typename T, typename... Args>
    void trace(const std::string& fmt, T arg1, Args&&... args);
    template <typename T, typename... Args>
    void debug(const std::string& fmt, T arg1, Args&&... args);
    template <typename T, typename... Args>
    void info(const std::string& fmt, T arg1, Args&&... args);
    template <typename T, typename... Args>
    void warning(const std::string& fmt, T arg1, Args&&... args);
    template <typename T, typename... Args>
    void error(const std::string& fmt, T arg1, Args&&... args);
    template <typename T, typename... Args>
    void fatal(const std::string& fmt, T arg1, Args&&... args);

    const std::string& name() const { return name_; }

private:
    explicit Logger(std::string name) : name_(std::move(name)) {}
    std::string name_;

    int toPocoPriority(LogLevel) const;
    LogLevel fromPocoPriority(int) const;
};

} // namespace logger

#include "logger_impl.h"   
} // namespace common
} // namespace AIstudy
#endif // COMMON_LOGGER_LOGGER_H
