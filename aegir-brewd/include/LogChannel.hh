/*
  A log channel with severity support
 */

#ifndef AEGIR_LOGCHANNEL_H
#define AEGIR_LOGCHANNEL_H

#include <string>

#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/trivial.hpp>

namespace blt = ::boost::log::trivial;

namespace aegir {
  class LogChannel {
  private:
  public:
    LogChannel(const std::string _name, blt::severity_level _level = blt::severity_level::info);
    LogChannel() = delete;
    ~LogChannel();

    void log(const char* _fmt, ...);

  private:
    boost::log::sources::severity_channel_logger_mt<blt::severity_level, std::string> c_channel;
  };
}

#endif

