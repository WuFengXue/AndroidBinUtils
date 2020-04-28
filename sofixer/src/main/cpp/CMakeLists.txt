# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.4.1)

project(SoFixer)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add.library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror")
# enable __SO64__ for 64bit so..
#add_definitions("-D__SO64__")

aux_source_directory(. ROOT_SRC)

#if(SLibrary)
#    add_library(SoFixer STATIC ${ROOT_SRC})
#else()
add_executable(SoFixer ${ROOT_SRC})
#endif()