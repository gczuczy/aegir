/*
  ServiceManager
  Registers services and maintains its indexes
  Can be used to deallocate the instance stack
 */

#ifndef AEGIR_SERVICEMANAGER
#define AEGIR_SERVICEMANAGER

#include <concepts>
#include <memory>
#include <typeinfo>
#include <typeindex>
#include <map>
#include <atomic>

#include <boost/type_index.hpp>

#include "common/Exception.hh"

namespace aegir {

  class Service {
  public:
    typedef std::shared_ptr<Service> service_ptr;
  public:
    virtual ~Service()=0;

    virtual void bailout()=0;
  };
  template<class T>
  concept IsService = std::is_base_of<Service, T>::value;

  class ServiceManager {
  public:
    typedef std::shared_ptr<ServiceManager> ptr;

  public:
    ServiceManager();
    ServiceManager(const ServiceManager&)=delete;
    ServiceManager(ServiceManager&&)=delete;
    virtual ~ServiceManager();

  public:
    template<IsService T>
    void add() {
      c_services.emplace(std::piecewise_construct,
			 std::forward_as_tuple(std::type_index(typeid(T))),
			 std::forward_as_tuple(std::shared_ptr<T>{new T()}));
    };

    template<IsService T>
    static inline std::shared_ptr<T> get() {
      if ( c_instance == nullptr )
	throw Exception("ServiceMonitor not yet instantiated");
      return c_instance.load()->getService<T>();
    }

    template<IsService T>
    std::shared_ptr<T> getService() {
      const auto& it = c_services.find(std::type_index(typeid(T)));
      if ( it == c_services.end() )
	throw Exception("Type %s not registered in SeviceManager@%p",
			boost::typeindex::type_id<T>().pretty_name().c_str(),
			this);

      return std::static_pointer_cast<T>(it->second);
    };

  private:
    static std::atomic<ServiceManager*> c_instance;
    std::map<std::type_index, Service::service_ptr> c_services;
  };
}

#endif
