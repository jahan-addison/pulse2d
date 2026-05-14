# ##############################################################################
# Resolve Teensyduino hardware root
# ##############################################################################

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

if(NOT IS_DIRECTORY "${TEENSY41_HW}")
  message(
    FATAL_ERROR
      "Teensyduino hardware not found at: ${TEENSY41_HW}\n"
      "Install via Arduino board manager or set TEENSY41_HW=/path/to/hardware/avr/1.60.0"
  )
endif()

set(TEENSY41_CORE "${TEENSY41_HW}/cores/teensy4")
set(TEENSY41_LIBS "${TEENSY41_HW}/libraries")

# ##############################################################################
# Interface target — defines and system includes shared by all Teensy libraries
# ##############################################################################

add_library(teensy_defs INTERFACE)

target_compile_definitions(
  teensy_defs
  INTERFACE __IMXRT1062__
            F_CPU=600000000
            TEENSYDUINO=160
            DEBUG
            ARDUINO=10819
            ARDUINO_TEENSY41
            USB_SERIAL
            LAYOUT_US_ENGLISH
            ETL_LOG_ERRORS)

target_include_directories(
  teensy_defs SYSTEM
  INTERFACE "${TEENSY41_CORE}"
            "${TEENSY41_LIBS}/ILI9341_t3"
            "${TEENSY41_LIBS}/SdFat/src"
            "${TEENSY41_LIBS}/Audio"
            "${TEENSY41_LIBS}/SPI"
            "${TEENSY41_LIBS}/SD/src"
            "${TEENSY41_LIBS}/SerialFlash"
            "${TEENSY41_LIBS}/Wire"
            "${etl_SOURCE_DIR}/include")

# ##############################################################################
# Teensy core
# ##############################################################################

file(GLOB _core_cpp "${TEENSY41_CORE}/*.cpp")
file(GLOB _core_c "${TEENSY41_CORE}/*.c")
file(GLOB _core_S "${TEENSY41_CORE}/*.S")

add_library(teensy_core OBJECT ${_core_cpp} ${_core_c} ${_core_S})
target_link_libraries(teensy_core PUBLIC teensy_defs)
target_compile_options(
  teensy_core PRIVATE -w -felide-constructors -fno-exceptions -fno-rtti
                      -fpermissive)

# ##############################################################################
# ILI9341_t3
# ##############################################################################

file(GLOB _ili_cpp "${TEENSY41_LIBS}/ILI9341_t3/*.cpp")
file(GLOB _ili_c "${TEENSY41_LIBS}/ILI9341_t3/*.c")

add_library(ili9341_t3 STATIC ${_ili_cpp} ${_ili_c})
target_link_libraries(ili9341_t3 PUBLIC teensy_defs)
target_compile_options(
  ili9341_t3 PRIVATE -w -felide-constructors -fno-exceptions -fno-rtti
                     -fpermissive)

# ##############################################################################
# Teensy Audio — only the files used by pulse2d::Audio
# ##############################################################################

add_library(
  teensy_audio STATIC
  "${TEENSY41_LIBS}/Audio/output_i2s.cpp"
  "${TEENSY41_LIBS}/Audio/control_sgtl5000.cpp"
  "${TEENSY41_LIBS}/Audio/utility/imxrt_hw.cpp"
  "${TEENSY41_LIBS}/Audio/utility/sqrt_integer.c"
  "${TEENSY41_LIBS}/Audio/memcpy_audio.S")
target_link_libraries(teensy_audio PUBLIC teensy_defs)
target_compile_options(
  teensy_audio PRIVATE -w -felide-constructors -fno-exceptions -fno-rtti
                       -fpermissive)

# ##############################################################################
# SdFat
# ##############################################################################

file(GLOB_RECURSE _sdfat_cpp "${TEENSY41_LIBS}/SdFat/src/*.cpp")

add_library(sdfat STATIC ${_sdfat_cpp})
target_link_libraries(sdfat PUBLIC teensy_defs)
target_compile_options(sdfat PRIVATE -w -felide-constructors -fno-exceptions
                                     -fno-rtti -fpermissive)

# ##############################################################################
# SPI
# ##############################################################################

add_library(teensy_spi STATIC "${TEENSY41_LIBS}/SPI/SPI.cpp")
target_link_libraries(teensy_spi PUBLIC teensy_defs)
target_compile_options(
  teensy_spi PRIVATE -w -felide-constructors -fno-exceptions -fno-rtti
                     -fpermissive)

# ##############################################################################
# Wire — WireIMXRT.cpp needs a patch to compile under C++20
# ##############################################################################

set(_wire_src "${TEENSY41_LIBS}/Wire/WireIMXRT.cpp")
set(_wire_patch "${CMAKE_CURRENT_BINARY_DIR}/WireIMXRT_patched.cpp")

if(NOT EXISTS "${_wire_patch}" OR "${_wire_src}" IS_NEWER_THAN "${_wire_patch}")
  execute_process(
    COMMAND
      sed "s/constexpr TwoWire::I2C_Hardware_t/const TwoWire::I2C_Hardware_t/g"
      "${_wire_src}"
    OUTPUT_FILE "${_wire_patch}"
    RESULT_VARIABLE _wire_result)
  if(NOT _wire_result EQUAL 0)
    message(FATAL_ERROR "Failed to patch WireIMXRT.cpp")
  endif()
endif()

add_library(teensy_wire STATIC "${TEENSY41_LIBS}/Wire/Wire.cpp"
                               "${_wire_patch}")
target_link_libraries(teensy_wire PUBLIC teensy_defs)
target_compile_options(
  teensy_wire PRIVATE -w -felide-constructors -fno-exceptions -fno-rtti
                      -fpermissive)

# ##############################################################################
# SD
# ##############################################################################

add_library(teensy_sd STATIC "${TEENSY41_LIBS}/SD/src/SD.cpp")
target_link_libraries(teensy_sd PUBLIC teensy_defs)
target_compile_options(teensy_sd PRIVATE -w -felide-constructors
                                         -fno-exceptions -fno-rtti -fpermissive)

# ##############################################################################
# SerialFlash
# ##############################################################################

file(GLOB _serialflash_cpp "${TEENSY41_LIBS}/SerialFlash/*.cpp")

add_library(teensy_serialflash STATIC ${_serialflash_cpp})
target_link_libraries(teensy_serialflash PUBLIC teensy_defs)
target_compile_options(
  teensy_serialflash PRIVATE -w -felide-constructors -fno-exceptions -fno-rtti
                             -fpermissive)

# ##############################################################################
# pulse2d engine
# ##############################################################################

file(GLOB_RECURSE _pulse2d_src CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

add_library(pulse2d_firmware STATIC ${_pulse2d_src})
target_link_libraries(pulse2d_firmware PUBLIC teensy_defs)
target_include_directories(pulse2d_firmware
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_options(
  pulse2d_firmware PRIVATE -Wall -Wextra -felide-constructors -fno-exceptions
                           -fno-rtti -fpermissive)

# ##############################################################################
# game-teensy — firmware executable
# ##############################################################################

add_executable(game-teensy "${CMAKE_CURRENT_SOURCE_DIR}/shift/game-teensy.cc")
target_link_libraries(
  game-teensy
  PRIVATE pulse2d_firmware
          teensy_core
          ili9341_t3
          teensy_audio
          sdfat
          teensy_spi
          teensy_wire
          teensy_sd
          teensy_serialflash)
target_link_options(game-teensy PRIVATE "-T${TEENSY41_CORE}/imxrt1062_t41.ld")

add_custom_command(
  TARGET game-teensy
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom "$<TARGET_FILE:game-teensy>"
          "$<TARGET_FILE_DIR:game-teensy>/game-teensy.hex"
  COMMENT "Converting to game-teensy.hex")

find_program(_size_tool arm-none-eabi-size)
if(_size_tool)
  add_custom_command(
    TARGET game-teensy
    POST_BUILD
    COMMAND ${_size_tool} "$<TARGET_FILE:game-teensy>"
    COMMENT "Memory usage")
endif()
