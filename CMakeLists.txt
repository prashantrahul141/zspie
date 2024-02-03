cmake_minimum_required(VERSION 3.22)

# default build type will be Debug, if nothing is specified.
set(default_build_type "Debug")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")

  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)

  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Debug only flags
set(CMAKE_C_FLAGS_DEBUG_INIT "")

# release only flags
set(CMAKE_C_FLAGS_RELEASE_INIT "-s")

# project name.
project(zspie)

# Set C standard
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# all source files.
file(GLOB SOURCES src/external/log.c src/chunk.c src/memory.c src/debug.c src/value.c src/vm.c src/main.c)

# include dir
include_directories(${PROJECT_NAME} PRIVATE src/ src/external/)

# macros
add_compile_definitions(LOG_USE_COLOR)

# compiler flags
if(MSVC)
  # msvc related flags
  add_compile_options(/W4 /WX)
else()
  # gcc and clang 
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# linking libs
target_link_libraries(${PROJECT_NAME} PRIVATE)
