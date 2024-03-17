/*
  Logging subsystem basics
 */

#ifndef AEGIR_LOGGING_H
#define AEGIR_LOGGING_H

#include <string>
#include <functional>

#include <boost/log/trivial.hpp>
namespace blt = ::boost::log::trivial;

namespace aegir {
  namespace logging {

    void init();
    std::string str(blt::severity_level _level);
    void setGetLogLevel(std::function<blt::severity_level()> _getloglevel);
  }
}

#endif
