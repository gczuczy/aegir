/*
 * misc functions
 */

#ifndef AEGIR_MISC_H
#define AEGIR_MISC_H

#include <string>

#include <boost/log/trivial.hpp>

namespace blt = ::boost::log::trivial;

namespace aegir {
  blt::severity_level parseLoglevel(const std::string& _str);
  std::string toStr(blt::severity_level _level);
}

#endif
