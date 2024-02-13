/* config loader
 */

#ifndef AEGIR_CONFIG_BASE_H
#define AEGIR_CONFIG_BASE_H

#include <string>
#include <memory>
#include <list>
#include <functional>
#include <concepts>

#include <boost/log/trivial.hpp>

#include "common/ryml.hh"

#include "common/Exception.hh"
#include "common/misc.hh"

namespace blt = ::boost::log::trivial;

namespace aegir {

  // This is a base class for configuring
  // nodes in the config file.
  // Subsystems are providing their own
  // serialization to yaml here
  class ConfigNode {
  public:
    ConfigNode();
    virtual ~ConfigNode()=0;

    virtual void marshall(ryml::NodeRef&)=0;
    virtual void unmarshall(ryml::ConstNodeRef&)=0;
  };

  // concepts

  typedef std::shared_ptr<ConfigNode> ConfigNodeHandler;

  class ConfigBase {
  private:
    // defining a section path in yaml
    typedef std::list<std::string> SectionPath;
    typedef std::function<void(ryml::NodeRef&)> marshall_f;
    typedef std::function<void(const ryml::NodeRef&)> unmarshall_f;

    // specialized config nodes
    class SeverityLevelNode: public ConfigNode {
    public:
      SeverityLevelNode() = delete;
      SeverityLevelNode(blt::severity_level& _loglevel);
      virtual ~SeverityLevelNode();
      virtual void marshall(ryml::NodeRef&);
      virtual void unmarshall(ryml::ConstNodeRef&);
    private:
      blt::severity_level& c_loglevel;
    };

    struct Section {
      SectionPath path;
      ConfigNodeHandler handler;
    };

  public:
    ConfigBase(const ConfigBase&) = delete;
    ConfigBase();
    virtual ~ConfigBase()=0;

    void load(const std::string& _file);
    void save();
    void save(const std::string& _file);
    void autosave(bool _auto);

  protected:
    template<Derived<ConfigNode> T>
    void registerHandler(const std::string& _name) {
      auto h = std::static_pointer_cast<ConfigNode>(T::getInstance());
      if ( checkHandler({_name}) )
	throw Exception("Handler already defined: %s", _name.c_str());
      c_handlers.emplace_back(SectionPath{_name},
			      h);
    }

    template<typename T>
    void registerHandler(const std::string& _name,
			 T& _variable,
			 std::function<void(const T&)> _checker = nullptr
			 ) {
      throw Exception("Generic ConfigBase::registerHandler not implemented");
    };
    template<>
    void registerHandler(const std::string& _name,
			 blt::severity_level& _variable,
			 std::function<void(const blt::severity_level&)> _checker);

  private:
    bool checkHandler(SectionPath _path);
    void walkConfig(const SectionPath& _path, ryml::ConstNodeRef& _root);

  private:
    // config sections
    std::list<Section> c_handlers;
    // the configfile
    std::string c_cfgfile;
    bool c_autosave;
  };
}

#endif
