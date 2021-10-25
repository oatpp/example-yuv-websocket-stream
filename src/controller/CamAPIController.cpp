
#include "CamAPIController.hpp"

#include <utility>
#include <backend/Utils.hpp>

const char* apiv0::CamAPIController::TAGCam = "Cam";
/*
 * Class-Logic
 */

apiv0::CamAPIController::~CamAPIController() {
  v4lDeinit();
}

int apiv0::CamAPIController::v4lInit() {
  m_imageReceivers = ImageWSRegistry::createShared();
  char device[12] = "/dev/videoX";

  if(V4LGrabber::testDevice("/dev/video0") == 0) {
    device[10] = '0';
  } else if (V4LGrabber::testDevice("/dev/video1") == 0) {
    device[10] = '1';
  } else if (V4LGrabber::testDevice("/dev/video2") == 0) {
    device[10] = '2';
  } else {
    OATPP_LOGE(TAGCam, "Non of the tested /dev/video devices could be opened");
    return -1;
  }

  m_grabber = std::make_shared<V4LGrabber>(device, &CamAPIController::handle_frame, m_imageReceivers.get(), V4LGrabber::IO_METHOD_MMAP);
  m_imagewsConnectionHandler = oatpp::websocket::ConnectionHandler::createShared();
  m_imagewsConnectionHandler->setSocketInstanceListener(std::make_shared<ImageWSInstanceListener>(m_imageReceivers, m_grabber));
  m_v4linit = true;
  return 0;
}

int apiv0::CamAPIController::v4lDeinit() {
  m_grabber->stop_capturing();
  m_imageReceivers.reset();
  m_grabber.reset();
  m_imagewsConnectionHandler->stop();
  m_imagewsConnectionHandler.reset();
  m_v4linit = false;
  return 0;
}

/* *******************************
 * Streaming
 */

void apiv0::CamAPIController::handle_frame(void *data, const void *image, int size) {
  OATPP_LOGD("ImageStreamingWSController", "Handling Frame");
  ImageWSRegistry* registry = (ImageWSRegistry*)data;
  registry->distributeImage(image, size);
}

std::shared_ptr<oatpp::web::protocol::http::outgoing::Response> apiv0::CamAPIController::stream() {

  try {
    oatpp::String str = oatpp::String::loadFromFile(WWW_FOLDER "/cam/wsImageView.html");
    auto rsp = createResponse(Status::CODE_200, str);
    rsp->putHeader("Content-Type", "text/html; charset=utf-8");
    return rsp;
  } catch(std::exception &e) {
    return createResponse(Status::CODE_505, "Error reading HTML-File");
  }
}


std::shared_ptr<oatpp::web::protocol::http::outgoing::Response> apiv0::CamAPIController::streamres(const oatpp::String &filename) {
  try {
    oatpp::String filteredName(basename(filename->c_str()));
    oatpp::String folderName(WWW_FOLDER "/cam/");
    oatpp::String str = oatpp::String::loadFromFile((folderName + filteredName)->c_str());
    auto rsp = createResponse(Status::CODE_200, str);
    rsp->putHeader("Content-Type", Utils::guessMimeType(filteredName));
    return rsp;
  } catch(std::exception &e) {
    return createResponse(Status::CODE_505, "Error reading HTML-File");
  }
}

const char * apiv0::CamAPIController::basename(const char *filename) {
  const char *p = strrchr (filename, '/');
  return p ? p + 1 : (const char *) filename;
}
