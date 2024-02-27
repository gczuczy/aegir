/*
  ThreadPool
 */

#include "Bluetooth.hh"

#include <thread>
#include <chrono>

#include "common/ServiceManager.hh"

#include <catch2/catch_test_macros.hpp>

class BluetoothTestSM: public aegir::ServiceManager {
public:
  BluetoothTestSM() {
    add<aegir::fermd::ZMQConfig>();
    add<aegir::fermd::Bluetooth>();
  };
  virtual ~BluetoothTestSM() {};
};

void runBluetooth() {
  auto bt = aegir::ServiceManager::get<aegir::fermd::Bluetooth>();

  bt->worker();
}

TEST_CASE("Bluetooth", "[fermd]") {
  BluetoothTestSM sm;
  auto bt = sm.get<aegir::fermd::Bluetooth>();

  bt->init();

  std::thread btt(runBluetooth);

  std::chrono::seconds s(2);
  std::this_thread::sleep_for(s);

  // shut it down
  bt->stop();
  btt.join();
}
