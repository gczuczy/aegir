/*
  the main loop is a threadmanager
 */

#ifndef AEGIR_FERMD_MAINLOOP
#define AEGIR_FERMD_MAINLOOP

#include <memory>

#include "common/ThreadManager.hh"
#include "common/ConfigBase.hh"
#include "common/ServiceManager.hh"

namespace aegir {
  namespace fermd {

    class MainLoop: public aegir::ThreadManager,
		    public aegir::ConfigNode {

      friend class aegir::ServiceManager;
    protected:
      MainLoop();
    public:
      MainLoop(const MainLoop&)=delete;
      MainLoop(MainLoop&&)=delete;
      virtual ~MainLoop();

    public:
      // ConfigNode
      virtual void marshall(ryml::NodeRef&);
      virtual void unmarshall(ryml::ConstNodeRef&);

    protected:
      virtual void init();
    }; // class MainLoop
  } // namespace fermd
} // namespace aegir

#endif
