
#include "ElapsedTime.hh"

#include <stdio.h>

namespace aegir {

  ElapsedTime::ElapsedTime(std::string _message): c_message(_message) {
    printf("ElapsedTime start: %s\n", c_message.c_str());
    c_start = std::chrono::high_resolution_clock::now();
  }

  ElapsedTime::~ElapsedTime() {
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end-c_start;
    printf("ElapsedTime end: %s %.4f ms\n", c_message.c_str(), diff.count()*1000);
  }
}
