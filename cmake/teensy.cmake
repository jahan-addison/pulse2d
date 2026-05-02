# ##############################################################################
# Teensy 4.1 - Library Paths and Target Configuration
# ##############################################################################
#
# Resolves paths to teensy4_core, ILI9341_t3, ILI9341_t3n, SdFat, and the Teensy
# Audio Library from a Teensyduino installation.
#
# Cache variables (override with -D on the cmake command line): TEENSYDUINO_PATH
# - root of the Teensyduino hardware directory TEENSY41_CORE_PATH     -
# teensy4_core source tree TEENSY41_LIB_PATH      - Teensyduino bundled
# libraries directory TEENSY41_ILI9341_T3_PATH TEENSY41_ILI9341_T3N_PATH
# TEENSY41_SDFAT_PATH TEENSY41_AUDIO_PATH
#
# Provides: luya_add_teensy_target(<target>) - configure a CMake target for
# Teensy 4.1

# ##############################################################################
# Installation paths
# ##############################################################################

set(TEENSYDUINO_PATH
    "/Applications/Arduino.app/Contents/Java/hardware/teensy"
    CACHE PATH "Root of the Teensyduino hardware directory")

set(TEENSY41_CORE_PATH
    "${TEENSYDUINO_PATH}/avr/cores/teensy4"
    CACHE PATH "teensy4_core source files")

set(TEENSY41_LIB_PATH
    "${TEENSYDUINO_PATH}/avr/libraries"
    CACHE PATH "Teensyduino bundled libraries directory")

set(TEENSY41_ILI9341_T3_PATH
    "${TEENSY41_LIB_PATH}/ILI9341_t3"
    CACHE PATH "ILI9341_t3 library path")

set(TEENSY41_ILI9341_T3N_PATH
    "${TEENSY41_LIB_PATH}/ILI9341_t3n"
    CACHE PATH "ILI9341_t3n library path (with DMA support)")

set(TEENSY41_SDFAT_PATH
    "${TEENSY41_LIB_PATH}/SdFat/src"
    CACHE PATH "SdFat library path")

set(TEENSY41_AUDIO_PATH
    "${TEENSY41_LIB_PATH}/Audio"
    CACHE PATH "Teensy Audio Library path")

if(NOT EXISTS "${TEENSY41_CORE_PATH}")
  message(
    WARNING "Teensy 4.1 core not found at: ${TEENSY41_CORE_PATH}\n"
            "Set -DTEENSYDUINO_PATH=<path> to your Teensyduino installation.")
endif()

# ##############################################################################
# luya_add_teensy_target
# ##############################################################################
#
# Configure <target> for Teensy 4.1: - Applies __IMXRT1062__, F_CPU, ARDUINO,
# and TEENSYDUINO definitions - Adds core and library include paths - Optionally
# defines LUYA_USE_ILI9341_T3N when USE_ILI9341_T3N is set
#
function(luya_add_teensy_target target)
  target_compile_definitions(
    ${target} PUBLIC __IMXRT1062__ TEENSYDUINO=159 ARDUINO=10824
                     F_CPU=600000000 USB_SERIAL LAYOUT_US_ENGLISH)

  target_include_directories(
    ${target}
    PUBLIC "${TEENSY41_CORE_PATH}" "${TEENSY41_ILI9341_T3_PATH}"
           "${TEENSY41_ILI9341_T3N_PATH}" "${TEENSY41_SDFAT_PATH}"
           "${TEENSY41_AUDIO_PATH}")

  if(USE_ILI9341_T3N)
    target_compile_definitions(${target} PUBLIC LUYA_USE_ILI9341_T3N)
    message(STATUS "luya: using ILI9341_t3n (DMA) display driver")
  else()
    message(STATUS "luya: using ILI9341_t3 display driver")
  endif()
endfunction()
