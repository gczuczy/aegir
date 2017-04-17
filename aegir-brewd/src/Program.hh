/*
 * The definition of the program to brew
 */

#ifndef AEGIR_PROGRAM_H
#define AEGIR_PROGRAM_H

#include <cstdint>
#include <string>
#include <vector>
#include <list>

namespace aegir {

  class Program {
  public:
    struct MashStep {
      uint32_t orderno;
      float temp;
      uint16_t holdtime;
    };
    typedef std::vector<MashStep> MashSteps;
    struct Hop {
      uint32_t id; // for the api/ui, unused here
      int attime;
    };
    typedef std::list<Hop> Hops;

  public:
    Program() = delete;
    Program(Program&&) = delete;
    Program &operator=(Program&&) = delete;

    Program(float _starttemp, float _endtemp, uint16_t boiltime, int startat, const MashSteps &_ms, const Hops _hops);
    Program(const Program &) = default;
    ~Program();
    Program &operator=(const Program &) = default;

  private:
    float c_starttemp;
    float c_endtemp;
    uint16_t c_boiltime;
    int c_startat;
    MashSteps c_mashsteps;
    Hops c_hops;
  };
}

#endif
