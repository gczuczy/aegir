
#ifndef AEGIR_FERMD_DB_CONNECTION
#define AEGIR_FERMD_DB_CONNECTION

#include <string>
#include <list>
#include <shared_mutex>

#include <sqlite3.h>

#include "DBTypes.hh"
#include "DBStatement.hh"
#include "DBTransaction.hh"
#include "DBResult.hh"
#include "DBSchema.hh"
#include "common/ConfigBase.hh"
#include "common/LogChannel.hh"
#include "common/ServiceManager.hh"

namespace aegir {
	namespace fermd {
		namespace DB {
			class Transaction;
			class Connection: public aegir::ConfigNode,
												public aegir::Service,
												private aegir::LogChannel {
		    friend class Transaction;
		    friend class aegir::ServiceManager;
      public:
				typedef std::shared_ptr<Connection> pointer_type;

      protected:
				Connection();

      public:
				Connection(const Connection&)=delete;
				Connection(Connection&&)=delete;
				virtual ~Connection();

				virtual void bailout();

				virtual void marshall(ryml::NodeRef&);
				virtual void unmarshall(ryml::ConstNodeRef&);

				void init();
				void setConnectionFile(const std::string& _file);

				// reloads
      protected:
				void reload();
      private:
				void reload_fermenter_types();
				void reload_fermenters();
				void reload_tilthydrometers();
				void reload_yeasts();
				void reload_brews();
				void reload_transfers();
				void reload_fermentationlog();
				void reload_fermentingbrews();

				// Tilt Hydrometers
      public:
				tilthydrometer_cdb getTilthydrometers() const;
				tilthydrometer::cptr getTilthydrometerByUUID(uuid_t _uuid) const;
				tilthydrometer::cptr getTilthydrometerByID(int _id) const;
      protected:
				void setTilthydrometer(const tilthydrometer& _item);

				// Fermenter Types
      public:
				fermenter_types_cdb getFermenterTypes() const;
				fermenter_types::cptr getFermenterTypeByID(int _id) const;
      protected:
				void updateFermenterType(const fermenter_types& _item);
				fermenter_types::cptr addFermenterType(const fermenter_types& _item);
				void deleteFermenterType(int _id);

				// fermenters
      public:
				fermenter_cdb getFermenters() const;
	      fermenter::cptr getFermenterByID(int _id) const;
				brew::cptr getFermenterBrew(int _fid) const;
      protected:
				void updateFermenter(const fermenter& _item);
				fermenter::cptr addFermenter(const fermenter& _item);
				void deleteFermenter(int _id);

				// yeasts
      public:
				yeast_cdb getYeasts() const;
				yeast::cptr getYeastByID(int _id) const;
      protected:
				void updateYeast(const yeast& _item);
				yeast::cptr addYeast(const yeast& _item);
				void deleteYeast(int _id);

				// brew
      public:
				brew_cdb getBrews() const;
				brew::cptr getBrewByID(int _id) const;
      protected:
				void updateBrew(const brew& _item);
				brew::cptr addBrew(const brew& _item);
				void deleteBrew(int _id);

				// transfer
      public:
				transfer_cdb getTransfers() const;
				transfer::cptr getTransferByID(int _id) const;
				transfer_cdb getTransfersByBrew(const brew& _brew) const;
      protected:
				transfer::cptr addTransfer(const transfer& _item);

				// fermentationlog
      public:
				fermentationlog_cdb getFermentationlogsByBrew(const brew& _brew) const;
      protected:
				fermentationlog::cptr addFermentationlog(const fermentationlog& _item);

      public:
				inline Transaction txn() {
					return Transaction(this);
				};
      protected:
				void begin();
				void commit();

      private:
				void prepare(const std::string& _name,
										 const std::string& _stmt,
										 bool _temporary=false);
				Statement& getStatement(const std::string& _name);

      private:
				std::string c_dbfile;
				sqlite3 *c_db;
				static thread_local bool c_threadlocked;
				std::map<std::string, Statement> c_statements;
				std::list<Schema> c_schemas;
				mutable std::mutex c_mtx;
				// storages
				mutable std::shared_mutex c_mtx_tilthydrometers;
				tilthydrometer_db cache_tilthydrometers;
				mutable std::shared_mutex c_mtx_fermenter_types;
				fermenter_types_db cache_fermenter_types;
				mutable std::shared_mutex c_mtx_fermenters;
				fermenter_db cache_fermenters;
				mutable std::shared_mutex c_mtx_yeasts;
				yeast_db cache_yeasts;
				mutable std::shared_mutex c_mtx_brews;
				brew_db cache_brews;
				mutable std::shared_mutex c_mtx_transfers;
				transfer_db cache_transfers;
				mutable std::shared_mutex c_mtx_fermentationlogs;
				fermentationlog_db cache_fermentationlogs;
      }; // class Connection

    } // ns DB
  } // ns fermd
} // ns aegir

#endif
