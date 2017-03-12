/*
  Ageir-brewd's internal exception handling
 */

#ifndef AEGIR_EXCEPTION_H
#define AEGIR_EXCEPTION_H

#include <exception>

#include <string>

namespace aegir {
  class Exception {
  public:
    Exception(const char *_fmt, ...);
    virtual ~Exception();

    virtual const char *what() const noexcept;

  private:
    std::string c_msg;
  };
}

#endif
