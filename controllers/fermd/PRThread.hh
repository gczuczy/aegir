
#ifndef AEGIR_FERMD_PRTHREAD
#define AEGIR_FERMD_PRTHREAD

#include <functional>
#include <map>
#include <string>

#include "common/ThreadManager.hh"
#include "common/ZMQ.hh"
#include "common/ryml.hh"
#include "common/LogChannel.hh"
#include "common/ServiceManager.hh"
#include "DBTypes.hh"

#define PRCMD(NAME) void handle_##NAME(ryml::ConstNodeRef&, ryml::NodeRef&)

namespace aegir {
  namespace fermd {
    typedef std::function<void(ryml::ConstNodeRef&, ryml::NodeRef&)> handler_func;

    class PRThread: public ThreadPool,
		    public Service,
		    private LogChannel {
      friend class aegir::ServiceManager;
    protected:
      PRThread();

    public:
      virtual ~PRThread();

      virtual void init();
      virtual void stop();
      virtual void bailout();
      virtual void controller();
      virtual void worker(std::atomic<bool>& _run);
      virtual uint32_t maxWorkers() const;

    private:
      // helpers
      void setFermenterType(ryml::NodeRef& _node,
			    DB::fermenter_types::cptr _ft);
      // handlers
      PRCMD(getFermenterTypes);
      PRCMD(getFermenters);
      PRCMD(getTilthydrometers);

    private:
      zmqproxy_type c_proxy;
      std::map<std::string, handler_func> c_handlers;
    };
  }
}
#undef PRCMD

#endif
