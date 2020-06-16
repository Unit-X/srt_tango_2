cmake_minimum_required(VERSION 3.15)
project(srt_test)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

find_package (Threads REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/srt/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/srt/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cppSRTWrapper/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/efp/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mpegts/mpegts/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/restapi/)

add_library(srtnet STATIC ${CMAKE_CURRENT_SOURCE_DIR}/cppSRTWrapper/SRTNet.cpp)
target_link_libraries(srtnet ${CMAKE_CURRENT_SOURCE_DIR}/srt/libsrt.a ${OPENSSL_LIBRARIES} Threads::Threads)

file(GLOB libfiles ${CMAKE_CURRENT_SOURCE_DIR}/mpegts/mpegts/*.cpp)
add_library(mpegts STATIC ${libfiles} ${CMAKE_CURRENT_SOURCE_DIR}/amf0/amf0/amf0.cpp)

add_executable(srt_sender client.cpp ${CMAKE_CURRENT_SOURCE_DIR}/efp/ElasticFrameProtocol.cpp ${CMAKE_CURRENT_SOURCE_DIR}/restapi/RESTInterface.cpp)
target_link_libraries(srt_sender srtnet mpegts)

add_executable(srt_receiver receiver.cpp ${CMAKE_CURRENT_SOURCE_DIR}/efp/ElasticFrameProtocol.cpp  ${CMAKE_CURRENT_SOURCE_DIR}/restapi/RESTInterface.cpp)
target_link_libraries(srt_receiver srtnet)