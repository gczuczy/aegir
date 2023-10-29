cmake_minimum_required(VERSION 3.26)

set(default_build_type "Release")
set(CMAKE_CXX_STANDARD 20)
set(Boost_NO_WARN_NEW_VERSIONS 1)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")


project(brewd VERSION 0.1.0 LANGUAGES CXX)

add_executable(brewd)

find_package(Catch2 3 REQUIRED)
add_executable(tests)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
include(CTest)
include(Catch)
catch_discover_tests(tests)

set_target_properties(
  brewd
  PROPERTIES
  OUTPUT_NAME "aegir-brewd"
  RUNTIME_OUTPUT_DIRECTORY ${brewd_SOURCE_DIR}/bin
)

set_target_properties(
  tests
  PROPERTIES
  OUTPUT_NAME "tests"
  RUNTIME_OUTPUT_DIRECTORY ${brewd_SOURCE_DIR}/bin
)

configure_file(include/cmakeconfig.hh.in include/cmakeconfig.hh)

target_include_directories(brewd
  PRIVATE ${brewd_SOURCE_DIR}/include
  PRIVATE ${brewd_SOURCE_DIR}/contrib/include
  SYSTEM /usr/local/include
  SYSTEM $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

target_include_directories(tests
  PRIVATE ${brewd_SOURCE_DIR}/include
  PRIVATE ${brewd_SOURCE_DIR}/contrib/include
  SYSTEM /usr/local/include
  SYSTEM $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

include(CheckSymbolExists)
include(CheckCXXSymbolExists)
include(CheckIPOSupported)

# threads
find_package(Threads)
if(NOT Threads_FOUND)
  message(FATAL_ERROR "threading is required")
endif()

# boost
find_package(Boost
  1.83.0
  REQUIRED
  COMPONENTS program_options
)
if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost program options is required")
endif()

# yaml-cpp
find_package(yaml-cpp
  0.8.0
  REQUIRED
)

# kqueue
# non-bsd users can try https://github.com/mheily/libkqueue, however this requires code support as well,
# as it is not a drop-in replacement
# mmap
check_symbol_exists(kqueue "sys/event.h" HAVE_KQUEUE)
if(NOT HAVE_KQUEUE)
  message(FATAL_ERROR "kqueue is missing")
endif()

# gpio
find_library(HAVE_GPIO
  gpio
  REQUIRED
)
if(NOT HAVE_GPIO)
  message(FATAL_ERROR "GPIO is missing")
endif()

# zero-mq
find_library(HAVE_ZMQ
  zmq
  REQUIRED
)

# IPO/LTO check
check_ipo_supported(RESULT LTO_supported OUTPUT error)
if(LTO_supported)
  message(STATUS "Enabling IPO/LTO")
  set_property(TARGET brewd PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(STATUS "IPO/LTO not supported: <${error}>")
endif()

target_compile_options(brewd
  PRIVATE -Werror -pedantic -std=c++20 -Wno-c11-extensions -I${Boost_INCLUDE_DIRS}
  $<$<CONFIG:Debug>:-O0 -glldb -ferror-limit=2>
  $<$<CONFIG:Release>:-O2>
)

target_link_options(brewd
  PRIVATE
  -L/usr/local/lib
  -L${Boost_LIBRARY_DIRS}
  -pthread
  -lboost_program_options
  -lgpio
  -lyaml-cpp
  -lzmq
  -ljsoncpp
)

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(tests)

set_property(TARGET brewd PROPERTY CXX_STANDARD 20)
set_property(TARGET brewd PROPERTY CXX_STANDARD_REQUIRED True)
set_property(TARGET brewd PROPERTY CXX_EXTENSIONS False)

target_compile_features(brewd PRIVATE cxx_std_20)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")
