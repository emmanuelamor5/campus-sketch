cmake_minimum_required(VERSION 3.16)
project(campus_renderer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

add_executable(campus
  src/main.cpp
  src/canvas.cpp
  src/raster.cpp
  src/clipping.cpp
  src/scene.cpp
)
target_include_directories(campus PRIVATE include)
