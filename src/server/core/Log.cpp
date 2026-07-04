#include "Log.hh"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace jstine::log {

static Level currentLogLevel{Level::trace};

void init(const LoggerOptions& options) {
    auto colorSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    if (options.enableColors) {
        colorSink->set_color(spdlog::level::trace, colorSink->green);
        colorSink->set_color(spdlog::level::debug, colorSink->magenta);
        colorSink->set_color(spdlog::level::info, colorSink->white);
        colorSink->set_color(spdlog::level::warn, colorSink->yellow);
        colorSink->set_color(spdlog::level::err, colorSink->red);
        colorSink->set_color(spdlog::level::critical, colorSink->red_bold);
        colorSink->set_color(spdlog::level::off, colorSink->reset);
    } else {
        colorSink->set_color(spdlog::level::trace, colorSink->reset);
        colorSink->set_color(spdlog::level::debug, colorSink->reset);
        colorSink->set_color(spdlog::level::info, colorSink->reset);
        colorSink->set_color(spdlog::level::warn, colorSink->reset);
        colorSink->set_color(spdlog::level::err, colorSink->reset);
        colorSink->set_color(spdlog::level::critical, colorSink->reset);
        colorSink->set_color(spdlog::level::off, colorSink->reset);
    }

    auto logger = std::make_shared<spdlog::logger>("global_logger", colorSink);
    spdlog::set_default_logger(logger);

    spdlog::set_pattern(options.formatPattern);
    spdlog::set_level(spdlog::level::trace);
}

Level level() { return currentLogLevel; }

void setLogLevel(Level level) {
    currentLogLevel = level;

    switch (level) {
        case Level::trace:
            spdlog::set_level(spdlog::level::trace);
            break;
        case Level::debug:
            spdlog::set_level(spdlog::level::debug);
            break;
        case Level::info:
            spdlog::set_level(spdlog::level::info);
            break;
        case Level::warn:
            spdlog::set_level(spdlog::level::warn);
            break;
        case Level::error:
            spdlog::set_level(spdlog::level::err);
            break;
        case Level::critical:
            spdlog::set_level(spdlog::level::critical);
            break;
        case Level::off:
            spdlog::set_level(spdlog::level::off);
            break;
    }
}

}  // namespace jstine::log
