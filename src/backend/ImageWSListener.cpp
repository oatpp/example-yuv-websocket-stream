/**
 * @file ${FILE_NAME}
 *
 * @brief ToDo
 *
 * @author Benedikt-Alexander Mokroß <oatpp@bamkrs.de>
 */

#include "ImageWSListener.hpp"

#include <oatpp/core/macro/component.hpp>

#undef OATPP_LOGD
#define OATPP_LOGD(...) do{}while(false)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageWSRegistry
int ImageWSRegistry::add(const oatpp::websocket::WebSocket *recv) {
  auto it = find(recv);
  if(it == m_registry.end()) {
    OATPP_LOGD("ImageWSRegistry", "Registering %p", recv);
    m_registry.push_back(recv);
  } else {
    OATPP_LOGD("ImageWSRegistry", "%p already registered", recv);
  }
  return 0;
}

int ImageWSRegistry::rm(const oatpp::websocket::WebSocket *recv) {
  auto it = find(recv);
  if(it != m_registry.end()) {
    OATPP_LOGD("ImageWSRegistry", "Unregistering %p", recv);
    m_registry.erase(it);
  } else {
    OATPP_LOGD("ImageWSRegistry", "%p not registered", recv);
  }
  return 0;
}

int ImageWSRegistry::distributeImage(const void *p, int size) {
  OATPP_LOGD("ImageWSRegistry", "Distributing image");
  oatpp::String image((const char*)p, size, true);
  for(auto & it : m_registry) {
    OATPP_LOGD("ImageWSRegistry", "Sending image to %p", it);
    it->sendOneFrameBinary(image);
  }
  return m_registry.size();
}

std::vector<const oatpp::websocket::WebSocket *>::const_iterator ImageWSRegistry::find(const oatpp::websocket::WebSocket *recv) {
  for(auto it = m_registry.begin(); it != m_registry.end(); ++it) {
    if((*it) == recv) {
      return it;
    }
  }
  return m_registry.end();
}

ImageWSRegistry::~ImageWSRegistry() {
  for(auto & it : m_registry) {
    OATPP_LOGD("ImageWSRegistry", "Sending close to %p", it);
    it->sendClose();
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageWSListener

void ImageWSListener::onPing(const WebSocket& socket, const oatpp::String& message) {
  OATPP_LOGD(TAG, "onPing");
  socket.sendPong(message);
}

void ImageWSListener::onPong(const WebSocket& socket, const oatpp::String& message) {
  OATPP_LOGD(TAG, "onPong");
}

void ImageWSListener::onClose(const WebSocket& socket, v_uint16 code, const oatpp::String& message) {
  OATPP_LOGD(TAG, "onClose code=%d", code);
  //OATPP_COMPONENT(std::shared_ptr<USBBridgeEvents>, evs, "eventDistributor");
}

void ImageWSListener::readMessage(const WebSocket& socket, v_uint8 opcode, p_char8 data, oatpp::v_io_size size) {

  if(size == 0) { // message transfer finished

    auto wholeMessage = m_messageBuffer.toString();
    m_messageBuffer.clear();
    OATPP_LOGD(TAG, "onMessage message='%s'", wholeMessage->c_str());

  } else if(size > 0) { // message frame received
    m_messageBuffer.writeSimple(data, size);
  }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageWSInstanceListener

std::atomic<v_int32> ImageWSInstanceListener::SOCKETS(0);

void ImageWSInstanceListener::onAfterCreate(const oatpp::websocket::WebSocket& socket, const std::shared_ptr<const ParameterMap>& params) {

  SOCKETS ++;
  OATPP_LOGD(TAG, "New Incoming Connection. Connection count=%d", SOCKETS.load());

  /* In this particular case we create one ImageWSListener per each connection */
  /* Which may be redundant in many cases */

  // each one gets their own listener
  socket.setListener(std::make_shared<ImageWSListener>());

  if(SOCKETS > 0) {
    m_grabber->start_capturing();
  }

  m_reg->add(&socket);
}

void ImageWSInstanceListener::onBeforeDestroy(const oatpp::websocket::WebSocket& socket) {

  SOCKETS --;
  OATPP_LOGD(TAG, "Connection closed. Connection count=%d", SOCKETS.load());

  m_reg->rm(&socket);

  if(SOCKETS < 1) {
    m_grabber->stop_capturing();
  }

}
