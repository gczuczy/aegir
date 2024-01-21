/*
  ThreadPool
 */

#include "Bluetooth.hh"

#include <thread>
#include <chrono>

#include <catch2/catch_test_macros.hpp>

void runBluetooth() {
  auto bt = aegir::fermd::Bluetooth::getInstance();

  bt->worker();
}

TEST_CASE("Bluetooth", "[fermd]") {
  auto bt = aegir::fermd::Bluetooth::getInstance();

  bt->init();

  std::thread btt(runBluetooth);

  std::chrono::seconds s(2);
  std::this_thread::sleep_for(s);

  // shut it down
  bt->stop();
  btt.join();
}
