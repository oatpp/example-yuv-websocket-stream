
#include "Utils.hpp"
oatpp::String Utils::getExtension(const oatpp::String &filename) {
  v_int32 dotPos = 0;
  for(v_int32 i = filename->size() - 1; i > 0; i--) {
    if(filename->data()[i] == '.') {
      dotPos = i;
      break;
    }
  }
  if(dotPos != 0 && dotPos < filename->size() - 1) {
    return oatpp::String((const char*)&filename->data()[dotPos + 1], filename->size() - dotPos - 1);
  }
  return oatpp::String();
}

oatpp::String Utils::guessMimeType(const oatpp::String &filename) {
  auto extension = getExtension(filename);
  if(extension) {

    if(extension == "js"){
      return "application/javascript; charset=utf-8";
    } else if(extension == "html"){
      return "text/html; charset=utf-8";
    } else if(extension == "css"){
      return "text/css; charset=utf-8";
    } else if(extension ==  "jpg" || extension == "jpeg"){
      return "image/jpeg";
    } else if(extension == "png"){
      return "image/png";
    } else if(extension == "ico"){
      return "image/x-icon";
    } else if(extension == "gif"){
      return "image/gif";
    }

  }
  return "application/octet-stream";
}
