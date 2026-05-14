<div align="center">
  <img src="docs/pulse2d-logo-white.png" width="800px" alt="pulse2d"> </img>
</div>

<h5 align="center">
  Teensy 4.1 2D Game Engine 🎮
</h5>


## Overview

<table border="0">
  <tr>
    <td width="320">
      <img src="/docs/teensy-front.png" width="300">
    </td>
    <td>
      <p>
      The Teensy 4.1 is a microcontroller development board based on the NXP i.MX RT1062, an ARM Cortex-M7 running at up to 600 MHz. pulse2d enables you to turn the microcontroller into a 2D game platform with a display and controller, as it has hardware floating-point, a dedicated SPI bus, and a built-in SDIO SD card slot. 🎮
      </p>
    </td>
  </tr>
</table>

The project builds a sample game for desktop and the teensy hardware called `shift`.

You can use the `Makefile.teensy` to build and flash the sample game:

```bash
make -f Makefile.teensy -j     # build
make -f Makefile.teensy clean  # remove build-teensy/
make -f Makefile.teensy flash  # flash with teensy_loader_cli
```

## Requirements

### Arduino IDE and Teensyduino

Building for Teensy 4.1 requires the Arduino IDE with the Teensy board support package (Teensyduino). Follow the installation instructions at:

https://www.pjrc.com/teensy/td_download.html

This installs the Teensy core, libraries, and linker scripts into your local Arduino package directory. **You do not need to use the Arduino IDE**, `cmake` and the provided `Makefile.teensy` will find the libraries automatically.

---

Install the ARM bare-metal toolchain:

```bash
# macOS
brew install --cask gcc-arm-embedded
export PATH="/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin:$PATH"

# Ubuntu
sudo apt install gcc-arm-none-eabi
```

## How to use

### C++

Link against `pulse2d::pulse2d` in your `CMakeLists.txt`:

```cmake
add_subdirectory(pulse2d)          # or use CPM, FetchContent
target_link_libraries(my_game PRIVATE pulse2d::pulse2d)
```

### NodeJS

> NodeJS bindings coming soon.

## Game Development

### Sample game

The included sample game `shift` targets both SDL2 and Teensy 4.1.

**Host:**

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSDL2_DIR=$(brew --prefix sdl2)/lib/cmake/SDL2
cmake --build build
./build/shift_game
```

**Teensy 4.1:**

```bash
make -f Makefile.teensy -j
make -f Makefile.teensy flash
```

---

### Building your own game

Set three variables and include `Makefile.teensy` from your own Makefile:

```makefile
# MyGame/Makefile
PULSE2D_ROOT = /path/to/pulse2d
GAME_SRCS    = src/mygame.cc src/level.cc
GAME_NAME    = mygame

include $(PULSE2D_ROOT)/Makefile.teensy
```

Then from your game directory:

```bash
make -j
make flash
```

`TEENSY_HW` is auto-detected from the Arduino package directory; see [Makefile.teensy](Makefile.teensy) for all configurable variables.

# Architecture

* [Display](#display): `pulse2d::Display`
  - The display adapter, with a host SDL2 interface
* [Storage](#storage): `pulse2d::Storage`
  - Storage of textures, sprites, and other assets in memory
* [Physics](#physics): `pulse2d::graphics::`
  - The physics engine
* [Renderer](#renderer): `pulse2d::Renderer`
  - The RGB565 framebuffer, rasterization, blitting
* [Audio](#audio): `pulse2d::Audio`
  - Audio interface via the Teensy audio library

## Display

On Teensy, the display driver targets the [PJRC ILI9341 TFT](https://www.pjrc.com/store/display_ili9341_touch.html), driven by the `ILI9341_t3` library. On host and local development, the driver opens an SDL2 window at the same logical resolution scaled up by `pulse2d::config::scale`.

## Storage

Load sprites via `Storage::load_sprite()`. On the host any image format supported by stb_image works; the image is nearest-neighbour scaled to the requested dimensions and converted to RGB565:

```cpp
pulse2d::Sprite s = engine.storage().load_sprite("hero.png", 32, 32);
```

On Teensy, `load_sprite` reads the raw binary format (`uint16_t` width, `uint16_t` height, then `width x height` RGB565 pixels) from the SD card.

* TODO

The storage component will also enable loading of sounds and audio files.

## Audio

* TODO

## Physics

The physics component is a port of [box2d-lite](https://github.com/erincatto/box2d-lite) modified for embedded use: dynamic allocation replaced with fixed-size containers, all math in single-precision float, and the solver tuned for the Teensy 4.1's Cortex-M7.


For more details, see the [physics readme](pulse2d/graphics/readme.md).


## Renderer

The `Renderer` holds the full-screen RGB565 framebuffer for razterization and blitting. Each frame runs clear, draw, and render:

```cpp
auto& renderer = engine.renderer();

pulse2d::Sprite my_sprite = engine.storage().load_sprite("hero.png", 32, 32);

renderer.clear();
renderer.add_sprite(&my_sprite, x, y); // queue sprites before draw
// engine.tick() will call draw() + render()
```

Use `Renderer::project_coordinates()` to convert a physics body position to a pixel coordinate:

```cpp
auto [sx, sy] = renderer.project_coordinates(body.position.x, body.position.y);
renderer.add_sprite(&my_sprite,
    static_cast<int16_t>(sx - my_sprite.width  / 2),
    static_cast<int16_t>(sy - my_sprite.height / 2));
```

## Dependencies

Host dependencies are fetched automatically via [CPM](https://github.com/cpm-cmake/CPM.cmake).

- [`ETLCPP`](https://www.etlcpp.com/) - Embedded Template Library
- `box2d-lite` - Heavily modified port of [box2d-lite](https://github.com/erincatto/box2d-lite) for embedded devices
- `doctest` - Test framework
- `stb` - Image loading for the host storage backend
- `SDL2` - Display driver for host development

## License

MIT License

