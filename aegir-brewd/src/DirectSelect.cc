
#include "DirectSelect.hh"
#include "Exception.hh"

namespace aegir {

  DirectSelect::DirectSelect(GPIO &_gpio, const std::map<int, std::string> &_chips): c_gpio(_gpio), c_chips(_chips) {
    for (auto &it: c_chips) {
      c_gpio[it.second].high();
    }
  }

  void DirectSelect::high(int _id) {
    auto it = c_chips.find(_id);
    if ( it == c_chips.end() )
      throw Exception("DirectSelect: id not found: %i", _id);

    c_gpio[it->second].high();
  }

  void DirectSelect::low(int _id) {
    auto it = c_chips.find(_id);
    if ( it == c_chips.end() )
      throw Exception("DirectSelect: id not found: %i", _id);

    c_gpio[it->second].low();
  }

  DirectSelect::~DirectSelect() {
    for (auto &it: c_chips) {
      c_gpio[it.second].low();
    }
  }

}
