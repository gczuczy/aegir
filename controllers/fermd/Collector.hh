/*
	Collector service

	Collects sensor readings and produces datapoints based on that
 */

#ifndef AEGIR_FERMD_COLLECTOR
#define AEGIR_FERMD_COLLECTOR

#include <memory>
#include <map>

#include "common/ThreadManager.hh"
#include "common/ServiceManager.hh"
#include "common/ZMQ.hh"
#include "common/LogChannel.hh"
#include "common/NoiseFilter.hh"
#include "uuid.hh"
#include "Message.hh"

// how long is an hour - for testing
#define HOUR_SECONDS 3600

namespace aegir {
	namespace fermd {

		class Collector: public Thread,
										 public Service,
										 private LogChannel {
      friend class aegir::ServiceManager;
		private:
			struct fermenter {
				struct reading {
					bool has_sensor;
					float temp_sensor;
					bool has_tilt;
					float temp_tilt;
					float sg;
				};
				void reset(bool _keepid=false);
				int id;
				int brewid;
				bool active;
				reading readings[3600];
			};
			typedef std::shared_ptr<fermenter> fermenter_ptr;

		protected:
			Collector();
			Collector(const Collector&)=delete;
			Collector(Collector&&)=delete;

		public:
			virtual ~Collector();

			virtual void init();
			virtual void worker();
			virtual void stop();
			virtual void bailout();

		private:
			void installHourly();
			void aggregateData();
			void reloadFermenters();

			void storeTiltReading(std::shared_ptr<TiltReadingMessage> _msg);

		private:
			NoiseFilter<HOUR_SECONDS> c_nf;
      aegir::zmqsocket_type c_sock;
			int c_kq;
			std::map<int, fermenter_ptr> c_fermenters;
			bool c_isactive;
		};
	}
}


#endif
