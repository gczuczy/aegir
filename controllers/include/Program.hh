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
      uint32_t attime;
    };
    typedef std::list<Hop> Hops;

  public:
    Program() = delete;
    Program(Program&&) = delete;
    Program &operator=(Program&&) = delete;

    Program(uint32_t _progid, float _starttemp, float _endtemp, uint16_t boiltime,
	    bool _nomash, bool _noboil,
	    const MashSteps &_ms, const Hops _hops);
    Program(const Program &) = default;
    Program &operator=(const Program &) = default;
    ~Program();

    // getters
    inline uint32_t getId() const {return c_progid;};
    inline float getStartTemp() const { return c_starttemp; };
    inline float getEndTemp() const { return c_endtemp; };
    inline uint16_t getBoilTime() const { return c_boiltime; };
    inline bool getNoMash() const { return c_nomash; };
    inline bool getNoBoil() const { return c_noboil; };
    inline const MashSteps &getMashSteps() const {return c_mashsteps; };
    inline const Hops &getHops() const { return c_hops; };

  private:
    uint32_t c_progid;
    float c_starttemp;
    float c_endtemp;
    uint16_t c_boiltime;
    bool c_nomash;
    bool c_noboil;
    MashSteps c_mashsteps;
    Hops c_hops;
  };
}

#endif
