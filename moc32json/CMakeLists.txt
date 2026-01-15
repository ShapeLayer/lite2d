cmake_minimum_required(VERSION 3.20)
project(moc32json LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIVE2D_CORE_DIR "${CMAKE_SOURCE_DIR}/external/CubismNativeFramework/Core" CACHE PATH "Path to Live2D Cubism Core SDK (contains include/ and lib/)")

if(NOT LIVE2D_CORE_DIR OR LIVE2D_CORE_DIR STREQUAL "")
  message(FATAL_ERROR "LIVE2D_CORE_DIR is not set. Please download the Live2D Cubism Core SDK from https://www.live2d.com/download/cubism-sdk/download-native/ and set LIVE2D_CORE_DIR to the extracted directory containing include/ and lib/.")
endif()

find_path(LIVE2D_CORE_INCLUDE Live2DCubismCore.h
  PATHS
    "${LIVE2D_CORE_DIR}/include"
    "${LIVE2D_CORE_DIR}"
  NO_DEFAULT_PATH)
find_library(LIVE2D_CORE_LIB
  NAMES Live2DCubismCore libLive2DCubismCore
  PATHS
    "${LIVE2D_CORE_DIR}/lib"
    "${LIVE2D_CORE_DIR}/lib/macos"
    "${LIVE2D_CORE_DIR}/lib/macos/arm64"
    "${LIVE2D_CORE_DIR}/lib/macos/x86_64"
    "${LIVE2D_CORE_DIR}/lib/macOS"
    "${LIVE2D_CORE_DIR}"
  NO_DEFAULT_PATH)

if(NOT LIVE2D_CORE_INCLUDE OR NOT LIVE2D_CORE_LIB)
  message(FATAL_ERROR "Live2D Cubism Core not found at ${LIVE2D_CORE_DIR}. Ensure the SDK has include/Live2DCubismCore.h and lib/libLive2DCubismCore (or similar). Download from https://www.live2d.com/download/cubism-sdk/download-native/")
endif()

file(GLOB CUBISM_BASE
  external/CubismNativeFramework/Framework/src/*.cpp)
file(GLOB CUBISM_EFFECT
  external/CubismNativeFramework/Framework/src/Effect/*.cpp)
file(GLOB CUBISM_ID
  external/CubismNativeFramework/Framework/src/Id/*.cpp)
file(GLOB CUBISM_MATH
  external/CubismNativeFramework/Framework/src/Math/*.cpp)
file(GLOB CUBISM_MODEL
  external/CubismNativeFramework/Framework/src/Model/*.cpp)
file(GLOB CUBISM_MOTION
  external/CubismNativeFramework/Framework/src/Motion/*.cpp)
file(GLOB CUBISM_PHYSICS
  external/CubismNativeFramework/Framework/src/Physics/*.cpp)
file(GLOB CUBISM_TYPE
  external/CubismNativeFramework/Framework/src/Type/*.cpp)
file(GLOB CUBISM_UTILS
  external/CubismNativeFramework/Framework/src/Utils/*.cpp)

set(CUBISM_RENDER_BASE
  external/CubismNativeFramework/Framework/src/Rendering/CubismRenderer.cpp)

add_library(CUBISM_FRAMEWORK STATIC
  ${CUBISM_BASE}
  ${CUBISM_EFFECT}
  ${CUBISM_ID}
  ${CUBISM_MATH}
  ${CUBISM_MODEL}
  ${CUBISM_MOTION}
  ${CUBISM_PHYSICS}
  ${CUBISM_TYPE}
  ${CUBISM_UTILS}
  ${CUBISM_RENDER_BASE})

target_include_directories(CUBISM_FRAMEWORK
  PUBLIC
    ${CMAKE_SOURCE_DIR}/external/CubismNativeFramework/Framework/src
    ${LIVE2D_CORE_INCLUDE})

set(HEADERS
  src/csm_allocate.h
  src/csm_serialize.h
  src/debug.h
  src/fio.h
  external/commons/json.hpp
)
set(SOURCES
  src/csm_allocate.cc
  src/csm_serialize.cc
  src/fio.cc
  src/main.cc
)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
  external/commons)
target_link_libraries(${PROJECT_NAME} PRIVATE CUBISM_FRAMEWORK ${LIVE2D_CORE_LIB})
