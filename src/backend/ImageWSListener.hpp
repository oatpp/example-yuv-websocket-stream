/**
 * @file ${FILE_NAME}
 *
 * @brief ToDo
 *
 * @author Benedikt-Alexander Mokroß <oatpp@bamkrs.de>
 */
#ifndef IMAGEWSLISTENER_HPP_
#define IMAGEWSLISTENER_HPP_

#include <oatpp-websocket/ConnectionHandler.hpp>
#include <oatpp-websocket/WebSocket.hpp>
#include <utility>
#include <vector>
#include <memory>

#include "backend/V4LGrabber.hpp"

class ImageWSRegistry {
 public:
  static constexpr const char *TAG = "ImageWSRegistry";
  explicit ImageWSRegistry() {};
  static std::shared_ptr<ImageWSRegistry> createShared() {
    return std::make_shared<ImageWSRegistry>();
  }
  ~ImageWSRegistry();

  int add(const oatpp::websocket::WebSocket *recv);
  int rm(const oatpp::websocket::WebSocket *recv);

  int distributeImage(const void* p, int size);

 private:
  std::vector<const oatpp::websocket::WebSocket *>::const_iterator find(const oatpp::websocket::WebSocket *recv);
  std::vector<const oatpp::websocket::WebSocket *> m_registry;
};

/**
 * WebSocket listener listens on incoming WebSocket events.
 */
class ImageWSListener : public oatpp::websocket::WebSocket::Listener {
 private:
  static constexpr const char *TAG = "ImageWSListener";

  /**
   * Buffer for messages. Needed for multi-frame messages.
   */
  oatpp::data::stream::ChunkedBuffer m_messageBuffer;

 private:

 public:

  explicit ImageWSListener() {};

  /**
   * Called on "ping" frame.
   */
  void onPing(const WebSocket &socket, const oatpp::String &message) override;

  /**
   * Called on "pong" frame
   */
  void onPong(const WebSocket &socket, const oatpp::String &message) override;

  /**
   * Called on "close" frame
   */
  void onClose(const WebSocket &socket, v_uint16 code, const oatpp::String &message) override;


  /**
   * Called on each message frame. After the last message will be called once-again with size == 0 to designate end of the message.
   */
  void readMessage(const WebSocket &socket, v_uint8 opcode, p_char8 data, oatpp::v_io_size size) override;

};

/**
 * Listener on new WebSocket connections.
 */
class ImageWSInstanceListener : public oatpp::websocket::ConnectionHandler::SocketInstanceListener {
 private:
  static constexpr const char *TAG = "Server_WSInstanceListener";
  const std::shared_ptr<ImageWSRegistry> m_reg;
  const std::shared_ptr<V4LGrabber> m_grabber;

 public:
  /**
   * Counter for connected clients.
   */
  static std::atomic<v_int32> SOCKETS;
 public:
  ImageWSInstanceListener(std::shared_ptr<ImageWSRegistry> reg, std::shared_ptr<V4LGrabber> grabber) : m_reg(std::move(reg)), m_grabber(std::move(grabber)) {};
  static std::shared_ptr<ImageWSInstanceListener> createShared(const std::shared_ptr<ImageWSRegistry> &reg, const std::shared_ptr<V4LGrabber> &grabber) {
    return std::make_shared<ImageWSInstanceListener>(reg, grabber);
  }

  /**
   *  Called when socket is created
   */
  void onAfterCreate(const oatpp::websocket::WebSocket &socket,
                     const std::shared_ptr<const ParameterMap> &params) override;

  /**
   *  Called before socket instance is destroyed.
   */
  void onBeforeDestroy(const oatpp::websocket::WebSocket &socket) override;

};


#endif //IMAGEWSLISTENER_HPP_
