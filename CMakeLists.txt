cmake_minimum_required(VERSION 3.1)
project(yuv-websocket)

set(CMAKE_CXX_STANDARD 11)

if(NOT UNIX)
    message( FATAL_ERROR "This example only targets Linux systems with V4L2 development libs installed!" )
endif(NOT UNIX)

add_library(yuv-websocket-lib
        src/AppComponent.hpp
        src/SwaggerComponent.hpp
        src/controller/CamAPIController.hpp
        src/controller/CamAPIController.cpp
        src/backend/V4LGrabber.hpp
        src/backend/V4LGrabber.cpp
        src/backend/ImageWSListener.hpp
        src/backend/ImageWSListener.cpp
        src/backend/Utils.hpp
        src/backend/Utils.cpp
        src/dto/DTOs.hpp)

## include directories

target_include_directories(yuv-websocket-lib PUBLIC src)


## link libs

find_package(oatpp            1.1.0 REQUIRED)
find_package(oatpp-swagger    1.1.0 REQUIRED)
find_package(oatpp-websocket  1.1.0 REQUIRED)

target_link_libraries(yuv-websocket-lib
        PUBLIC oatpp::oatpp
        PUBLIC oatpp::oatpp-swagger
        PUBLIC oatpp::oatpp-websocket
)

## define path to swagger-ui res folder

add_definitions(-DOATPP_SWAGGER_RES_PATH="${oatpp-swagger_INCLUDE_DIRS}/../bin/oatpp-swagger/res")
add_definitions(-DWWW_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/res")

## add executables

add_executable(yuv-websocket-exe
        src/App.cpp
)

target_link_libraries(yuv-websocket-exe yuv-websocket-lib)
