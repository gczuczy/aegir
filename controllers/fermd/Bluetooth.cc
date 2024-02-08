
#include "Bluetooth.hh"

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/bitstring.h>
#include <netgraph/bluetooth/include/ng_hci.h>
#include <netgraph/bluetooth/include/ng_l2cap.h>
#include <netgraph/bluetooth/include/ng_btsocket.h>

namespace aegir {
  namespace fermd {

    /*
      Internal helper functions
    */
    static std::string bdaddr(bdaddr_t &bdaddr) {
      char buff[32];

      bt_ntoa(&bdaddr, buff);

      return std::string(buff);
    }

    static int bt_devenum_cb(int s, struct bt_devinfo const *di, void *arg) {
      std::set<std::string>* devlist = (std::set<std::string>*)arg;

      devlist->insert(di->devname);
      return 0;
    }

    /*
      LTE subclass
     */
    Bluetooth::LE::LE(const std::string& _device): c_device(_device),
						     c_logger("LE:"+_device) {

      if ( (c_sock = bt_devopen(c_device.c_str()))<0 ) {
	c_logger.fatal("Unable to open: %s", strerror(errno));
	throw Exception("Unable to open device %s:: %s",
			c_device.c_str(),
			strerror(errno));
      }

      try {
	setmask(NG_HCI_OGF_HC_BASEBAND, NG_HCI_OCF_SET_EVENT_MASK,
		NG_HCI_EVENT_MASK_LE | NG_HCI_EVENT_MASK_DEFAULT);
	setmask(NG_HCI_OGF_LE, NG_HCI_OCF_LE_SET_EVENT_MASK,
		NG_HCI_LE_EVENT_MASK_ALL);
      }
      catch (Exception& e) {
	bt_devclose(c_sock);
	throw e;
      }

      // these two calls are required for the init sequence
      // per ng_hci(4)
      try {
	readBufferSize();
      }
      catch (Exception& e) {
	c_logger.error("readBufferSize(): %s", e.what());
      }
      catch (...) {
	c_logger.error("readBufferSize(): unknown error");
      }

      try {
	readSupprtedFeatures();
      }
      catch (Exception& e) {
	c_logger.error("readSupportedFeatures(): %s", e.what());
      }
      catch (...) {
	c_logger.error("readSupportedFeatures(): unknown error");
      }

      try {
	bdaddr_t addr;
	getBDAddress(addr);
	printf("bdaddr: %s\n", bdaddr(addr).c_str());
      }
      catch (Exception& e) {
	c_logger.error("getBDAddress(): %s", e.what());
      }
      catch (...) {
	c_logger.error("getBDAddress(): unknown error");
      }

      try {
	initNode();
      }
      catch (Exception& e) {
	c_logger.error("initNode(): %s", e.what());
      }
      catch (...) {
	c_logger.error("initNode(): unknown error");
      }

      // skipping LE scan parameters, testprog got
      // failure at it
      try {
	setEventFilter(NG_HCI_EVENT_LE);
      }
      catch (Exception& e) {
	bt_devclose(c_sock);
	throw e;
      }
    }

    Bluetooth::LE::~LE() {
      bt_devclose(c_sock);
    }

    void Bluetooth::LE::setmask(std::uint64_t _opgroup,
				std::uint64_t _opcode,
				std::uint64_t _mask) {
      ng_hci_set_event_mask_cp req;
      ng_hci_set_event_mask_rp resp;

      uint64_t mask = _mask;

      // copy the new event mask
      for (int i=0; i<NG_HCI_EVENT_MASK_SIZE; ++i) {
	req.event_mask[i] = (mask>>(8*i))&0xff;
      }
      btDeviceRequest(_opgroup, _opcode,
		      req, resp, 1);

      if ( resp.status != 0 ) {
	c_logger.error("setmask status: %i", resp.status);
	throw Exception("bt_devreq failed: %i", resp.status);
      }
    }

    uint32_t Bluetooth::LE::readBufferSize() {
      ng_hci_le_read_buffer_size_rp rp;

      btDeviceRequest(NG_HCI_OGF_LE,
		      NG_HCI_OCF_LE_READ_BUFFER_SIZE,
		      rp, 1);
      if ( rp.status != 0 ) {
	throw Exception("LE read buff Status: %i\n", rp.status);
      }
      return rp.hc_le_data_packet_length;
    }

    /*
      According to ng_hci(4) this must be done to init
      the device, but we don't need anything out of this.
      So, this call just ignores everything returned.
     */
    void Bluetooth::LE::readSupprtedFeatures() {
      ng_hci_le_read_local_supported_features_rp rp;

      try {
	btDeviceRequest(NG_HCI_OGF_LE,
			NG_HCI_OCF_LE_READ_LOCAL_SUPPORTED_FEATURES,
			rp, 1);
      }
      catch (Exception& e) {
	throw Exception("Reading supported features failed: %s", e.what());
      }
      c_logger.debug("Supported features: %0lx", rp.le_features);
    }

    void Bluetooth::LE::initNode() {
      struct bt_devreq dr;
      dr.opcode = NG_HCI_OPCODE(NG_HCI_OGF_LE,
				NGM_HCI_NODE_INIT);

      dr.cparam = (void*)0;
      dr.clen = 0;
      dr.rparam = (void*)0;
      dr.rlen = 0;

      if ( bt_devreq(c_sock, &dr, 1)<0 ) {
	c_logger.error("bt_devreq failed: %s", strerror(errno));
	throw Exception("bt_devreq failed: %s", strerror(errno));
      }
    }

    void Bluetooth::LE::getBDAddress(bdaddr_t &_addr) {

      try {
	btDeviceRequest(NG_HCI_OGF_LE,
			NGM_HCI_NODE_GET_BDADDR,
			_addr, 1);
      }
      catch (Exception& e) {
	throw Exception("Reading BDAddr: %s", e.what());
      }
    }

    void Bluetooth::LE::setEventFilter(std::uint64_t _filter) {
      struct bt_devfilter flt;
      int s;

      // grab the old filter
      if ( ( s = bt_devfilter(c_sock, 0, &flt)) < 0 ) {
	c_logger.error("Unable to get device filter");
	throw Exception("Unable to get device filter");
      }

      // add LE
      bt_devfilter_evt_set(&flt, _filter);

      // set the new filter
      if ( ( s = bt_devfilter(c_sock, &flt, 0)) < 0 ) {
	c_logger.error("Unable to set device filter");
	throw Exception("Unable to set device filter");
      }
    }

    /*
      Bluetooth main class
     */

    Bluetooth::Bluetooth(): ConfigNode(), ThreadManager::Thread(),
			    c_device("auto"), c_logger("Bluetooth") {
    }

    Bluetooth::~Bluetooth() {
    }

    std::shared_ptr<Bluetooth> Bluetooth::getInstance() {
      static std::shared_ptr<Bluetooth> instance{new Bluetooth()};
      return instance;
    }

    void Bluetooth::marshall(ryml::NodeRef& _node) {
      _node |= ryml::MAP;
      auto tree = _node.tree();

      // device
      {
	auto csubstr = tree->to_arena(c_device);
	_node["device"] = csubstr;
      }
    }

    void Bluetooth::unmarshall(ryml::ConstNodeRef& _node) {
      if ( !_node.is_map() )
	throw Exception("bluetooth node is not a map");

      if ( _node.has_child("device") )
	_node["device"] >> c_device;
    }

    void Bluetooth::init() {
      printf("Bluetooth::init\n");
      if ( c_device == "auto" ) {
	c_logger.debug("Enumerating Bluetooth devices");
	getDeviceList();
	if ( c_devlist.size()==0 ) {
	  c_logger.fatal("No bluetooth devices found");
	  throw Exception("No bluetooth devices found");
	}
	// select the first device
	c_device_selected = *c_devlist.begin();
      } else {
	c_device_selected = c_device;
      }
      printf("BT dev: %s\n", c_device_selected.c_str());

      // get the device information
      struct bt_devinfo di;
      memcpy((void*)di.devname,
	     (void*)c_device_selected.data(),
	     c_device_selected.size());

      if ( bt_devinfo(&di)!=0 ) {
	c_logger.fatal("Unable to get device info for %s",
			c_device_selected.c_str());
	throw Exception("Unable to get device info for %s",
			c_device_selected.c_str());
      }

      // state needs to be open
      if ( di.state != NG_HCI_CON_OPEN ) {
	c_logger.fatal("Device %s is not open",
		       c_device_selected.c_str());
	throw Exception("Device %s is not open",
		       c_device_selected.c_str());
      }

      // check for power control feature
      if ( !(di.features[2] & NG_HCI_LMP_POWER_CONTROL) ) {
	c_logger.fatal("Device %s lacks power control feature",
		       c_device_selected.c_str());
	throw Exception("Device %s lacks power control feature",
		       c_device_selected.c_str());
      }

      // we're good
      c_logger.info("Selected device %s with address %s",
		    c_device_selected.c_str(),
		    bdaddr(di.bdaddr).c_str());
    }

    void Bluetooth::worker() {
      int kq;
      char *buffer(0);
      long buffsize;
      struct kevent evlist[4];
      struct timespec to;

      buffsize = sysconf(_SC_PAGESIZE);
      buffer = (char*)malloc(buffsize);
      LE btle(c_device_selected);
      kq = kqueue();

      EV_SET(&evlist[0], btle.sock(),
	     EVFILT_READ, EV_ADD, 0, 0, 0);
      if ( kevent(kq, evlist, 1, 0, 0, 0)<0 ) {
	c_logger.fatal("kevent(): %s", strerror(errno));
	free(buffer);
	close(kq);
	throw Exception("Bluetooth: kevent(): %s", strerror(errno));
      }

      to.tv_sec = 1;
      to.tv_nsec = 0;
      while ( c_run ) {
	int nevents;
	if ( (nevents = kevent(kq, 0, 0, evlist, 4, &to))<0 ) {
	  c_logger.error("kevent(): %s", strerror(errno));
	  continue;
	}
	printf("nevents: %i\n", nevents);
	// if no events, we just loop back
	if ( !nevents ) continue;
	c_logger.debug("events from kqueue: %i", nevents);
	printf("%s:%i\n", __FILE__, __LINE__);

	// handle the events
	for (int i=0; i<nevents; ++i ) {
	  c_logger.info("Event data len: %i", evlist[i].data);
	  // empty the buffer fist for safety
	  memset((void*)buffer, 0, buffsize);

	  // now we can read the input
	  if ( recv(evlist[i].ident, buffer, buffsize, 0)
	       != evlist[i].data ) {
	    c_logger.error("Was unable to read the whole data");
	    continue;
	  }
	  hexdump(buffer, evlist[i].data);
	}
      }

      free(buffer);
      close(kq);
    }

    void Bluetooth::getDeviceList() {
      c_devlist.clear();

      int ndev;
      if ( (ndev = bt_devenum(&bt_devenum_cb, (void*)&c_devlist))<0 )
	throw Exception("bt_devenum error: %s", strerror(errno));
    }
  }
}
