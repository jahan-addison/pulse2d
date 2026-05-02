# Teensy 4.1 (NXP i.MX RT1062, ARM Cortex-M7) cross-compilation toolchain.
#
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-teensy41.cmake -B build .
#
# Requires arm-none-eabi-gcc from the Teensyduino bundled toolchain, or a
# separately installed arm-none-eabi toolchain on PATH.

cmake_minimum_required(VERSION 3.16)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# ##############################################################################
# Toolchain binaries
# ##############################################################################

set(TEENSY_TOOLS_PATH
    "/Applications/Arduino.app/Contents/Java/hardware/tools/arm"
    CACHE PATH "Path to the arm-none-eabi toolchain (Teensyduino bundled)")

find_program(
  CMAKE_C_COMPILER
  NAMES arm-none-eabi-gcc
  PATHS "${TEENSY_TOOLS_PATH}/bin"
  DOC "arm-none-eabi C compiler" REQUIRED)
find_program(
  CMAKE_CXX_COMPILER
  NAMES arm-none-eabi-g++
  PATHS "${TEENSY_TOOLS_PATH}/bin"
  DOC "arm-none-eabi C++ compiler" REQUIRED)
find_program(
  CMAKE_ASM_COMPILER
  NAMES arm-none-eabi-gcc
  PATHS "${TEENSY_TOOLS_PATH}/bin"
  DOC "arm-none-eabi assembler" REQUIRED)
find_program(
  CMAKE_AR
  NAMES arm-none-eabi-ar
  PATHS "${TEENSY_TOOLS_PATH}/bin"
  DOC "arm-none-eabi archiver" REQUIRED)
find_program(
  CMAKE_OBJCOPY
  NAMES arm-none-eabi-objcopy
  PATHS "${TEENSY_TOOLS_PATH}/bin"
  DOC "arm-none-eabi objcopy")
find_program(
  CMAKE_SIZE
  NAMES arm-none-eabi-size
  PATHS "${TEENSY_TOOLS_PATH}/bin"
  DOC "arm-none-eabi size")

# Skip compiler sanity-check builds; the bare-metal target has no host sysroot.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ##############################################################################
# Processor flags - Cortex-M7, FPv5-D16, hardware float ABI
# ##############################################################################

set(TEENSY41_MCU_FLAGS
    "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb"
    CACHE STRING "Processor flags for Teensy 4.1 (i.MX RT1062)")

set(CMAKE_C_FLAGS_INIT "${TEENSY41_MCU_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${TEENSY41_MCU_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${TEENSY41_MCU_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${TEENSY41_MCU_FLAGS} -Wl,--gc-sections")
