/*
  Bluetooth subsystem for fermd
 */

#ifndef AEGIR_FERMD_BLUETOOTH
#define AEGIR_FERMD_BLUETOOTH

#include <string>
#include <set>
#include <cstdint>

#define L2CAP_SOCKET_CHECKED
#include <bluetooth.h>

#include "common/ConfigBase.hh"
#include "common/ThreadManager.hh"
#include "common/LogChannel.hh"
#include "common/Exception.hh"

namespace aegir {
  namespace fermd {

    class Bluetooth: public ConfigNode,
		     public ThreadManager::Thread{
    private:
      class LE {
      public:
	LE()=delete;
	LE(const LE&)=delete;
	LE(LE&&)=delete;
	LE(const std::string& _device);
	~LE();

      public:
	inline int sock() const { return c_sock; };

      private:
	template<typename A, typename B>
	void btDeviceRequest(std::uint64_t _opgroup,
			     std::uint64_t _opcode,
			     A& _req, B& _resp,
			     time_t to = 1) {
	  struct bt_devreq dr;
	  dr.opcode = NG_HCI_OPCODE(_opgroup,
				    _opcode);

	  dr.cparam = (void*)&_req;
	  dr.clen = sizeof(_req);
	  dr.rparam = (void*)&_resp;
	  dr.rlen = sizeof(_resp);

	  if ( bt_devreq(c_sock, &dr, to)<0 ) {
	    c_logger.error("bt_devreq failed: %s", strerror(errno));
	    throw Exception("bt_devreq failed: %s", strerror(errno));
	  }
	}
	template<typename B>
	void btDeviceRequest(std::uint64_t _opgroup,
			     std::uint64_t _opcode,
			     B& _resp,
			     time_t to = 1) {
	  struct bt_devreq dr;
	  dr.opcode = NG_HCI_OPCODE(_opgroup,
				    _opcode);

	  dr.cparam = (void*)0;
	  dr.clen = 0;
	  dr.rparam = (void*)&_resp;
	  dr.rlen = sizeof(_resp);

	  if ( bt_devreq(c_sock, &dr, to)<0 ) {
	    c_logger.error("bt_devreq failed: %s", strerror(errno));
	    throw Exception("bt_devreq failed: %s", strerror(errno));
	  }
	}
	void setmask(std::uint64_t _opgroup, std::uint64_t _opcode, std::uint64_t _mask);
	uint32_t readBufferSize();
	void readSupprtedFeatures();
	void getBDAddress(bdaddr_t &_addr);
	void initNode();
	void setEventFilter(std::uint64_t _filter);

      private:
	std::string c_device;
	int c_sock;
	LogChannel c_logger;
      };

    private:
      Bluetooth();
    public:
      Bluetooth(const Bluetooth&)=delete;
      Bluetooth(Bluetooth&&)=delete;
      virtual ~Bluetooth();

      static std::shared_ptr<Bluetooth> getInstance();

      // confignode
      virtual void marshall(ryml::NodeRef&);
      virtual void unmarshall(ryml::ConstNodeRef&);

      // Thread
      virtual void init();
      virtual void worker();

    private:
      void getDeviceList();

    private:
      std::string c_device, c_device_selected;
      LogChannel c_logger;
      std::set<std::string> c_devlist;
    };
  }
}


#endif
