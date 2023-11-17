#include <cstdarg>
#include <cstdio>

#include "LogChannel.hh"

namespace bl = ::boost::log;
namespace blt = ::boost::log::trivial;
namespace blk = ::boost::log::keywords;

namespace aegir {

  LogChannel::LogChannel(const std::string _name, blt::severity_level _level):
    c_channel(blk::severity = _level,
	      blk::channel = _name) {
  }

  LogChannel::~LogChannel() {
  }

  void LogChannel::log(const char* _fmt, ...) {
    char buff[1024];
    int len;

    std::va_list args;
    va_start(args, _fmt);
    len = std::vsnprintf(buff, (std::size_t)sizeof(buff)-1, _fmt, args);
    va_end(args);

    if ( bl::record rec = c_channel.open_record() ) {
      bl::record_ostream strm(rec);
      strm << std::string(buff, len);
      strm.flush();
      c_channel.push_record(boost::move(rec));
    }
  }
}
