/*
 * misc functions
 */

#ifndef AEGIR_MISC_H
#define AEGIR_MISC_H

#include <stdio.h>
#include <string.h>

#include <string>
#include <memory>

#include <boost/log/trivial.hpp>

namespace blt = ::boost::log::trivial;

namespace aegir {

  // concepts
  // dervied class
  template<class T, class U>
  concept Derived = std::is_base_of<U, T>::value;

  // helper functions
  blt::severity_level parseLoglevel(const std::string& _str);
  std::string toStr(blt::severity_level _level);
  bool floateq(float _a, float _b, float _error=0.000001f);

  // zero memory area
  template<typename T>
  void zero(T* _dst, size_t _len = sizeof(T)) {
    memset((void*)_dst, 0, _len);
  }

  // hexdumping
  template<typename T>
  void hexdump(T* _data, std::uint32_t _len=sizeof(T)) {
    char *data = (char*)_data;

    printf("::hexdump(%p, %u)\n", (void*)_data, _len);
    for (auto i=0; i<_len; ++i) {
      if ( i%16 == 0 ) printf("%08x  ", i);
      printf("%02x ", data[i]);

      if ( i>0 && i%8 == 0 && i%16 != 0 ) {
	printf(" ");
	continue;
      }

      if ( (i>0 && i%16==15) || i == _len-1 ) {
	printf("  |");
	for (auto j=i>=16?i-16:0; j<i; ++j) {
	  if ( data[j]>=33 && data[j]<=126 ) printf("%c", data[j]);
	  else printf(".");
	  if ( j==8 ) printf(" ");
	}
	printf("|\n");
      }
    }
    putchar('\n');
  }
}

#endif
