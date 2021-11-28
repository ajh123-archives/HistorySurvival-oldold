
cmake_minimum_required(VERSION 3.20)
project(
    game
    LANGUAGES CXX
)


if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please run \"cmake ..\" from the build directory. You may need to delete \"${CMAKE_SOURCE_DIR}/CMakeCache.txt\" first.")
endif()

# Requires C11 standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

# Set this to the minimal version you want to support
find_package(raylib 3.0 QUIET) # Let CMake search for a raylib-config.cmake

# You could change the QUIET above to REQUIRED and remove this if() clause
# This part downloads raylib and builds it if it's not installed on your system
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)

  FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/master.tar.gz
  )

  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raylib)

    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples

    # build raylib
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})

  endif()

endif()


include(FetchContent)
FetchContent_Declare(
  asiocmake
  GIT_REPOSITORY "https://github.com/OlivierLDff/asio.cmake"
  GIT_TAG        "main"
)
FetchContent_MakeAvailable(asiocmake)

FetchContent_Declare(
  argparse
  GIT_REPOSITORY "https://github.com/p-ranav/argparse"
  GIT_TAG        "master"
)
FetchContent_MakeAvailable(argparse)


add_subdirectory(${CMAKE_SOURCE_DIR}/deps/entt)

# This is the main part:

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src
)
file(GLOB all_SRCS
  "${PROJECT_SOURCE_DIR}/include/*.h"
  "${PROJECT_SOURCE_DIR}/include/*.hpp"
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/*.c"
)

option(BUILD_SERVER "Build the server" OFF)
if(NOT BUILD_SERVER)
    add_definitions(-Dgame_type=1)
    add_executable(${PROJECT_NAME} ${all_SRCS})
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "game-client")
endif()
if(BUILD_SERVER)
    add_definitions(-Dgame_type=2)
    add_executable(${PROJECT_NAME} ${all_SRCS})
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "game-server")
endif()


#set(raylib_VERBOSE 1)
target_link_libraries(${PROJECT_NAME} raylib EnTT::EnTT asio::asio argparse::argparse)

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit -framework Cocoa -framework OpenGL")
endif()

# That's it! You should have an example executable that you can run. Have fun!
