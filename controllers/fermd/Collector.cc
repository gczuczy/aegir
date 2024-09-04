
#include "Collector.hh"

#include <sys/event.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "ZMQConfig.hh"
#include "common/Exception.hh"
#include "DBConnection.hh"

// sleep time for main loop in milliseconds
#define SLEEP_TIME_MS 100
// offset is when we're starting up and have to wait for the next whole hour
#define TIMER_OFFSET 1
// id of the hourly regular timer
#define TIMER_HOURLY 2
// how long is an hour - for testing
#define HOUR_SECONDS 3600

// uncomment after data gathering
#define STORE_DATA
#ifdef STORE_DATA
#define DATA_PATH "/tmp/fermd-data"

static std::string dataFileName(int _fid, time_t _ts) {
	char buff[128];
	size_t len;
	len = snprintf(buff, 127, "%s/datapoint-%i-%li", DATA_PATH, _fid, _ts);
	return std::string(buff, len);
}

#endif

namespace aegir {
	namespace fermd {

		void Collector::fermenter::reset(bool _keepid) {
			if ( !_keepid ) {
				id = 0;
				active = false;
			}
			memset((void*)readings, 0, sizeof(readings));
		}

		Collector::Collector(): aegir::Thread(), aegir::Service(),
														LogChannel("Collector"), c_kq(-1), c_isactive(false) {
		}

		Collector::~Collector() {
			close(c_kq);
		}

		void Collector::init() {
      c_sock = ServiceManager::get<ZMQConfig>()->dstSocket("sensorbus");
      c_sock->setRecvTimeout(100);
      c_sock->subscribe("");
      c_sock->brrr();

			c_kq = kqueue();
			// first we check whether we install the hourly, or the
			// offset to the next hour
			struct timeval tv;
			if ( gettimeofday(&tv, 0) != 0 )
				throw Exception("Gettimeofday error(%i): %s", errno, strerror(errno));

			if ( tv.tv_sec%HOUR_SECONDS == 0 ) {
				info("Installing hourly event");
				installHourly();
			} else {
				info("Installing offset event");
				struct kevent ev;
				int offset = HOUR_SECONDS - (tv.tv_sec%HOUR_SECONDS);

				// EV_SET(kev, ident, filter, flags, fflags, data, udata);
				EV_SET(&ev, TIMER_OFFSET, EVFILT_TIMER, EV_ADD|EV_ONESHOT,
							 NOTE_SECONDS, offset, 0);

				if ( kevent(c_kq, &ev, 1, 0, 0, 0) < 0 )
					throw Exception("kevent() error(%i): %s", errno, strerror(errno));
			}

		}

		void Collector::worker() {
			struct timespec to;
			struct kevent events[2];
			int nevents;
			message_type msg;

			while ( c_run ) {
				to.tv_sec = 0;
				to.tv_nsec = SLEEP_TIME_MS * 1000000;

				// poll the events
				if ( (nevents = kevent(c_kq, 0, 0, events, 2, &to)) < 0 ) {
					error("kevent(%i): %s", errno, strerror(errno));
					continue;
				}

				// first handle the timer (we only have a single timer here)
				if ( nevents == 1 ) {
					if ( events[0].ident == TIMER_OFFSET ) {
						installHourly();
						reloadFermenters();
					} else if ( events[0].ident == TIMER_HOURLY ) {
						aggregateData();
						reloadFermenters();
					}
				}

				// once we have the timers, handle the ZMQ sockets
				while ( (msg = c_sock->recv(false)) ) {
					// if we haven't installed the hourly, then we just skip it
					if ( ! c_isactive ) continue;
					if ( msg->group() == TiltReadingMessage::msg_group &&
							 msg->type() == TiltReadingMessage::msg_type ) {
						auto tiltmsg = msg->as<TiltReadingMessage>();
#if 0
						printf("Collector UUID:%s %.2fC %.4fSG\n",
									 boost::lexical_cast<std::string>(tiltmsg->uuid()).c_str(),
									 tiltmsg->temp(), tiltmsg->sg());
#endif
						storeTiltReading(tiltmsg);
					} else {
						warning("Got unknown message type on sensorbus");
					}
				}
			}
		}

		void Collector::stop() {
			c_run = false;
		}

		void Collector::bailout() {
			stop();
		}

		void Collector::installHourly() {
			struct kevent ev;

			// EV_SET(kev, ident, filter, flags, fflags, data, udata);
			EV_SET(&ev, TIMER_HOURLY, EVFILT_TIMER, EV_ADD, NOTE_SECONDS,
						 HOUR_SECONDS, 0);

			if ( kevent(c_kq, &ev, 1, 0, 0, 0) < 0 )
				throw Exception("kevent() error(%i): %s", errno, strerror(errno));

			info("Hourly timer installed");
			c_isactive = true;
		}

		void Collector::aggregateData() {
			time_t t = time(0)%HOUR_SECONDS;

			for (auto& it: c_fermenters) {
				std::shared_ptr<fermenter> f = it.second;
				if ( !f->active ) continue;
#ifdef STORE_DATA
				trace("Storing data for fermenter %i", f->id);
				int fd;
				std::string fpath = dataFileName(f->id, t);
				if ( (fd = open(fpath.c_str(), O_WRONLY|O_TRUNC|O_CREAT,
												S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0 ){
					error("Unable to dump data: open(): %i/%s", errno, strerror(errno));
				} else {
					ssize_t len;
					if ( (len = write(fd, (void*)f.get(), sizeof(fermenter)))<0 ) {
						error("Unable to dump data: write(): %i/%s", errno, strerror(errno));
					} else {
						trace("Dumped fermenter %i data at %li", f->id, t);
					}
					close(fd);
				}
#endif
			}
			info("aggregateData called");
		}

		void Collector::reloadFermenters() {
			auto dbc = aegir::ServiceManager::get<aegir::fermd::DB::Connection>();
			auto fermenters = dbc->getFermenters();
			// add fermenters which are not present
			for (auto it: fermenters) {
				if ( c_fermenters.find(it->id) == c_fermenters.end() ) {
					auto f = std::make_shared<fermenter>();
					f->reset();
					f->id = it->id;
					f->active = !!it->cache_brewid;
					c_fermenters.emplace(it->id, f);
					info("Added %s fermenter %s with id %i",
							 it->cache_brewid ? "active" : "passive",
							 it->name.c_str(), it->id);
					continue;
				}
			}
			// now remove obsolete fermenters
			for (auto it: c_fermenters) {
				bool found(false);
				bool active(false);
				for (auto it2: fermenters) {
					if ( it2->id == it.second->id ) {
						found = true;
						active = !!it2->cache_brewid;
						break;
					}
				}
				if ( !found ) {
					c_fermenters.erase(it.first);
					info("Removed fermenter with id %i", it.second->id);
				} else {
					it.second->reset(true);
					it.second->active = active;
				}
			}
			info("Reloaded fermenters");
		}

		void Collector::storeTiltReading(std::shared_ptr<TiltReadingMessage> _msg) {
			auto dbc = aegir::ServiceManager::get<aegir::fermd::DB::Connection>();
			auto tilt = dbc->getTilthydrometerByUUID(_msg->uuid());

			// check whether we indeed have this tilt
			if ( !tilt ) {
				warn("storeTiltReading: Unable to find tilt by UUID %s",
						 boost::lexical_cast<std::string>(_msg->uuid()).c_str());
				return;
			}

			// check whether it's assigned
			if ( !tilt->fermenter ) {
				warn("storeTiltReading: Hydrometer %s has no fermenter",
						 boost::lexical_cast<std::string>(_msg->uuid()).c_str());
				return;
			}

			// chack that we indeed have the assigned fermenter active
			auto fit = c_fermenters.find(tilt->fermenter->id);
			if ( fit == c_fermenters.end() ) {
				warn("storeTiltReading: Hydrometer %s assigned fermenter not present",
						 boost::lexical_cast<std::string>(_msg->uuid()).c_str());
				return;
			}

			// check whether it is active
			if ( !fit->second->active ) {
				warn("storeTiltReading: Hydrometer %s assigned fermenter inactive",
						 boost::lexical_cast<std::string>(_msg->uuid()).c_str());
				return;
			}

			// TODO: apply SG calibration first
			// now we can store the result
			int tidx = _msg->time()%HOUR_SECONDS;
			fit->second->readings[tidx].temp_tilt = _msg->temp();
			fit->second->readings[tidx].sg = _msg->sg();
			fit->second->readings[tidx].has_tilt = true;
			trace("storeTiltReading: Hydrometer %s reading stored: T:%.2f SG:%.3f",
						boost::lexical_cast<std::string>(_msg->uuid()).c_str(),
						_msg->temp(), _msg->sg());
		}
	}
}
