cmake_minimum_required(VERSION 3.1)
project( demo )

SET(CMAKE_C_COMPILER g++)
add_compile_options(-std=c++11)

#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -march=native -O3 -pthread" )
SET(CMAKE_BUILD_TYPE "Release") 
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -std=c++11 -march=native -O3" )
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -std=c++11 -O0 -Wall -g -ggdb")
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
message(${CMAKE_MODULE_PATH})
find_package(OpenCV 3 REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

message(${OpenCV_LIBS})

if(NOT DEFINED ENV{GENICAM_ROOT_V2_3})
    message(FATAL_ERROR "cannot find genicam root directory")
endif(NOT DEFINED ENV{GENICAM_ROOT_V2_3})

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/third_party/camera/include
    ${PROJECT_SOURCE_DIR}/third_party/jsoncpp
    ${PROJECT_SOURCE_DIR}/third_party/mongoose
    ${CURL_INCLUDE_DIR}
    # $ENV{GENICAM_ROOT_V2_3}/library/CPP/include
    # $ENV{GENICAM_ROOT_V2_3}/../../sdk/include
)

link_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/third_party/camera/lib
    # ${PROJECT_SOURCE_DIR}/lib
    # $ENV{GENICAM_ROOT_V2_3}/bin/Linux64_x64
    # $ENV{GENICAM_ROOT_V2_3}/bin/Linux64_x64/GenApi/Generic
)

set(CAMERALIBS
    libgxiapi.so
    Threads::Threads
    libdximageproc.so
    libGCBase_gcc40_v2_3.so
    libGenApi_gcc40_v2_3.so
    liblog4cpp_gcc40_v2_3.so
    libLog_gcc40_v2_3.so
    libMathParser_gcc40_v2_3.so
    ${OpenCV_LIBS}
)

add_library(Log SHARED
    src/Log.cpp
)
target_link_libraries(Log
    Threads::Threads
)

add_library(Camera SHARED 
    src/camera/CameraUtils.cpp
    src/camera/Camera.cpp
    src/utils.cpp
    third_party/jsoncpp/src/jsoncpp.cpp
)
target_link_libraries(Camera 
    ${CAMERALIBS}
    Log
)

add_library(Detection SHARED
    src/detection/Detector.cpp
    src/detection/DetectUtils.cpp
    src/utils.cpp
    src/Serial.cpp
    third_party/jsoncpp/src/jsoncpp.cpp
)
target_link_libraries(Detection 
    ${CURL_LIBRARY} 
    Camera
    Log
)

add_library(Client SHARED
    src/web/http_client.cpp
    third_party/jsoncpp/src/jsoncpp.cpp
)
target_link_libraries(Client 
    ${CURL_LIBRARY}
    Log
)

add_library(Websocket SHARED
    src/web/Websocket_server.cpp
    third_party/mongoose/mongoose.c
)

add_library(System SHARED
    src/System.cpp
    src/utils.cpp
    src/Serial.cpp
    src/web/Websocket_server.cpp
    third_party/mongoose/mongoose.c
    third_party/jsoncpp/src/jsoncpp.cpp
)
target_link_libraries(System
    ${CURL_LIBRARY}
    ${OpenCV_LIBS}
    Camera
    Detection
    Client
    Log
)

# NOTE RELEASE
add_executable(server_demo
    test/server_demo.cpp
    src/web/http_server.cpp
    src/utils.cpp
    src/Serial.cpp
    third_party/mongoose/mongoose.c
    third_party/jsoncpp/src/jsoncpp.cpp
)
target_link_libraries(server_demo
    ${CURL_LIBRARY}
    ${OpenCV_LIBS}
    Camera
    Detection
    Client
    Log
)

add_executable(color
    src/color.cpp
    src/web/http_server.cpp
    src/utils.cpp
    src/Serial.cpp
    third_party/mongoose/mongoose.c
    third_party/jsoncpp/src/jsoncpp.cpp
)
target_link_libraries(color
    ${CURL_LIBRARY}
    ${OpenCV_LIBS}
    Camera
    Detection
    Client
    Log
)

# NOTE TEST
add_executable(mono_demo
    test/mono_demo.cpp
    src/web/http_server.cpp
    src/utils.cpp
    src/Serial.cpp
    third_party/mongoose/mongoose.c
    third_party/jsoncpp/src/jsoncpp.cpp
)
target_link_libraries(mono_demo
    ${CURL_LIBRARY}
    ${OpenCV_LIBS}
    Camera
    Detection
    Client
    Log
)

add_executable(serial_demo
    test/serial.cpp
    src/Serial.cpp
)

add_executable(CameraDemo 
    test/camera_demo.cpp
)
target_link_libraries(CameraDemo
    ${CAMERALIBS}
    Camera
)

add_executable(DetectDemo 
    test/detect_demo.cpp
)
target_link_libraries(DetectDemo
    Threads::Threads
    ${OpenCV_LIBS}
    Detection
)

add_executable(url_demo
    test/curl_demo.cpp
    third_party/jsoncpp/src/jsoncpp.cpp
)
target_link_libraries(url_demo
    ${CURL_LIBRARY}
    Client
    Threads::Threads
)

add_executable(system_test
    test/system_test.cpp
)
target_link_libraries(system_test
    System
)

add_executable(logtest
    test/log_test.cpp
)
target_link_libraries(logtest
    Log
)

add_executable(websockettest
    test/websockettest.cpp
)
target_link_libraries(websockettest
    Websocket
    Threads::Threads
    Log
)