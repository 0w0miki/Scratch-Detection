cmake_minimum_required(VERSION 3.1)
project( demo )

SET(CMAKE_C_COMPILER g++)
add_compile_options(-std=c++11)

#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -march=native -O3 -pthread" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -march=native -O3" )
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

if(NOT DEFINED ENV{GENICAM_ROOT_V2_3})
    message(FATAL_ERROR "cannot find genicam root directory")
endif(NOT DEFINED ENV{GENICAM_ROOT_V2_3})

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/third_party/camera/include
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
)

# demo for continuous capture
add_executable(GxAcquireContinuous 
    src/GxAcquireContinuous.cpp
)
target_link_libraries(GxAcquireContinuous
    ${CAMERALIBS}
)

add_executable(GxAcquireContinuousSofttrigger
    src/GxAcquireContinuousSofttrigger.cpp
    src/CameraUtils.cpp
)
target_link_libraries(GxAcquireContinuousSofttrigger
    ${CAMERALIBS}
)

# demo for image callback
add_executable(CallbackDemo 
    src/cam_callback_demo.cpp
    src/CameraUtils.cpp
    src/Camera.cpp
)
target_link_libraries(CallbackDemo
    ${CAMERALIBS}
)