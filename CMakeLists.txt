cmake_minimum_required(VERSION 3.10)

project(VirtualPhenixEngine)

#---3rd party------------------------------------------------------------------
# Disable assimp extras
set (ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set (ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)

# Disable assimp exports
set (ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)

# Enable specific assimp imports
set (ASSIMP_BUILD_FBX_IMPORTER ON BOOL)
set (ASSIMP_BUILD_OBJ_IMPORTER ON BOOL)
set (ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)

add_subdirectory(vendor/assimp)
#------------------------------------------------------------------------------

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror -std=c++17")
#set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -ggdb -Wweak-vtables -fexceptions -fsanitize=address")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -ggdb -Wweak-vtables -fexceptions")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory (src/)

file(GLOB sourceFiles
  "src/*.hpp"
  "src/*.cpp"
  "src/Managers/*.hpp"
  "src/Managers/*.cpp"
)
add_executable(VPEngine ${sourceFiles})

add_library(stb_image INTERFACE)
target_sources(stb_image INTERFACE ${CMAKE_SOURCE_DIR}/vendor/stb_image.h)
target_include_directories(stb_image INTERFACE vendor)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
include_directories(${Vulkan_INCLUDE_DIR})
include_directories(${glfw3_INCLUDE_DIR})
include_directories("./vendor/assimp/include")
target_link_libraries(VPEngine ${Vulkan_LIBRARIES} ${GLFW3_LIBRARIES} assimp glfw stb_image)
