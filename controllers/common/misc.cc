
#include "misc.hh"

#include <map>
#include <cmath>

#include "Exception.hh"

namespace aegir {
  static std::map<std::string, blt::severity_level> g_levelmap{
    {"trace", blt::severity_level::trace},
    {"debug", blt::severity_level::debug},
    {"info", blt::severity_level::info},
    {"warn", blt::severity_level::warning},
    {"warning", blt::severity_level::warning},
    {"error", blt::severity_level::error},
    {"fatal", blt::severity_level::fatal},
  };

  blt::severity_level parseLoglevel(const std::string& _str) {
    auto it = g_levelmap.find(_str);
    if ( it == g_levelmap.end() )
      throw Exception("Unable to parse %s as loglevel", _str.c_str());

    return it->second;
  }

  std::string toStr(blt::severity_level _level) {
    for (auto it: g_levelmap) {
      if ( it.second == _level ) return it.first;
    }
    throw Exception("Severity level not found");
  }

  bool floateq(float _a, float _b, float _error) {
    return std::fabs(_a - _b) < _error;
  }
}
