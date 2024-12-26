
#include "fermd/tests/prbase.hh"

TEST_CASE_METHOD(PRFixture, "pr_badcmd", "[fermd][pr]") {
  auto msg = send("{\"command\": \"bullShit\"}");
  REQUIRE( msg );
  REQUIRE( isError(msg) );
}

TEST_CASE_METHOD(PRFixture, "pr_randomdata", "[fermd][pr]") {
  SKIP("ryml just faints");
  std::string output("foo");
  output.resize(32);

  for (int i=0; i<32; i += sizeof(int) ) {
    int rnd = rand();
    memcpy((void*)(output.data()+i), &rnd, sizeof(int));
  }

  auto msg = send(output);

  REQUIRE( msg );
  REQUIRE( isError(msg) );
}
