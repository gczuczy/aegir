#include <cstdio>

#include "LogChannel.hh"

namespace bl = ::boost::log;
namespace blt = ::boost::log::trivial;
namespace blk = ::boost::log::keywords;

namespace aegir {

  LogChannel::LogChannel(const std::string _name, const blt::severity_level _level):
    c_channel(blk::severity = _level, blk::channel = _name),
    c_level(_level) {
  }

  LogChannel::~LogChannel() {
  }

  void LogChannel::log(const char* _fmt, ...) {
    std::va_list args;

    va_start(args, _fmt);
    log(c_level, _fmt, args);
    va_end(args);
  }

  void LogChannel::log(const blt::severity_level _level,
		       const char* _fmt, std::va_list _args) {
    char buff[1024];
    int len;

    len = std::vsnprintf(buff, (std::size_t)sizeof(buff)-1, _fmt, _args);

    if ( bl::record rec = c_channel.open_record(blk::severity = _level) ) {
      bl::record_ostream strm(rec);
      strm << std::string(buff, len);
      strm.flush();
      c_channel.push_record(boost::move(rec));
    }
  }

  void LogChannel::trace(const char* _fmt, ...) {
    std::va_list args;

    va_start(args, _fmt);
    log(blt::severity_level::trace, _fmt, args);
    va_end(args);
  }

  void LogChannel::debug(const char* _fmt, ...) {
    std::va_list args;

    va_start(args, _fmt);
    log(blt::severity_level::debug, _fmt, args);
    va_end(args);
  }

  void LogChannel::info(const char* _fmt, ...) {
    std::va_list args;

    va_start(args, _fmt);
    log(blt::severity_level::info, _fmt, args);
    va_end(args);
  }

  void LogChannel::warning(const char* _fmt, ...) {
    std::va_list args;

    va_start(args, _fmt);
    log(blt::severity_level::warning, _fmt, args);
    va_end(args);
  }

  void LogChannel::error(const char* _fmt, ...) {
    std::va_list args;

    va_start(args, _fmt);
    log(blt::severity_level::error, _fmt, args);
    va_end(args);
  }

  void LogChannel::fatal(const char* _fmt, ...) {
    std::va_list args;

    va_start(args, _fmt);
    log(blt::severity_level::fatal, _fmt, args);
    va_end(args);
  }

}
