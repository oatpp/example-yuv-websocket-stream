
#include "Utils.hpp"
oatpp::String Utils::getExtension(const oatpp::String &filename) {
  v_int32 dotPos = 0;
  for(v_int32 i = filename->getSize() - 1; i > 0; i--) {
    if(filename->getData()[i] == '.') {
      dotPos = i;
      break;
    }
  }
  if(dotPos != 0 && dotPos < filename->getSize() - 1) {
    return oatpp::String((const char*)&filename->getData()[dotPos + 1], filename->getSize() - dotPos - 1);
  }
  return oatpp::String();
}

oatpp::String Utils::guessMimeType(const oatpp::String &filename) {
  auto extension = getExtension(filename);
  if(extension) {

    if(extension->equals("js")){
      return "application/javascript; charset=utf-8";
    } else if(extension->equals("html")){
      return "text/html; charset=utf-8";
    } else if(extension->equals("css")){
      return "text/css; charset=utf-8";
    } else if(extension->equals("jpg") || extension->equals("jpeg")){
      return "image/jpeg";
    } else if(extension->equals("png")){
      return "image/png";
    } else if(extension->equals("ico")){
      return "image/x-icon";
    } else if(extension->equals("gif")){
      return "image/gif";
    }

  }
  return "application/octet-stream";
}
