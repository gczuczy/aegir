/*
  A log channel with severity support
 */

#ifndef AEGIR_LOGCHANNEL_H
#define AEGIR_LOGCHANNEL_H

#include <string>
#include <cstdarg>

#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/trivial.hpp>

namespace blt = ::boost::log::trivial;

namespace aegir {
  class LogChannel {
  private:
  public:
    LogChannel(const std::string _name, const blt::severity_level _level = blt::severity_level::info);
    LogChannel() = delete;
    ~LogChannel();

    // default severity
    void log(const char* _fmt, ...);
    void log(const blt::severity_level _level, const char* _fmt, std::va_list _args);
    // per-severity
    void trace(const char* _fmt, ...);
    void debug(const char* _fmt, ...);
    void info(const char* _fmt, ...);
    void warn(const char* _fmt, ...);
    void warning(const char* _fmt, ...);
    void error(const char* _fmt, ...);
    void fatal(const char* _fmt, ...);

  private:
    boost::log::sources::severity_channel_logger_mt<blt::severity_level, std::string> c_channel;
    blt::severity_level c_level;
  };
}

#endif

