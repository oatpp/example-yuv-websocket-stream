#ifndef DTOs_hpp
#define DTOs_hpp

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

namespace dtov0 {

class MessageDto : public oatpp::DTO {

  DTO_INIT(MessageDto, DTO);

  DTO_FIELD(Int32, statusCode);
  DTO_FIELD(String, message);

};

class ErrorDto : public oatpp::DTO{

  DTO_INIT(ErrorDto, DTO)

  DTO_FIELD(Int32, code);
  DTO_FIELD(String, message);
  DTO_FIELD(String, description);

};

}

#include OATPP_CODEGEN_END(DTO)

#endif /* DTOs_hpp */
