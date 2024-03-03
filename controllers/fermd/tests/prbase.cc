
#include "fermd/tests/prbase.hh"


void runPRManager() {
  auto tm = aegir::ServiceManager::get<PRManager>();

  tm->run();
}

PRManager::PRManager(): aegir::ThreadManager() {
  registerHandler<aegir::fermd::PRThread>("PR");
}

PRManager::~PRManager() {
}

PRTestSM::PRTestSM() {
  add<aegir::fermd::ZMQConfig>();
  add<aegir::fermd::PRThread>();
  add<aegir::fermd::DB::Connection>();
  add<PRManager>();
}

PRTestSM::~PRTestSM() {
}

PRFixture::PRFixture() {
  auto db = c_sm.get<aegir::fermd::DB::Connection>();
  db->setConnectionFile(c_fg.getFilename());
  db->init();

  c_manager = std::thread(runPRManager);

  c_sock = c_sm.get<aegir::fermd::ZMQConfig>()
    ->srcSocket("prpublic");
  c_sock->brrr();

  auto tm = c_sm.get<PRManager>();
  std::chrono::milliseconds s(10);
  while ( !tm->isRunning() )
    std::this_thread::sleep_for(s);
}

PRFixture::~PRFixture() {
  c_sm.get<PRManager>()->stop();
  c_manager.join();
}

aegir::RawMessage::ptr PRFixture::send(const std::string& _cmd) {
  INFO("Request: " << _cmd);
  c_sock->send(_cmd, true);
  auto msg = c_sock->recvRaw(true);
  INFO("Response: " << ((char*)msg->data()));
  return msg;
}

bool PRFixture::isError(aegir::RawMessage::ptr& _msg) {
  auto indata = c4::to_csubstr((char*)_msg->data());
  ryml::Tree tree = ryml::parse_in_arena(indata);
  ryml::NodeRef root = tree.rootref();
  return root["status"] == "error";
}
