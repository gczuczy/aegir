/*
  Representing a message sent over the ZMQ buses
 */

#ifndef AEGIR_MESSAGE_H
#define AEGIR_MESSAGE_H

#include <memory>
#include <cstdint>
#include <functional>
#include <map>
#include <concepts>
#include <typeinfo>

#include "common/Exception.hh"
#include "common/misc.hh"

namespace aegir {

  enum class MessageGroup: uint8_t {
    CORE=0,
    BREWD,
    FERMD
  };

  class Message;
  typedef std::shared_ptr<Message> message_type;
  template<class T>
  concept IsMessage = requires(T a) {
    std::is_base_of<T, Message>::value;
    std::same_as<decltype(T::msg_group), aegir::MessageGroup>;
    std::same_as<decltype(T::msg_type), std::uint8_t>;
    std::same_as<decltype(T::parse), aegir::message_type(const char*)>;
  };
  // base class
  class Message: public std::enable_shared_from_this<Message> {
  public:
    struct headers {
      MessageGroup group;
      std::uint8_t type;
      std::uint16_t size;
    };
  protected:
    Message(MessageGroup _group, std::uint8_t _type,
	    std::uint16_t _size = sizeof(headers));
    Message(const void*);
  public:
    Message()=delete;
    Message(const Message&)=delete;
    Message(Message&&)=delete;
    virtual ~Message()=0;

    inline MessageGroup group() const { return c_headers->group; };
    inline std::uint8_t type() const { return c_headers->type; };
    inline std::uint16_t size() const { return c_headers->size; };

    template<typename T>
    void setPointer(T*& _ptr, std::uint16_t _offset) {
      _ptr = (T*)(dataPtr<char>() + _offset);
    }

    template<IsMessage T>
    std::shared_ptr<T> as() {
      if ( !(group() == T::msg_group && type() == T::msg_type) )
	throw Exception("Invalid message cast to %s", typeid(T).name());
      return std::static_pointer_cast<T>(shared_from_this());
    };

  protected:
    // by default blank, but should be overridden if needed
    virtual void setPointers();

  public:
    // size() always tells us the serialized length
    // this should not copy the data
    // can be reimplemented if needed
    virtual void* serialize() const {
      return c_buffer;
    };

  protected:
    void resize(std::uint16_t _size);
    template<typename T>
    T* dataPtr() const {
      return (T*)(((char*)c_buffer) + sizeof(headers));
    };

  protected:
    void* c_buffer;
    headers* c_headers;
  };

  /*
    Factory to unserialize buffers to message objects
   */
  class MessageFactory {
  public:
    // the header is also passed which contains the message size
    // and parse() ensures the size match, so no len is needed
    typedef std::function<message_type(const char*)> handler_type;
  private:
    MessageFactory() noexcept;
    MessageFactory(const MessageFactory&)=delete;
    MessageFactory(MessageFactory&&)=delete;

  public:
    ~MessageFactory();

    static std::shared_ptr<MessageFactory> getInstance();
    message_type parse(const char* _buffer, std::uint16_t _len);

    template<IsMessage T>
    void registerHandler() {
      auto group = T::msg_group;
      auto type = T::msg_type;
      auto git = c_handlers.find(group);
      if ( git == c_handlers.end() ) {
	c_handlers[group][type] = T::parse;
	return;
      }
      if ( git->second.find(type) != git->second.end() )
	throw Exception("Handler already registered for %u/%u",
			group, type);

      git->second[type] = T::parse;
    };

  private:
    std::map<MessageGroup, std::map<std::uint16_t, handler_type> > c_handlers;
  };
}

#endif
