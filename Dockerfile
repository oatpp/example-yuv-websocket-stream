FROM lganzzzo/alpine-cmake:latest

RUN apk add linux-headers

WORKDIR /service

ADD res res
ADD src src
ADD utility utility
ADD CMakeLists.txt CMakeLists.txt

ADD . /service

WORKDIR /service/utility

RUN ./install-oatpp-modules.sh Release

WORKDIR /service/build

RUN cmake ..
RUN make

EXPOSE 8000 8000

ENTRYPOINT ["./example-yuv-websocket-stream-exe"]
