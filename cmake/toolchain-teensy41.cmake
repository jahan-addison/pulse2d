cmake_minimum_required(VERSION 3.16)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# ##############################################################################
# Toolchain binaries
# ##############################################################################

find_program(
  CMAKE_C_COMPILER
  NAMES arm-none-eabi-gcc
  DOC "arm-none-eabi C compiler" REQUIRED)
find_program(
  CMAKE_CXX_COMPILER
  NAMES arm-none-eabi-g++
  DOC "arm-none-eabi C++ compiler" REQUIRED)
find_program(
  CMAKE_ASM_COMPILER
  NAMES arm-none-eabi-gcc
  DOC "arm-none-eabi assembler" REQUIRED)
find_program(
  CMAKE_AR
  NAMES arm-none-eabi-ar
  DOC "arm-none-eabi archiver" REQUIRED)
find_program(
  CMAKE_OBJCOPY
  NAMES arm-none-eabi-objcopy
  DOC "arm-none-eabi objcopy")
find_program(
  CMAKE_SIZE
  NAMES arm-none-eabi-size
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
