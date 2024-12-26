
#include "common/tests/common.hh"

#include <stdio.h>

#include <filesystem>

FileGuard::FileGuard() {
  char *x;
  x = tempnam(0, "tests.fermd.");
  if ( x ) {
    c_filename = std::string(x);
    free(x);
  } else {
    throw std::runtime_error("Unable to allocate tempfile name");
  }
}

FileGuard::~FileGuard() {
  std::filesystem::remove(c_filename);
}
