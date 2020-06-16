cmake_minimum_required(VERSION 3.15)
project(srt_test)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

find_package (Threads REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/srt/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/srt/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cppSRTWrapper/)

add_library(srtnet STATIC ${CMAKE_CURRENT_SOURCE_DIR}/cppSRTWrapper/SRTNet.cpp)
target_link_libraries(srtnet ${CMAKE_CURRENT_SOURCE_DIR}/srt/libsrt.a ${OPENSSL_LIBRARIES} Threads::Threads)


add_executable(srt_sender sender.cpp)
target_link_libraries(srt_sender srtnet)

add_executable(srt_receiver receiver.cpp)
target_link_libraries(srt_receiver srtnet)