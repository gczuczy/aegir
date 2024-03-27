
#include "MainLoop.hh"
#include "common/Exception.hh"
#include "Bluetooth.hh"
#include "Message.hh"
#include "SensorProxy.hh"
#include "DBConnection.hh"
#include "PRThread.hh"
#include "common/ServiceManager.hh"
#include "SensorCache.hh"

namespace aegir {
  namespace fermd {

    MainLoop::MainLoop(): ThreadManager(), ConfigNode() {
      registerHandler<Bluetooth>("bluetooth");
      registerHandler<SensorProxy>("sensorproxy");
      registerHandler<PRThread>("PR");
      registerHandler<SensorCache>("SensorCache");

      auto msf = aegir::ServiceManager::get<MessageFactory>();

      msf->registerHandler<TiltReadingMessage>();
    }

    MainLoop::~MainLoop() {
    }

    void MainLoop::marshall(ryml::NodeRef& _node) {
      _node |= ryml::MAP;
      _node["metricsamples"] << c_metrics_samples;
      _node["scaledown"] << ryml::fmt::real(c_scale_down, 3);
      _node["scaleup"] << ryml::fmt::real(c_scale_down, 3);;

      // currently changing this on-the-fly is not supported
      // so we're not updating the circular buffers.
      // also, this should only run during startup, before
      // any circular buffers are allocated
    }

    void MainLoop::unmarshall(ryml::ConstNodeRef& _node) {
      if ( !_node.is_map() )
	throw Exception("MainLoop node is not a map");

      if ( _node.has_child("metricssamples") )
	_node["metricssamples"] >> c_metrics_samples;
      if ( _node.has_child("scaledown") )
	_node["scaledown"] >> c_scale_down;
      if ( _node.has_child("scaleup") )
	_node["scaleup"] >> c_scale_up;
    }

    void MainLoop::init() {
      ServiceManager::get<DB::Connection>()->init();
    }
  }
}