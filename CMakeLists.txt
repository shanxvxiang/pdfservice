cmake_minimum_required (VERSION 2.8)
project (pdfservice)
# set(CMAKE_CXX_FLAGS "-std=c++14 -rdynamic -g -pthread -Wno-deprecated")
set(CMAKE_CXX_FLAGS "-std=c++14 -pthread -O2 -Wno-deprecated")
#set(CMAKE_CXX_FLAGS "-pthread -O2 -Wno-deprecated")

set(EXECUTABLE_OUTPUT_PATH ./bin)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/third/libfastcommon/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/third/fastdfs/common)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/third/fastdfs/client)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/third/fastdfs/tracker)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/third/fastdfs/storage)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/third/libjpeg-turbo)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/third/libfastcommon/src)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/third/fastdfs/client)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/third/libjpeg-turbo)

add_executable(pdfservice service.cpp)
target_link_libraries(pdfservice turbojpeg fdfsclient fastcommon pdfium dl rt m)


