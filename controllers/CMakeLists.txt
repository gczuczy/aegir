cmake_minimum_required(VERSION 3.26)

set(default_build_type "Release")
set(CMAKE_CXX_STANDARD 20)
set(Boost_NO_WARN_NEW_VERSIONS 1)
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(Boost_NO_BOOST_CMAKE OFF)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

project(brewd VERSION 0.1.0 LANGUAGES CXX)

if ( (NOT DEFINED CMAKE_BUILD_TYPE) OR CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Debug")
endif()

include(FetchContent)
include(CTestCoverageCollectGCOV)

add_executable(brewd)
add_executable(fermd)

find_package(Catch2 3 REQUIRED)
add_executable(tests)
add_executable(tests_fermd)
include(CTest)
include(Catch)
catch_discover_tests(tests)
catch_discover_tests(tests_fermd)

add_subdirectory(brewd)
add_subdirectory(fermd)
add_subdirectory(common)
add_subdirectory(tests)

# coverage
FetchContent_Declare(cmake_modules
  GIT_REPOSITORY https://github.com/bilke/cmake-modules.git
  GIT_TAG "master"
  GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(cmake_modules)
list(APPEND CMAKE_MODULE_PATH ${cmake_modules_SOURCE_DIR})
set(CODE_COVERAGE_VERBOSE TRUE)
include(CodeCoverage)
append_coverage_compiler_flags_to_target(tests_fermd)
message("Project binary dir: ${PROJECT_BINARY_DIR}")

set_target_properties(
  brewd
  PROPERTIES
  OUTPUT_NAME "aegir-brewd"
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)

set_target_properties(
  tests
  PROPERTIES
  OUTPUT_NAME "tests"
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)

set_target_properties(
  tests_fermd
  PROPERTIES
  OUTPUT_NAME "tests-fermd"
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)

configure_file(common/cmakeconfig.hh.in common/cmakeconfig.hh)

target_include_directories(brewd
  SYSTEM PRIVATE /usr/local/include
)
target_include_directories(brewd
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/brewd
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_include_directories(tests
  SYSTEM PRIVATE /usr/local/include
)
target_include_directories(tests
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/brewd
  ${CMAKE_CURRENT_SOURCE_DIR}/fermd
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/common>
)

target_include_directories(tests_fermd
  SYSTEM PRIVATE /usr/local/include
)
target_include_directories(tests_fermd
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/fermd/
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/>
)

include(CheckSymbolExists)
include(CheckCXXSymbolExists)
include(CheckIPOSupported)

# threads
find_package(Threads)
if(NOT Threads_FOUND)
  message(FATAL_ERROR "threading is required")
endif()
message(STATUS "Thread library to use: ${CMAKE_THREAD_LIBS_INIT}")
message(STATUS "Thread library pthread compatible: ${CMAKE_USE_PTHREAD}")

# boost
find_package(Boost
  1.83.0
  REQUIRED
  CONFIG
  COMPONENTS program_options log
)
if(Boost_FOUND)
  message(STATUS "Boost found")
else()
  message(FATAL_ERROR "Boost program options is required")
endif()

# yaml-cpp
find_package(yaml-cpp
  0.8.0
  REQUIRED
)

# SQLite3
find_package(SQLite3 REQUIRED)

# rapidyaml - fermd uses it instead of yaml-cpp and jsoncpp
#find_package(ryml REQUIRED)
#if(ryml_FOUND)
#  message(STATUS "Rapid YAML found")
#else()
#  message(FATAL_ERROR "Rapid YAML is required but not found")
#endif()
FetchContent_Declare(ryml
  GIT_REPOSITORY https://github.com/biojppm/rapidyaml.git
  GIT_TAG "v0.5.0"
  GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(ryml)

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
  set_property(TARGET tests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  set_property(TARGET tests_fermd PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
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
  Threads::Threads
)

target_link_libraries(brewd
  PRIVATE
  Boost::program_options
  Boost::log
  gpio
  yaml-cpp::yaml-cpp
  zmq
  jsoncpp
)

target_link_options(tests
  PRIVATE
  -L/usr/local/lib
  -L${Boost_LIBRARY_DIRS}
  Threads::Threads
)

target_link_libraries(tests
  PRIVATE
  Catch2::Catch2WithMain
  Boost::log
  yaml-cpp::yaml-cpp
  zmq
  jsoncpp
  ryml::ryml
)

target_compile_options(tests_fermd
  PRIVATE
  -fprofile-arcs -ftest-coverage
)

target_link_options(tests_fermd
  PRIVATE
  -L/usr/local/lib
  -L${Boost_LIBRARY_DIRS}
  Threads::Threads
  --coverage
)

target_link_libraries(tests_fermd
  PRIVATE
  Catch2::Catch2WithMain
  Boost::log
  yaml-cpp::yaml-cpp
  zmq
  jsoncpp
  ryml::ryml
  bluetooth
  SQLite::SQLite3
)


set_property(TARGET brewd PROPERTY CXX_STANDARD 20)
set_property(TARGET brewd PROPERTY CXX_STANDARD_REQUIRED True)
set_property(TARGET brewd PROPERTY CXX_EXTENSIONS False)
target_compile_features(brewd PRIVATE cxx_std_20)

set_property(TARGET tests PROPERTY CXX_STANDARD 20)
set_property(TARGET tests PROPERTY CXX_STANDARD_REQUIRED True)
set_property(TARGET tests PROPERTY CXX_EXTENSIONS False)
target_compile_features(tests PRIVATE cxx_std_20)

set_property(TARGET tests_fermd PROPERTY CXX_STANDARD 20)
set_property(TARGET tests_fermd PROPERTY CXX_STANDARD_REQUIRED True)
set_property(TARGET tests_fermd PROPERTY CXX_EXTENSIONS False)
target_compile_features(tests_fermd PRIVATE cxx_std_20)

# and finally enable testing
enable_testing()
setup_target_for_coverage_lcov(
  NAME fermd_coverage
  EXECUTABLE bin/tests-fermd
  DEPENDENCIES tests_fermd
  EXCLUDE "/usr/include/*" "/usr/local/include/*" "${ryml_SOURCE_DIR}/*"
)


message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")

# install locations
install(FILES rc.d/aegir-brewd
  DESTINATION etc/rc.d
  PERMISSIONS
  OWNER_EXECUTE OWNER_WRITE OWNER_READ
  GROUP_EXECUTE GROUP_READ
  WORLD_EXECUTE WORLD_READ
  CONFIGURATIONS Release
)

install(TARGETS brewd
  DESTINATION sbin
  PERMISSIONS
  OWNER_EXECUTE OWNER_WRITE OWNER_READ
  GROUP_EXECUTE GROUP_READ
  WORLD_EXECUTE WORLD_READ
  CONFIGURATIONS Release
)

# Fermd
target_include_directories(fermd
  SYSTEM PRIVATE /usr/local/include
)
target_include_directories(fermd
  PRIVATE ${fermd_SOURCE_DIR}/fermd
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_compile_options(fermd
  PRIVATE -Werror -pedantic -std=c++20 -Wno-c11-extensions -I${Boost_INCLUDE_DIRS}
  $<$<CONFIG:Debug>:-O0 -glldb -ferror-limit=2>
  $<$<CONFIG:Release>:-O2>
)

target_link_options(fermd
  PRIVATE
  -L/usr/local/lib
  -L${Boost_LIBRARY_DIRS}
  Threads::Threads
)

target_link_libraries(fermd
  PRIVATE
  Boost::program_options
  Boost::log
  gpio
  zmq
  ryml::ryml
  bluetooth
  SQLite::SQLite3
)

install(TARGETS fermd
  DESTINATION sbin
  PERMISSIONS
  OWNER_EXECUTE OWNER_WRITE OWNER_READ
  GROUP_EXECUTE GROUP_READ
  WORLD_EXECUTE WORLD_READ
  CONFIGURATIONS Release
)

set_target_properties(
  fermd
  PROPERTIES
  OUTPUT_NAME "aegir-fermd"
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
