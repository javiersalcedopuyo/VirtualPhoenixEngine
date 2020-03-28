cmake_minimum_required(VERSION 3.10)

project(VirtualPhenixEngine)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror -std=c++17")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory (src/)
# TODO: Move the device management into its own class
#add_subdirectory (src/Managers)

file(GLOB sourceFiles
  "src/*.hpp"
  "src/*.cpp"
  # TODO: Move the device management into its own class
  #"src/Managers/*.hpp"
  #"src/Managers/*.cpp"
)
add_executable(VPEngine ${sourceFiles})

add_library(stb_image INTERFACE)
target_sources(stb_image INTERFACE ${CMAKE_SOURCE_DIR}/include/stb_image.h)
target_include_directories(stb_image INTERFACE include)

add_library(tiny_obj INTERFACE)
target_sources(tiny_obj INTERFACE ${CMAKE_SOURCE_DIR}/include/tiny_obj_loader.h)
target_include_directories(tiny_obj INTERFACE include)

find_package(Vulkan REQUIRED)
find_package (glfw3 REQUIRED)
include_directories(${Vulkan_INCLUDE_DIR})
include_directories(${glfw3_INCLUDE_DIR})
target_link_libraries(VPEngine ${Vulkan_LIBRARIES} ${GLFW3_LIBRARIES} glfw stb_image tiny_obj)