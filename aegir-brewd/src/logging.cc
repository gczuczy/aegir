/*
  Logging subsystem basics
 */

#include "logging.hh"

#define BOOST_LOG_USE_NATIVE_SYSLOG

#include <boost/log/core.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/sinks/syslog_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>

#ifdef BOOST_LOG_WITHOUT_SYSLOG
#error "BOOST_LOG_WITHOUT_SYSLOG is set"
#endif

#ifndef BOOST_LOG_USE_NATIVE_SYSLOG
#error "BOOST_LOG_USE_NATIVE_SYSLOG is unset"
#endif

namespace bl = ::boost::log;
namespace blt = ::boost::log::trivial;
namespace bla = ::boost::log::attributes;
namespace bls = ::boost::log::sinks;
namespace blk = ::boost::log::keywords;
//namespace blf = boost::log::filters;

namespace aegir {
  namespace logging {

    typedef bls::synchronous_sink<bls::syslog_backend> syslog_sink_t;

    static bool filter(const boost::log::attribute_value_set&);

    static blt::severity_level g_severity = blt::info;

    void init() {
      auto blcore = bl::core::get();
      blcore->add_global_attribute("ThreadID",
				   bla::current_thread_id()
				   );
      blcore->set_filter(&filter);

      // init syslog
      {
	boost::shared_ptr<bls::syslog_backend> backend(new bls::syslog_backend(
									       blk::facility = bls::syslog::daemon,
									       blk::use_impl = bls::syslog::native,
									       blk::ident = "aegir"
));
	backend->set_severity_mapper(bls::syslog::direct_severity_mapping<int>("Severity"));

	blcore->add_sink(boost::make_shared<syslog_sink_t>(backend));
      }
    }

    void setSeverity(blt::severity_level _level) {
      g_severity = _level;
    }

    bool filter(const boost::log::attribute_value_set& attr_set) {
      return attr_set["Severity"].extract<blt::severity_level>() >= g_severity;
    }
  } // ns lggoging
} // ns aegir
