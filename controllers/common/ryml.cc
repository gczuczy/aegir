
#include "common/ryml.hh"

namespace aegir {
  struct RYAMLErrorHandler {
    void on_error(const char* msg, size_t len, ryml::Location loc) {
      throw std::runtime_error(ryml::formatrs<std::string>("{}:{}:{} ({}B): ERROR: {}",
							   loc.name, loc.line, loc.col, loc.offset, ryml::csubstr(msg, len)));
    };

    ryml::Callbacks callbacks() {
      return ryml::Callbacks(this, nullptr, nullptr, RYAMLErrorHandler::s_error);
    };

    static void s_error(const char* msg, size_t len,
			ryml::Location loc, void *this_) {
      return ((RYAMLErrorHandler*)this_)->on_error(msg, len, loc);
    }

    RYAMLErrorHandler(): defaults(ryml::get_callbacks()) {};
    ryml::Callbacks defaults;
  };
  static RYAMLErrorHandler errh;

  void init_ryml() {
    ryml::set_callbacks(errh.callbacks());
  }
}
