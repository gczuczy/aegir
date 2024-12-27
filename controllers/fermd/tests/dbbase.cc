
#include "fermd/tests/dbbase.hh"

#include "common/Message.hh"

DBTestSM::DBTestSM() {
  add<aegir::MessageFactory>();
  add<aegir::fermd::DB::Connection>();
}

DBTestSM::~DBTestSM() {
}

DBFixture::DBFixture() {
  c_db = c_sm.get<aegir::fermd::DB::Connection>();
  c_db->setConnectionFile(c_fg.getFilename());
  c_db->init();
}

DBFixture::~DBFixture() {
}
