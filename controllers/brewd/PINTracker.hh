/*
 * This class is tracking PIN Changes for the Controller
 */

#ifndef AEGIR_PINTRACKER_H
#define AEGIR_PINTRACKER_H

#include <map>
#include <set>
#include <string>
#include <cstdint>
#include <memory>
#include <cstdint>

#include "GPIO.hh"

namespace aegir {

  class PINTracker {
  public:
    // types
    // reprsents a pin and its old and new state
    class PIN {
    public:
      enum class PINType: uint8_t {
	IN=0, OUT=1
      };
    public:
      PIN() = delete;
      PIN(PIN&&) = delete;
      PIN(const PIN&) = delete;
      PIN &operator=(PIN&&) = delete;
      PIN &operator=(const PIN&) = delete;
    protected:
      PIN(const std::string &_name);
    public:
      virtual ~PIN() = 0;

      inline const std::string &getName() const { return c_name; };
      inline PINState getOldValue() const { return c_value; };
      inline PINState getNewValue() const { return c_newvalue; };
      inline float getOldCycletime() const { return c_cycletime; };
      inline float getNewCycletime() const { return c_newcycletime; };
      inline float getOldOnratio() const { return c_onratio; };
      inline float getNewOnratio() const { return c_newonratio; };
      inline bool isChanged() const { return (c_value != c_newvalue ||
					      ((c_newvalue == PINState::Pulsate) &&
					       (c_cycletime != c_newcycletime || c_onratio != c_newonratio))); };
      virtual PINState getValue();
      virtual float getCycletime();
      virtual float getOnratio();
      virtual void setValue(PINState _v, float _cycletime = 3.0f, float _onratio=0.4f) = 0;
      PINType getType() const {return c_type; };

      void pushback();

    protected:
      std::string c_name;
      PINState c_value;
      PINState c_newvalue;
      float c_cycletime;
      float c_newcycletime;
      float c_onratio;
      float c_newonratio;
      PINType c_type;
    }; // PIN
    typedef std::map<std::string, std::shared_ptr<PIN> > PINMap;
    typedef std::set<PIN*> PINChanges;

    // Out Pins
    // new value goes into a queue
    class OutPIN: public PIN {
    public:
      OutPIN() = delete;
      OutPIN(OutPIN&&) = delete;
      OutPIN(const OutPIN *) = delete;
      OutPIN &operator=(OutPIN &&) = delete;
      OutPIN &operator=(const OutPIN &) = delete;
      OutPIN(const std::string &_name, PINChanges &_pcq);
      virtual ~OutPIN();

      virtual PINState getValue() override;
      virtual void setValue(PINState _v, float _cycletime = 3.0f, float _onratio=0.4f) override;

    private:
      PINChanges &c_pcq;
    }; // OutPIN

    // In Pins
    // new value goes into a queue
    class InPIN: public PIN {
    public:
      InPIN() = delete;
      InPIN(InPIN&&) = delete;
      InPIN(const InPIN *) = delete;
      InPIN &operator=(InPIN &&) = delete;
      InPIN &operator=(const InPIN &) = delete;
      InPIN(const std::string &_name, PINChanges &_inch);
      virtual ~InPIN();

      virtual PINState getValue() override;
      virtual void setValue(PINState _v, float _cycletime = 3.0f, float _onratio=0.4f) override;

    private:
      PINChanges &c_inch; // in changes
    }; // InPIN

  public:
    PINTracker(PINTracker&&) = delete;
    PINTracker(const PINTracker&) = delete;
    PINTracker &operator=(PINTracker&&) = delete;
    PINTracker &operator=(const PINTracker&) = delete;
    PINTracker();
    virtual ~PINTracker();

    void startCycle();
    void endCycle();

    std::shared_ptr<PIN> getPIN(const std::string &_name);
    void setPIN(const std::string &_name, PINState _value, float _cycletime=2.0f, float _onratio=0.4f);
    bool hasChanged(const std::string &_name);
    bool hasChanged(const std::string &_name, std::shared_ptr<PIN> &_pin);
    inline bool hasChanges() const { return !!c_inpinchanges.size(); };

  protected:
    void reconfigure();
    virtual void handleOutPIN(PIN &) = 0;

  private:
    PINMap c_pins;
    PINChanges c_inpinchanges;
    PINChanges c_pinchangequeue;
  };
}

#endif
