cmake_minimum_required(VERSION 3.10)
project(barkanoid)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_COMPILER /usr/bin/g++)

find_library(LIB_SFML_SYSTEM sfml-system)
find_library(LIB_SFML_WINDOW sfml-window)
find_library(LIB_SFML_GRAPHICS sfml-graphics)
find_library(LIB_SFML_AUDIO sfml-audio)
include_directories("${PROJECT_SOURCE_DIR}/include" "/usr/include/SFML")

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(barkanoid ${SRC_FILES})
target_link_libraries(barkanoid ${LIB_SFML_SYSTEM} ${LIB_SFML_WINDOW} ${LIB_SFML_GRAPHICS} ${LIB_SFML_AUDIO})