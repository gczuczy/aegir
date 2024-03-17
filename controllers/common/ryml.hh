
#ifndef AEGIR_RYAML_H
#define AEGIR_RYAML_H

#include "cmakeconfig.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfortify-source"

#if defined(RYAML_SINGLE_HEADER)
// single header
#define RYML_SINGLE_HDR_DEFINE_NOW
#include <ryml_all.hpp>
#elif defined(RYAML_SINGLE_HEADER_LIB)
// single header as a lib
#include <ryml_all.hpp>
#else
// regular lib
#include <ryml.hpp>
#include <ryml_std.hpp>
#include <c4/format.hpp>
#endif

#include <ryml_std.hpp>
#include <c4/format.hpp>

#pragma clang diagnostic pop

namespace aegir {
  void init_ryml();
}

#endif
