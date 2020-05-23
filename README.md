# Example-YUV-Websocket-Stream

Example project how-to create a YUV image stream from a V4L device (i.E. Webcam) using websockets.
The raw YUV image stream is send via a websocket connection. In the example Webpage, this YUV stream is converted to an HTML5 Canvas using JavaScript. 
If you experience lag in the video its either your PC not being fast enough for the JavaScript conversion or the V4L2 stack.
The example webpage also runs fine on newer Smartphones!

See more:

- [Oat++ Website](https://oatpp.io/)
- [Oat++ Github Repository](https://github.com/oatpp/oatpp)
- [Get Started](https://oatpp.io/docs/start)

## Overview

This project is using [oatpp](https://github.com/oatpp/oatpp), [oatpp-websocket](https://github.com/oatpp/oatpp-websocket) and [oatpp-swagger](https://github.com/oatpp/oatpp-swagger) modules.

### Project layout

```
|- CMakeLists.txt                        // projects CMakeLists.txt
|- src/
|   |
|   |- controller/                       // Folder containing CamAPIController where all endpoints are declared
|   |- backend/                          // Folder with "business logic"
|   |- dto/                              // DTOs are declared here
|   |- SwaggerComponent.hpp              // Swagger-UI config
|   |- AppComponent.hpp                  // Service config
|   |- App.cpp                           // main() is here
|
|- utility/install-oatpp-modules.sh      // utility script to install required oatpp-modules.
```

---

### Build and Run

#### Using CMake

**Requires**

- `oatpp`, `oatpp-websocket` and `oatpp-swagger` modules installed. You may run `utility/install-oatpp-modules.sh` 
script to install required oatpp modules.
- Linux with `V4L2` development libraries installed

```
$ mkdir build && cd build
$ cmake ..
$ make 
$ ./yuv-websocket-exe        # - run application.
```

#### In Docker

```
$ docker build -t example-yuv-websocket .
$ docker run -p 8000:8000 -t example-crud
```

---