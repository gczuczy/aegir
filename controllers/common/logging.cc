/*
  Logging subsystem basics
 */

#include "logging.hh"

#include <map>

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

#include "Exception.hh"

namespace bl = ::boost::log;
namespace blt = ::boost::log::trivial;
namespace bla = ::boost::log::attributes;
namespace bls = ::boost::log::sinks;
namespace blk = ::boost::log::keywords;
namespace ble = ::boost::log::expressions;

namespace aegir {
  namespace logging {

    typedef bls::synchronous_sink<bls::syslog_backend> syslog_sink_t;

    static bool filter(const boost::log::attribute_value_set&);
    std::function<blt::severity_level()> g_getloglevel;

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

	auto frontend = boost::make_shared<syslog_sink_t>(backend);

	frontend->set_formatter(
				ble::stream
				<< ble::attr<std::string>("Channel")
				<< ":"
				<< ble::attr<blt::severity_level>("Severity")
				<< " "
				<< ble::smessage
				);

	blcore->add_sink(frontend);
      }
    }

    std::string str(blt::severity_level _level) {
      static std::map<blt::severity_level, std::string> levelmap{
	{blt::severity_level::trace, "trace"},
	{blt::severity_level::debug, "debug"},
	{blt::severity_level::info, "info"},
	{blt::severity_level::warning, "warning"},
	{blt::severity_level::error, "error"},
	{blt::severity_level::fatal, "fatal"},
      };

      auto it = levelmap.find(_level);
      if ( it == levelmap.end() )
	throw Exception("Unknown severity");

      return it->second;
    }

    void setGetLogLevel(std::function<blt::severity_level()> _getloglevel) {
      g_getloglevel = _getloglevel;
    }

    bool filter(const boost::log::attribute_value_set& attr_set) {
      if ( g_getloglevel ) {
	return attr_set["Severity"].extract<blt::severity_level>()
	  >= g_getloglevel();
      }
      return true;
    }
  } // ns lggoging
} // ns aegir
