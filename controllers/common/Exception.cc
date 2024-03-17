#include <cstdarg>
#include <cstdio>

#include "Exception.hh"

namespace aegir {
  Exception::Exception(const char *_fmt, ...) {
    char buff[1024];
    int len;

    std::va_list args;
    va_start(args, _fmt);
    len = std::vsnprintf(buff, (std::size_t)sizeof(buff)-1, _fmt, args);
    va_end(args);
    c_msg = std::string(buff, len);
  }

  Exception::~Exception() {
  }

  const char *Exception::what() const noexcept {
    return c_msg.c_str();
  }

}
