/*
  Logging subsystem basics
 */

#include "logging.hh"

#include <boost/log/core.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/current_thread_id.hpp>

namespace bl = ::boost::log;
namespace blt = ::boost::log::trivial;
namespace bla = ::boost::log::attributes;
//namespace blf = boost::log::filters;

namespace aegir {
  namespace logging {

    static bool filter(const boost::log::attribute_value_set&);

    static blt::severity_level g_severity = blt::info;

    void init() {
      auto blcore = bl::core::get();
      blcore->add_global_attribute("ThreadID",
				   bla::current_thread_id()
				   );
      blcore->set_filter(&filter);
    }

    void setSeverity(blt::severity_level _level) {
      g_severity = _level;
    }

    bool filter(const boost::log::attribute_value_set& attr_set) {
      return attr_set["Severity"].extract<blt::severity_level>() >= g_severity;
    }
  } // ns lggoging
} // ns aegir
