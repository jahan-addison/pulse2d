<h5 align="center">
  Luya 2D Gaming Engine 🎮
</h5>

<table align="center">
<tr>
<td>

```text
         _   _   _ __   __ _
        | | | | | |\ \ / // \
        | | | | | | \ V // _ \
        | |_| |_| |  | |/ ___ \
        |___|\___/   |_/_/   \_\

              2D Gaming Engine
                on Teensy 4.1
```
</td>
</tr>
</table>

## Overview

The Teensy 4.1 is a microcontroller development board based on the NXP i.MX RT1062, an ARM Cortex-M7 running at up to 600 MHz. Luya enables you to turn the microcontroller into a 2D game platform with a display and controller, as it has hardware floating-point, a dedicated SPI bus, and a built-in SDIO SD card slot,

<table border="0">
  <tr>
    <td width="220">
      <img src="/docs/teensy-front.jpg" width="200">
    </td>
    <td>
      Luya targets the i.MX RT1062 at 600 MHz with FPv5-D16 hard-float ABI. The display component wraps <code>ILI9341_t3</code> (or the DMA-capable <code>ILI9341_t3n</code>), audio is routed through the Teensy Audio Library via I2S to the SGTL5000 codec, and asset storage is served from the built-in SDIO SD card slot via <code>SdFat</code>.
    </td>
  </tr>
</table>

### Architecture

* [Display](#display): `luya::display::Display`
  - The display adapter, with a host SDL2 interface
* [Storage](#storage): `luya::Storage`
  - Storage of textures, sprites, and other assets in memory
* [Physics](#physics): `luya::physics::World`
  - The physics engine
* [Renderer](#renderer): `luya::Renderer`
  - The RGB565 framebuffer, rasterization, blitting
* [Audio](#audio): `luya::Audio`
  - Audio interface via the Teensy audio library
* [Control](#control): `luya::Control`
  - Controller interface

## Installation

### Arduino IDE and Teensyduino

Building for Teensy 4.1 requires the Arduino IDE with the Teensy board support package (Teensyduino). Follow the installation instructions at:

https://www.pjrc.com/teensy/td_download.html

This installs the Teensy core, libraries, and linker scripts into your local Arduino package directory. **You will not** need to use the Arduino IDE, `cmake` and the build will find them automatically.

## Usage

### Using as a library

Link against `luya::luya` in your `CMakeLists.txt`:

```cmake
add_subdirectory(luya)          # or use CPM, FetchContent
target_link_libraries(my_game PRIVATE luya::luya)
```

`Engine::init()` is called once at startup and `Engine::tick(world)` each frame:

```cpp
#include <luya/engine.h>
#include <luya/physics/world.h>

static luya::Engine engine;
static luya::physics::World world({ 0.0f, -10.0f }, 10);

void setup()
{
    engine.init();
}

void loop()
{
    world.step(1.0f / 60.0f);
    engine.tick(world);
}

```

See [sample/teensy-main.cc](/sample/teensy-main.cc) for the Teensy sample game, and [sample/main.cc](/sample/main.cc) for the SDL2 host example.

### Teensy 4.1

The build produces two output files in `build-teensy/`:

* `sample_game.elf`: ELF with full debug symbols — use with a JTAG probe or `arm-none-eabi-gdb`
* `sample_game.hex`: The file you flash onto the Teensy

To flash, press the button on the Teensy to enter the bootloader, then use [Teensy Loader](https://www.pjrc.com/teensy/loader.html):

```bash
# GUI
open build-teensy/sample_game.hex   # macOS: opens Teensy Loader automatically

# CLI (install via brew install teensy-loader-cli or apt install teensy-loader-cli)
teensy_loader_cli --mcu=TEENSY41 -w -v build-teensy/sample_game.hex
```

## Components

## Display


* Adafruit: `luya::display::Adafruit_Display`
  - An Adafruit 2.8" ILI9341 TFT display driver that works well with the Teensy 4.1
* SDL2: `luya::display::SDL_Display`
  - A host desktop development driver for debugging and the game development phase

The display component defaults to `SDL2`. The SDL2 driver opens a desktop window at the ILI9341 native resolution scaled up by `config::scale`.

## Storage

Load sprites via `Storage::load_sprite()`. On the host any image format supported by stb_image works; the image is nearest-neighbour scaled to the requested dimensions and converted to RGB565:

```cpp
luya::Sprite s = engine.storage().load_sprite("hero.png", 32, 32);
```

On Teensy, `load_sprite` reads the raw binary format (`uint16_t` width, `uint16_t` height, then `width x height` RGB565 pixels) from the SD card.

* TODO

The storage component will also enable loading of sounds and audio files.

## Audio

* TODO

## Control

* TODO

## Physics

The physics component is a port of [box2d-lite](https://github.com/erincatto/box2d-lite) modified for embedded use, such as dynamic allocation replaced with ETL fixed-size containers, all math in single-precision float, and the solver tuned for the Teensy 4.1's Cortex-M7.


For more details, see the [physics readme](luya/physics/readme.md).


## Renderer

The `Renderer` holds the full-screen RGB565 framebuffer for razterization and blitting. Each frame runs clear, draw, and render:

```cpp
auto& renderer = engine.renderer();

luya::Sprite my_sprite = engine.storage().load_sprite("hero.png", 32, 32);

renderer.clear();
renderer.add_sprite(&my_sprite, x, y); // queue sprites before draw
// engine.tick() will call draw() + render()
```

Use `Renderer::world_to_screen()` to convert a physics body position to a pixel coordinate:

```cpp
auto [sx, sy] = renderer.world_to_screen(body.position.x, body.position.y);
renderer.add_sprite(&my_sprite,
    static_cast<int16_t>(sx - my_sprite.width  / 2),
    static_cast<int16_t>(sy - my_sprite.height / 2));
```

## Build

### macOS - SDL2

SDL2 is required for all host builds. Install both SDL2 and the ARM bare-metal toolchain:

```bash
brew install sdl2
brew install --cask gcc-arm-embedded
export PATH="/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin:$PATH"
```

> Add the `export` line to your `~/.zshrc` to make it permanent

Then build:

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DUSE_SANITIZER="Address;Undefined" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSDL2_DIR=$(brew --prefix sdl2)/lib/cmake/SDL2
cmake --build build
./build/sample_game
```

---

Run the test suite:

```bash
./build/test_suite
```

### Debian Based - SDL2

Install dependencies:

```bash
sudo apt update
sudo apt install cmake ninja-build libsdl2-dev gcc-arm-none-eabi
```

Then build:

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DUSE_SANITIZER="Address;Undefined" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
./build/sample_game
```

Run the test suite:

```bash
./build/test_suite
```

### Windows - MinGW + SDL2

Install [MSYS2](https://www.msys2.org/), then from an **MSYS2 MinGW64** shell install dependencies:

```bash
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-SDL2 mingw-w64-x86_64-arm-none-eabi-toolchain
```

```bash
cmake -Bbuild -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
./build/sample_game.exe
```

Run the test suite:

```bash
./build/test_suite.exe
```

> **Note:** The MSVC toolchain is not supported.

### Teensy 4.1

Install the ARM bare-metal toolchain:

```bash
# macOS
brew install --cask gcc-arm-embedded
export PATH="/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin:$PATH"

# Ubuntu
sudo apt install gcc-arm-none-eabi
```

```bash
cmake -Bbuild
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-teensy41.cmake -Bbuild-teensy -DCMAKE_BUILD_TYPE=Release
cmake --build build-teensy
```

This invokes `cmake/build-teensy.sh`, which compiles all Teensyduino libraries and luya sources and produces `build-teensy/sample_game.elf` and `build-teensy/sample_game.hex`.

To override the Teensyduino hardware root (e.g. a non-default install path):

```bash
export TEENSY41_HW=/path/to/hardware/avr/1.60.0
```

## Dependencies

Host dependencies are fetched automatically via [CPM](https://github.com/cpm-cmake/CPM.cmake).

- [`ETLCPP`](https://www.etlcpp.com/) - Embedded Template Library
- `box2d-lite` - Heavily modified port of [box2d-lite](https://github.com/erincatto/box2d-lite) for embedded devices
- `doctest` - Test framework
- `stb` - Image loading for the host storage backend
- `SDL2` - Display driver for host development

## Licensing

This project is dual-licensed under the **Apache License, Version 2.0** or the **GNU General Public License, Version 3.0 (or later)**.

You are free to choose the license that best fits your specific use case. For the full text of each license, please see [LICENSE.Apache-v2](LICENSE.Apache-v2) and [LICENSE-GPL-v3](LICENSE.GPL-v3).

