cmake_minimum_required(VERSION 3.1)
project( demo )

SET(CMAKE_C_COMPILER g++)
add_compile_options(-std=c++11)

#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -march=native -O3 -pthread" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -march=native -O3" )
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(OpenCV 3 REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

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

add_library(Camera SHARED 
    src/CameraUtils.cpp
    src/Camera.cpp
    third_party/jsoncpp/src/jsoncpp.cpp
)
target_link_libraries(Camera ${CAMERALIBS})

add_library(Detection SHARED
    src/Detector.cpp
    src/DetectUtils.cpp
    src/utils.cpp
    third_party/jsoncpp/src/jsoncpp.cpp
)
target_link_libraries(Detection 
    ${CURL_LIBRARY} 
    Camera
)

add_library(Client SHARED
    src/client.cpp
    third_party/jsoncpp/src/jsoncpp.cpp
)
target_link_libraries(Client ${CURL_LIBRARY})

# demo for camera
add_executable(CameraDemo 
    test/camera_demo.cpp
)
target_link_libraries(CameraDemo
    ${CAMERALIBS}
    Camera
)

# demo for detector
add_executable(DetectDemo 
    test/detect_demo.cpp
)
target_link_libraries(DetectDemo
    Threads::Threads
    ${OpenCV_LIBS}
    Detection
)

add_executable(Demo 
    src/demo.cpp
    src/utils.cpp
    src/client.cpp
)
target_link_libraries(Demo
    # ${CAMERALIBS}
    ${OpenCV_LIBS}
    Camera
    Detection
)

add_executable(FileReadDemo
    test/file_demo.cpp
    src/Camera.cpp
    src/CameraUtils.cpp
    third_party/jsoncpp/src/jsoncpp.cpp
)
target_link_libraries(FileReadDemo
    ${CAMERALIBS}
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

AUX_SOURCE_DIRECTORY(src/CppHttpDemo/httpserver httpserver)
add_executable(server
    ${httpserver}
    third_party/mongoose/mongoose.c
    third_party/jsoncpp/src/jsoncpp.cpp
)

AUX_SOURCE_DIRECTORY(src/CppHttpDemo/httpclient httpclient)
add_executable(client_demo
    ${httpclient}
    third_party/mongoose/mongoose.c
)

add_executable(server_demo
    test/server_demo.cpp
    src/http_server.cpp
    src/utils.cpp
    src/client.cpp
    third_party/mongoose/mongoose.c
    third_party/jsoncpp/src/jsoncpp.cpp
)
target_link_libraries(server_demo
    ${CURL_LIBRARY}
    ${OpenCV_LIBS}
    Camera
    Detection
)