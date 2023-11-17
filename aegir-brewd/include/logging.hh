/*
  Logging subsystem basics
 */

#ifndef AEGIR_LOGGING_H
#define AEGIR_LOGGING_H

#include <boost/log/trivial.hpp>
namespace blt = ::boost::log::trivial;

namespace aegir {
  namespace logging {

    void init();
    void setSeverity(blt::severity_level _level);
  }
}

#endif
