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
#include "common/ServiceManager.hh"

namespace aegir {

  enum class MessageGroup: uint8_t {
    CORE=0,
    BREWD,
    FERMD
  };

  enum class MessageType: uint8_t {
    RAW=0,
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
    typedef uint16_t size_type;
    struct headers {
      MessageGroup group;
      std::uint8_t type;
      size_type size;
    };
  protected:
    Message(MessageGroup _group, std::uint8_t _type,
	    size_type _size = sizeof(headers));
    Message(const void*);
  public:
    Message()=delete;
    Message(const Message&)=delete;
    Message(Message&&)=delete;
    virtual ~Message()=0;

    inline MessageGroup group() const { return c_headers->group; };
    inline std::uint8_t type() const { return c_headers->type; };
    inline size_type size() const { return c_headers->size; };

    template<typename T>
    void setPointer(T*& _ptr, size_type _offset) {
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
    virtual const void* serialize() const {
      return c_buffer;
    };

  protected:
    void resize(size_type _size);
    template<typename T>
    T* dataPtr() const {
      return (T*)(((char*)c_buffer) + sizeof(headers));
    };

  protected:
    void* c_buffer;
    headers* c_headers;
  };

  /*
    RawMessage for holding raw data
   */
  class RawMessage: public Message {
  public:
    typedef std::shared_ptr<RawMessage> ptr;
    static constexpr aegir::MessageGroup msg_group =
      aegir::MessageGroup::CORE;
    static constexpr std::uint8_t msg_type = (uint8_t)aegir::MessageType::RAW;

  public:
    RawMessage()=delete;
    RawMessage(const RawMessage&)=delete;
    RawMessage(RawMessage&&)=delete;
    RawMessage(const void* _data, size_type _size, bool _copy=false);
    virtual ~RawMessage();

    inline const void* data() const { return c_data; };

    virtual const void* serialize() const;

  private:
    void* c_data;
    bool c_iscopy;
  };

  /*
    Factory to unserialize buffers to message objects
   */
  class MessageFactory: public Service {
  public:
    // the header is also passed which contains the message size
    // and parse() ensures the size match, so no len is needed
    typedef std::function<message_type(const char*)> handler_type;
  protected:
    MessageFactory() noexcept;
    friend class ServiceManager;
  private:
    MessageFactory(const MessageFactory&)=delete;
    MessageFactory(MessageFactory&&)=delete;

  public:
    virtual ~MessageFactory();
    virtual void bailout();

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
