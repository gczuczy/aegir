
#include "ConfigBase.hh"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <filesystem>

#include "logging.hh"
#include "Exception.hh"
#include "misc.hh"

namespace aegir {

  /*
   * ConfigNode
   */
  ConfigNode::ConfigNode() {
  }

  ConfigNode::~ConfigNode() {
  }

  /*
   * SeverityLevelNode
   */
  ConfigBase::SeverityLevelNode::SeverityLevelNode(blt::severity_level& _loglevel)
    : ConfigNode(), c_loglevel(_loglevel) {
  }

  ConfigBase::SeverityLevelNode::~SeverityLevelNode() {};

  void ConfigBase::SeverityLevelNode::marshall(ryml::NodeRef& _node) {
    _node << toStr(c_loglevel);
  }

  void ConfigBase::SeverityLevelNode::unmarshall(ryml::ConstNodeRef& _node) {
    if ( _node.is_map() || _node.is_seq() )
      throw Exception("Node is not a scalar type");

    if ( !_node.has_val() )
      throw Exception("Node has no value");

    std::string val;
    _node >> val;
    c_loglevel = parseLoglevel(val);
  }

  /*
   * Config
   */
  ConfigBase::ConfigBase(): c_autosave(true) {
  }

  ConfigBase::~ConfigBase() {
    if ( c_autosave ) save();
  }

  void ConfigBase::load(const std::string& _file) {
    c_cfgfile = _file;
    // read the file
    ryml::Tree tree;
    {
      std::ifstream f(_file, std::ios::in | std::ios::binary);
      const auto size = std::filesystem::file_size(_file);
      std::string contents(size, '\0');

      f.read(contents.data(), size);

      ryml::csubstr csrcview(contents.c_str(), contents.size());
      tree = ryml::parse_in_arena(csrcview);
      f.close();
    }
    ryml::ConstNodeRef root = tree.crootref();

    SectionPath p;
    walkConfig(p, root);
  }

  void ConfigBase::walkConfig(const SectionPath& _path, ryml::ConstNodeRef& _root) {
    for (ryml::ConstNodeRef node: _root.children()) {
      SectionPath p = _path;
      auto c4substr = node.key();
      p.emplace_back(std::string(c4substr.data(), c4substr.size()));

      // look for the handler
      bool found = false;
      for (auto it: c_handlers) {
	if ( it.path != p ) continue;
	found = true;
	it.handler->unmarshall(node);
	break;
      }
      if ( !found ) {
	std::string strpath;
	for (auto it: p) strpath += it + ".";
	strpath.resize(strpath.size()-1);
	throw Exception("Path not found in config: %s", strpath.c_str());
      }
    }
  }

  void ConfigBase::save() {
    if ( c_cfgfile.empty() ) return;
    ryml::Tree tree;
    ryml::NodeRef root = tree.rootref();
    root |= ryml::MAP;

    // invoke the handlers
    for (auto hit: c_handlers) {
      ryml::NodeRef currnode = tree.rootref();
      for (auto pit = hit.path.begin(); pit != hit.path.end(); ++pit) {
	//auto csubstr = c4::to_csubstr(*pit);
	//auto csubstr = ryml::csubstr(pit->c_str(), pit->size());
	auto csubstr = tree.to_arena(*pit);
	if ( pit == --hit.path.end() ) {
	  // if it's the last one, then call the handler
	  ryml::NodeRef newnode;
	  newnode = currnode[csubstr];
	  hit.handler->marshall(newnode);
	  break;
	} else if ( currnode.has_child(csubstr) ) {
	  // if the key already exists, we're going to the next one
	  currnode = currnode[csubstr];
	  continue;
	} else {
	  // otherwise it's not the last one
	  // and the key doesn't exist yet
	  // so we create the key
	  currnode[csubstr] |= ryml::MAP;
	  currnode = currnode[csubstr];
	}
      }
    }

    std::ofstream outfile;
    outfile.open(c_cfgfile, std::ios::out | std::ios::trunc);
    outfile << tree;
    outfile.flush();
    outfile.close();
  }

  void ConfigBase::save(const std::string& _file) {
    c_cfgfile = _file;
    save();
  }

  void ConfigBase::autosave(bool _auto) {
    c_autosave = _auto;
  }

  template<>
  void ConfigBase::registerHandler(const std::string& _name,
				   blt::severity_level& _variable,
				   std::function<void(const blt::severity_level&)> _checker) {
    if ( checkHandler({_name}) )
      throw Exception("Handler already defined: %s", _name.c_str());

    c_handlers.emplace_back(SectionPath{_name},
			    ConfigNodeHandler{new SeverityLevelNode(_variable)});
  }

  bool ConfigBase::checkHandler(SectionPath _path) {
    for (auto it: c_handlers) {
      if ( it.path == _path ) return true;
    }
    return false;
  }
}
