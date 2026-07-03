cmake_minimum_required(VERSION 3.16...3.29)

set(CMAKE_POLICY_VERSION_MINIMUM 3.10)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(
    FATAL_ERROR
      "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source."
  )
endif()

if(CMAKE_CROSSCOMPILING)
  project(
    pulse2d
    VERSION 1.1.0
    LANGUAGES C CXX ASM)
else()
  project(
    pulse2d
    VERSION 1.1.0
    LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(GNUInstallDirs)
include(CMakeDependentOption)
include(cmake/CPM.cmake)

include(GNUInstallDirs)

# ##############################################################################
# DEPENDENCIES
# ##############################################################################

if(NOT CMAKE_CROSSCOMPILING)
  option(PULSE2D_USE_SDL2 "" ON)
else()
  option(PULSE2D_USE_SDL2 "" OFF)
endif()

cpmaddpackage("gh:ETLCPP/etl#20.47.1")
cpmaddpackage("gh:boost-ext/sml#v1.2.0")

if(NOT CMAKE_CROSSCOMPILING)
  cpmaddpackage("gh:StableCoder/cmake-scripts#24.04")
  cpmaddpackage("gh:TheLartians/PackageProject.cmake@1.8.0")
  cpmaddpackage("gh:doctest/doctest@2.4.12")

  include(${cmake-scripts_SOURCE_DIR}/tools.cmake)

  if(PULSE2D_USE_SDL2)
    find_package(SDL2 CONFIG QUIET)
    if(SDL2_FOUND)
      set(PULSE2D_SDL2_TARGET SDL2::SDL2)
    else()
      find_package(PkgConfig REQUIRED)
      pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
      set(PULSE2D_SDL2_TARGET PkgConfig::SDL2)
    endif()
    get_target_property(_sdl2_incs ${PULSE2D_SDL2_TARGET}
                        INTERFACE_INCLUDE_DIRECTORIES)
    foreach(_inc IN LISTS _sdl2_incs)
      if(_inc MATCHES "SDL2$")
        get_filename_component(_parent "${_inc}" DIRECTORY)
        set_property(
          TARGET ${PULSE2D_SDL2_TARGET}
          APPEND
          PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${_parent}")
      endif()
    endforeach()
  endif()

  cpmaddpackage(
    NAME
    stb
    GITHUB_REPOSITORY
    nothings/stb
    GIT_TAG
    master
    DOWNLOAD_ONLY
    YES)
endif()

# ##############################################################################
# BUILD OPTIONS
# ##############################################################################

if(MSVC)
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} /Zc:__cplusplus /utf-8 /DEBUG /D_CRT_SECURE_NO_DEPRECATE /W4 /WX"
  )
else()
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -Werror -Wpedantic -DDEBUG")
  else()
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wpedantic -O2 -march=native")
  endif()
endif()

# Enable sanitizer with `-DUSE_SANITIZER=<VALUE>` argument to CMake Address,
# Memory, MemoryWithOrigins, Undefined, Thread, Leak, 'Address;Undefined'
if(USE_SANITIZER)
  include(${cmake-scripts_SOURCE_DIR}/sanitizers.cmake)
endif()

# Include-what-you-use https://include-what-you-use.org/
if(IWYU)
  find_program(IWYU_EXECUTABLE include-what-you-use)
  if(IWYU_EXECUTABLE)
    if(APPLE)
      execute_process(
        COMMAND xcrun --sdk macosx --show-sdk-path
        OUTPUT_VARIABLE _macos_sdk_path
        OUTPUT_STRIP_TRAILING_WHITESPACE)
      set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE
          "${IWYU_EXECUTABLE};-isysroot;${_macos_sdk_path}")
    else()
      set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU_EXECUTABLE}")
    endif()
    message(STATUS "IWYU enabled for C++ targets.")
  else()
    message(
      WARNING "Include-What-You-Use executable not found. IWYU will not be run."
    )
  endif()
endif()

# ##############################################################################
# Executables
# ##############################################################################

file(GLOB_RECURSE headers CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/*.cc")
file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cc")

if(NOT PULSE2D_USE_SDL2)
  list(REMOVE_ITEM sources
       "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/display.cc")
endif()

add_library(libpulse2d STATIC ${sources} ${headers})
add_library(pulse2d::pulse2d ALIAS libpulse2d)
set_target_properties(libpulse2d PROPERTIES OUTPUT_NAME "pulse2d")
target_compile_definitions(libpulse2d PUBLIC ETL_LOG_ERRORS)

target_include_directories(
  libpulse2d PUBLIC $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}>
                    $<INSTALL_INTERFACE:${PROJECT_NAME}-${PROJECT_VERSION}>)

target_link_libraries(libpulse2d PUBLIC etl)

if(stb_ADDED)
  target_include_directories(libpulse2d PRIVATE "${stb_SOURCE_DIR}")
endif()

if(sml_ADDED)
  target_include_directories(libpulse2d SYSTEM
                             PUBLIC "${sml_SOURCE_DIR}/include")
endif()

if(PULSE2D_USE_SDL2)
  target_compile_definitions(libpulse2d PUBLIC PULSE2D_USE_SDL2)
  target_link_libraries(libpulse2d PUBLIC ${PULSE2D_SDL2_TARGET})
endif()

include(cmake/doctest.cmake)
