#include "logger_config.h"
#include "logger.h"
#include "common/io/json/json_reader.h"
#include <Poco/Logger.h>
#include <Poco/Message.h>
#include <Poco/Formatter.h>
#include <Poco/PatternFormatter.h>
#include <Poco/FormattingChannel.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/FileChannel.h>
#include <Poco/SplitterChannel.h>
#include <Poco/AsyncChannel.h>
#include <Poco/Util/PropertyFileConfiguration.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <mutex>
#include <sstream>

namespace AIstudy {
namespace common {
namespace logger {

using io::json::JsonReader;

namespace {

std::mutex& configMutex() {
    static std::mutex m;
    return m;
}

std::string g_lastConfigPath;
enum class ConfigSource { None, Json, Properties };
ConfigSource g_configSource = ConfigSource::None;
LoggerConfig g_currentConfig;

int levelToPoco(LogLevel l) {
    switch (l) {
        case LogLevel::TRACE:   return Poco::Message::PRIO_TRACE;
        case LogLevel::DEBUG:   return Poco::Message::PRIO_DEBUG;
        case LogLevel::INFO:    return Poco::Message::PRIO_INFORMATION;
        case LogLevel::WARNING: return Poco::Message::PRIO_WARNING;
        case LogLevel::ERROR:   return Poco::Message::PRIO_ERROR;
        case LogLevel::FATAL:   return Poco::Message::PRIO_FATAL;
    }
    return Poco::Message::PRIO_INFORMATION;
}

std::string defaultPattern() {
    return "%Y-%m-%d %H:%M:%S.%i [%p] [PID:%P/TID:%I] - %t";
}

Poco::Formatter::Ptr createFormatter(bool includeSource) {
    auto* f = new Poco::PatternFormatter();
    f->setProperty("pattern", includeSource ? "%Y-%m-%d %H:%M:%S.%i [%p] [PID:%P/TID:%I] [%s] [%O:%u] - %t" : defaultPattern());
    f->setProperty("times", "local");
    return Poco::Formatter::Ptr(f);
}

} // namespace

LoggerConfig loadLoggerConfigFromProperties(const std::string& path) {
    LoggerConfig out;
    try {
        auto* raw = new Poco::Util::PropertyFileConfiguration(path);
        Poco::Util::AbstractConfiguration::Ptr cfg(raw);
        std::string L;
        if (cfg->has("logging.loggers.root.level")) {
            L = cfg->getString("logging.loggers.root.level");
            if (L == "trace") out.level = LogLevel::TRACE;
            else if (L == "debug") out.level = LogLevel::DEBUG;
            else if (L == "information" || L == "info") out.level = LogLevel::INFO;
            else if (L == "warning" || L == "warn") out.level = LogLevel::WARNING;
            else if (L == "error") out.level = LogLevel::ERROR;
            else if (L == "critical" || L == "fatal") out.level = LogLevel::FATAL;
        }
        if (cfg->has("logging.loggers.root.console")) out.consoleOutput = (cfg->getString("logging.loggers.root.console") != "false");
        if (cfg->has("logging.loggers.root.file")) out.fileOutput = (cfg->getString("logging.loggers.root.file") == "true");
        if (cfg->has("logging.loggers.root.filePath")) out.filePath = cfg->getString("logging.loggers.root.filePath");
        if (cfg->has("logging.loggers.root.maxFileSize")) out.maxFileSize = static_cast<std::size_t>(cfg->getInt("logging.loggers.root.maxFileSize"));
        if (cfg->has("logging.loggers.root.maxFileCount")) out.maxFileCount = cfg->getInt("logging.loggers.root.maxFileCount");
        if (cfg->has("logging.loggers.root.enableColor")) out.enableColor = (cfg->getString("logging.loggers.root.enableColor") != "false");
        if (cfg->has("logging.loggers.root.enableTrace")) out.enableTrace = (cfg->getString("logging.loggers.root.enableTrace") != "false");
        if (cfg->has("logging.loggers.root.async")) out.async = (cfg->getString("logging.loggers.root.async") == "true");
    } catch (...) {}
    return out;
}

static bool jsonBool(JsonReader& r, const std::string& path, bool defaultVal) {
    auto o = r.getValueOptional<int>(path);
    if (o.ok() && !o.value().isNull()) return o.value().value() != 0;
    auto s = r.getValueOptional<std::string>(path);
    if (s.ok() && !s.value().isNull()) {
        std::string v = s.value().value();
        if (v == "true" || v == "1") return true;
        if (v == "false" || v == "0") return false;
    }
    return defaultVal;
}

LoggerConfig loadLoggerConfigFromJson(const std::string& path) {
    LoggerConfig out;
    JsonReader r;
    auto ok = r.readFile(path);
    if (!ok.ok()) return out;
    auto L = r.getValueOptional<std::string>("logger.level");
    if (L.ok() && !L.value().isNull()) {
        std::string s = L.value().value();
        if (s == "TRACE" || s == "trace") out.level = LogLevel::TRACE;
        else if (s == "DEBUG" || s == "debug") out.level = LogLevel::DEBUG;
        else if (s == "INFO" || s == "info" || s == "information") out.level = LogLevel::INFO;
        else if (s == "WARN" || s == "WARNING" || s == "warn" || s == "warning") out.level = LogLevel::WARNING;
        else if (s == "ERROR" || s == "error") out.level = LogLevel::ERROR;
        else if (s == "FATAL" || s == "fatal" || s == "critical") out.level = LogLevel::FATAL;
    }
    out.consoleOutput = jsonBool(r, "logger.console_output", true);
    out.fileOutput = jsonBool(r, "logger.file_output", false);
    auto fp = r.getValueOptional<std::string>("logger.file_path");
    if (fp.ok() && !fp.value().isNull()) out.filePath = fp.value().value();
    auto mfs = r.getValueOptional<int>("logger.max_file_size");
    if (mfs.ok() && !mfs.value().isNull()) out.maxFileSize = static_cast<std::size_t>(mfs.value().value());
    auto mfc = r.getValueOptional<int>("logger.max_file_count");
    if (mfc.ok() && !mfc.value().isNull()) out.maxFileCount = mfc.value().value();
    out.enableColor = jsonBool(r, "logger.enable_color", true);
    out.enableTrace = jsonBool(r, "logger.enable_trace", true);
    out.async = jsonBool(r, "logger.async", false);
    return out;
}

static void applyLoggerConfigImpl(const LoggerConfig& cfg) {
    using namespace Poco;
    Channel::Ptr target;

    if (cfg.consoleOutput) {
        Channel::Ptr console;
#if defined(POCO_OS_FAMILY_WINDOWS)
        (void)cfg;
        console = new ConsoleChannel();
#else
        if (cfg.enableColor) {
            auto* cc = new ColorConsoleChannel();
            cc->setProperty("enableColors", "true");
            console = cc;
        } else {
            console = new ConsoleChannel();
        }
#endif
        target = new FormattingChannel(createFormatter(cfg.enableTrace), console);
    }

    if (cfg.fileOutput && !cfg.filePath.empty()) {
        std::string parent = Poco::Path(cfg.filePath).parent().toString();
        if (!parent.empty() && parent != ".") { Poco::File d(parent); d.createDirectories(); }
        auto* fc = new FileChannel(cfg.filePath);
        std::ostringstream rot;
        rot << (cfg.maxFileSize / (1024 * 1024)) << " M";
        fc->setProperty("rotation", rot.str());
        fc->setProperty("archive", "number");
        fc->setProperty("purgeCount", std::to_string(cfg.maxFileCount));
        auto fchan = new FormattingChannel(createFormatter(cfg.enableTrace), fc);
        if (target) {
            auto* split = new SplitterChannel();
            split->addChannel(target);
            split->addChannel(Channel::Ptr(fchan));
            target = split;
        } else {
            target = fchan;
        }
    }

    if (!target) {
        target = new FormattingChannel(createFormatter(cfg.enableTrace), new ConsoleChannel());
    }

    if (cfg.async) {
        auto* ac = new AsyncChannel(target);
        if (cfg.asyncQueueSize > 0)
            ac->setProperty("queueSize", std::to_string(cfg.asyncQueueSize));
        target = ac;
    }

    Poco::Logger::root().setChannel(target);
    Poco::Logger::root().setLevel(levelToPoco(cfg.level));
}

void applyLoggerConfig(const LoggerConfig& cfg) {
    std::lock_guard<std::mutex> lock(configMutex());
    g_currentConfig = cfg;
    applyLoggerConfigImpl(cfg);
}

void configureLoggerFromJson(const std::string& path) {
    std::lock_guard<std::mutex> lock(configMutex());
    LoggerConfig cfg = loadLoggerConfigFromJson(path);
    g_lastConfigPath = path;
    g_configSource = ConfigSource::Json;
    g_currentConfig = cfg;
    applyLoggerConfigImpl(cfg);
}

void configureLoggerFromProperties(const std::string& path) {
    std::lock_guard<std::mutex> lock(configMutex());
    LoggerConfig cfg = loadLoggerConfigFromProperties(path);
    g_lastConfigPath = path;
    g_configSource = ConfigSource::Properties;
    g_currentConfig = cfg;
    applyLoggerConfigImpl(cfg);
}

void refreshLoggerConfig() {
    std::lock_guard<std::mutex> lock(configMutex());
    if (g_lastConfigPath.empty()) return;
    if (g_configSource == ConfigSource::Json) {
        LoggerConfig cfg = loadLoggerConfigFromJson(g_lastConfigPath);
        g_currentConfig = cfg;
        applyLoggerConfigImpl(cfg);
    } else if (g_configSource == ConfigSource::Properties) {
        LoggerConfig cfg = loadLoggerConfigFromProperties(g_lastConfigPath);
        g_currentConfig = cfg;
        applyLoggerConfigImpl(cfg);
    }
}

void setLoggerLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(configMutex());
    g_currentConfig.level = level;
    Poco::Logger::root().setLevel(levelToPoco(level));
}

void setLoggerFilePath(const std::string& path) {
    std::lock_guard<std::mutex> lock(configMutex());
    g_currentConfig.filePath = path;
    g_currentConfig.fileOutput = true;
    applyLoggerConfigImpl(g_currentConfig);
}

void setLoggerFileOutput(bool enable) {
    std::lock_guard<std::mutex> lock(configMutex());
    g_currentConfig.fileOutput = enable;
    applyLoggerConfigImpl(g_currentConfig);
}

void setLoggerConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(configMutex());
    g_currentConfig.consoleOutput = enable;
    applyLoggerConfigImpl(g_currentConfig);
}

} // namespace logger
} // namespace common
} // namespace AIstudy
