/*
 * misc functions
 */

#ifndef AEGIR_MISC_H
#define AEGIR_MISC_H

#include <string>
#include <memory>

#include <boost/log/trivial.hpp>

namespace blt = ::boost::log::trivial;

namespace aegir {

  // concepts
  // dervied class
  template<class T, class U>
  concept Derived = std::is_base_of<U, T>::value;

  // helper functions
  blt::severity_level parseLoglevel(const std::string& _str);
  std::string toStr(blt::severity_level _level);
  bool floateq(float _a, float _b, float _error=0.000001f);
}

#endif
