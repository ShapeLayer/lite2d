cmake_minimum_required(VERSION 3.16)
project(lite2d LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
find_package(glm REQUIRED)

set(ONNXRUNTIME_DIR external/onnxruntime)
if (EXISTS "${ONNXRUNTIME_DIR}/include/onnxruntime_cxx_api.h")
  set(USE_ONNX TRUE)
  add_definitions(-DUSE_ONNX)
  message(STATUS "Found ONNX Runtime at ${ONNXRUNTIME_DIR}")
endif()

set(LIB_HEADERS
  src/debug.h
  src/deformer.h
  src/easing.h
  src/engine.h
  src/anim_clip.h
  src/expression.h
  src/model.h
  src/spring.h
  src/texture.h
  src/glmesh.h
  src/shader.h
  src/model_loader.h
  external/stb/stb_image.h
  external/commons/json.hpp
)
set(LIB_SOURCES
  src/anim_clip.cc
  src/glmesh.cc
  src/engine.cc
  src/shader.cc
  src/texture.cc
  src/model_loader.cc
  external/glad/src/glad.c
)

add_library(lite2d STATIC ${LIB_HEADERS} ${LIB_SOURCES})
add_executable(lite2d_viewer src/main.cc)

set_source_files_properties(external/glad/src/glad.c PROPERTIES LANGUAGE C)

target_include_directories(lite2d PUBLIC
  ${OpenCV_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  ${OPENGL_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/external
  ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
)

if(APPLE)
  target_include_directories(lite2d PUBLIC /opt/homebrew/include)
  target_link_directories(lite2d PUBLIC /opt/homebrew/lib)
endif()

target_link_directories(lite2d PUBLIC ${GLFW_LIBRARY_DIRS})

target_link_libraries(lite2d PUBLIC
  glm::glm
  ${OpenCV_LIBS}
  ${OPENGL_gl_LIBRARY}
)

target_link_libraries(lite2d_viewer PRIVATE
  lite2d
  ${GLFW_LIBRARIES}
)

if (USE_ONNX)
  target_include_directories(lite2d PUBLIC ${ONNXRUNTIME_DIR}/include)
  target_link_directories(lite2d PUBLIC ${ONNXRUNTIME_DIR}/lib)
  target_link_libraries(lite2d PUBLIC onnxruntime)
endif()

if (APPLE)
  target_link_libraries(lite2d PUBLIC 
    "-framework OpenGL"
    "-framework Cocoa" 
    "-framework IOKit" 
    "-framework CoreVideo"
    "-framework QuartzCore"
  )
endif()
