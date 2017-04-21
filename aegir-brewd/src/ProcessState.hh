/*
 * This class holds the current process state
 * - Stores the program
 * - Holds sensor state information
 * - Holds sensor statistics info
 */

#ifndef AEGIR_PROCESSSTATE_H
#define AEGIR_PROCESSSTATE_H

#include <memory>
#include <atomic>
#include <cstdint>
#include <mutex>

#include "Program.hh"

namespace aegir {

  class ProcessState {
  private:
    ProcessState();
    ProcessState(ProcessState&&) = delete;
    ProcessState(const ProcessState &) = delete;
    ProcessState &operator=(ProcessState&&) = delete;
    ProcessState &operator=(const ProcessState &) = delete;
  public:
    ~ProcessState();
    static ProcessState &getInstance();

    bool isActive();
    ProcessState &loadProgram(const Program &_prog, uint32_t _startat, uint32_t _volume);
    std::shared_ptr<Program> getProgram();

  private:
    // inputs
    std::shared_ptr<Program> c_program;
    uint32_t c_startat;
    uint32_t c_volume; //liters
    // states
    std::recursive_mutex c_mtx_state;
  };
}

#endif
