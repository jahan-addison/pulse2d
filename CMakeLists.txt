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
    luya
    VERSION 0.0.1
    LANGUAGES C CXX ASM)
else()
  project(
    luya
    VERSION 0.0.1
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

# LUYA_USE_SDL2 defaults ON for host builds; pass -DLUYA_USE_SDL2=OFF to disable
# (e.g. when targeting Teensy via a host compiler for unit tests).
if(NOT CMAKE_CROSSCOMPILING)
  option(LUYA_USE_SDL2 "Use SDL2 display (default for host builds)" ON)
else()
  option(LUYA_USE_SDL2 "Use SDL2 display (default for host builds)" OFF)
endif()

cpmaddpackage("gh:ETLCPP/etl#20.47.1")

if(NOT CMAKE_CROSSCOMPILING)
  cpmaddpackage("gh:StableCoder/cmake-scripts#24.04")
  cpmaddpackage("gh:TheLartians/PackageProject.cmake@1.8.0")
  cpmaddpackage("gh:doctest/doctest@2.4.12")

  include(${cmake-scripts_SOURCE_DIR}/tools.cmake)

  if(LUYA_USE_SDL2)
    find_package(SDL2 CONFIG QUIET)
    if(SDL2_FOUND)
      set(LUYA_SDL2_TARGET SDL2::SDL2)
      message(STATUS "SDL2 found via CMake config: ${SDL2_VERSION}")
    else()
      find_package(PkgConfig REQUIRED)
      pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
      set(LUYA_SDL2_TARGET PkgConfig::SDL2)
      message(STATUS "SDL2 found via pkg-config: ${SDL2_VERSION}")
    endif()
    get_target_property(_sdl2_incs ${LUYA_SDL2_TARGET}
                        INTERFACE_INCLUDE_DIRECTORIES)
    foreach(_inc IN LISTS _sdl2_incs)
      if(_inc MATCHES "SDL2$")
        get_filename_component(_parent "${_inc}" DIRECTORY)
        set_property(
          TARGET ${LUYA_SDL2_TARGET}
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
elseif(CMAKE_CROSSCOMPILING)
  # Cross-compiling for Teensy 4.1: MCU flags come from toolchain-teensy41.cmake
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -Wpedantic -DDEBUG")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -O2")
  endif()
else()
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -Werror -Wpedantic -DDEBUG")
  else()
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wpedantic -O2 -march=native")
  endif()
endif()

if(NOT CMAKE_CROSSCOMPILING)
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
        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE
            "${IWYU_EXECUTABLE};;-isysroot $(xcrun --sdk macosx --show-sdk-path)"
        )
      else()
        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU_EXECUTABLE};;")
      endif()
      message(STATUS "IWYU enabled for C++ targets.")
    else()
      message(
        WARNING
          "Include-What-You-Use executable not found. IWYU will not be run.")
    endif()
  endif()
endif()

# ##############################################################################
# Executables
# ##############################################################################

file(GLOB_RECURSE headers CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/*.cc")
file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cc")

# display.cc contains both SDL2 and ILI9341 implementations under #ifdef;
# exclude it when cross-compiling since the Teensy build compiles sources
# directly.
if(CMAKE_CROSSCOMPILING OR NOT LUYA_USE_SDL2)
  list(REMOVE_ITEM sources
       "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/display.cc")
endif()

# Engine as a static library - games link against luya::luya
if(NOT CMAKE_CROSSCOMPILING)
  add_library(libluya STATIC ${sources} ${headers})
  add_library(luya::luya ALIAS libluya)
  set_target_properties(libluya PROPERTIES OUTPUT_NAME "luya")
  target_compile_definitions(libluya PUBLIC ETL_LOG_ERRORS)

  target_include_directories(
    libluya
    PUBLIC $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:${PROJECT_NAME}-${PROJECT_VERSION}>)

  target_link_libraries(libluya PUBLIC etl)
endif()

if(CMAKE_CROSSCOMPILING)
  # Teensy 4.1 build is handled entirely by cmake/build-teensy.sh Use `cmake
  # --build build-teensy`
  if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(_teensy_default_hw
        "$ENV{HOME}/Library/Arduino15/packages/teensy/hardware/avr/1.60.0")
  else()
    set(_teensy_default_hw
        "$ENV{HOME}/.arduino15/packages/teensy/hardware/avr/1.60.0")
  endif()
  set(TEENSY41_HW
      "${_teensy_default_hw}"
      CACHE
        PATH
        "Root of the Teensyduino hardware directory (contains cores/, libraries/)"
  )
  find_program(BASH_EXECUTABLE bash REQUIRED)
  add_custom_target(
    luya ALL
    COMMAND
      env LUYA_ETL_INCLUDE=${etl_SOURCE_DIR}/include TEENSY41_HW=${TEENSY41_HW}
      ${BASH_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/build-teensy.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Teensy 4.1 firmware via cmake/build-teensy.sh"
    VERBATIM)
else()
  if(stb_ADDED)
    target_include_directories(libluya PRIVATE "${stb_SOURCE_DIR}")
  endif()

  if(LUYA_USE_SDL2)
    target_compile_definitions(libluya PUBLIC LUYA_USE_SDL2)
    target_link_libraries(libluya PUBLIC ${LUYA_SDL2_TARGET})
    message(STATUS "luya: SDL2 display backend enabled")
  endif()

  # Sample executable - demonstrates the engine with sprites + physics
  add_executable(sample "${CMAKE_CURRENT_SOURCE_DIR}/sample/sample_game.cc")
  target_link_libraries(sample PRIVATE libluya)
  set_target_properties(sample PROPERTIES OUTPUT_NAME "sample_game")
  # Enforce Teensy-like constraints on the host build
  if(LUYA_TEENSY_CONSTRAINTS)
    message(STATUS "Enabling Teensy-like constraints on host build")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      target_compile_options(sample PRIVATE -Wstack-usage=256 -fstack-usage)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(sample PRIVATE -Wframe-larger-than=256
                                            -Wno-error=frame-larger-than)
    endif()
  endif()
  add_custom_command(
    TARGET sample
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/sample/images"
      "${CMAKE_BINARY_DIR}/sample/images"
    COMMENT "Syncing sample/images to build directory")

  include(cmake/doctest.cmake)
endif()
