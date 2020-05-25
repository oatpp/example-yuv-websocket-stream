
#ifndef SRC_BACKEND_UTILS_HPP
#define SRC_BACKEND_UTILS_HPP

#include <oatpp/core/Types.hpp>

class Utils {
 public:
  static oatpp::String getExtension(const oatpp::String& filename);
  static oatpp::String guessMimeType(const oatpp::String& filename);
};

#endif //SRC_BACKEND_UTILS_HPP
