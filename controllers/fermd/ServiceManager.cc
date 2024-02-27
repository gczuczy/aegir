
#include "ServiceManager.hh"

#include "FermdConfig.hh"
#include "MainLoop.hh"
#include "common/Message.hh"
#include "DBConnection.hh"
#include "PRThread.hh"
#include "SensorProxy.hh"
#include "ZMQConfig.hh"
#include "Bluetooth.hh"

namespace aegir {
  namespace fermd {

    ServiceManager::ServiceManager(): aegir::ServiceManager() {
      add<MessageFactory>();
      add<ZMQConfig>();
      add<SensorProxy>();
      add<PRThread>();
      add<Bluetooth>();
      add<MainLoop>();
      add<DB::Connection>();

      add<aegir::fermd::FermdConfig>();
    }

    ServiceManager::~ServiceManager() {
    }
  }
}
