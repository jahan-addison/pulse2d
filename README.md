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

The Teensy 4.1 is a microcontroller development board based on the NXP i.MX RT1062, an ARM Cortex-M7 running at up to 600 MHz. It has hardware floating-point, a dedicated SPI bus, and a built-in SDIO SD card slot - enough for a 2D game platform along side an Adafruit ILI9341 TFT for display.

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

## Architecture

The engine is structured around three components:

| Component | Class | Library |
|-----------|-------|---------|
| Display | `luya::display::Display` | ILI9341_t3, SDL2 |
| Audio | `luya::Audio` | Teensy Audio Library |
| Storage | `luya::Storage` | SdFat, stb_image |

`Engine::init()` is called from Teensy `setup()` and `Engine::tick(world)` from `loop()`. On the host the SDL2 game loop runs both.

### Display drivers

The display component is compile-time polymorphic and defaults to `SDL2`

| Driver | Class | Target |
|--------|-------|--------|
| Adafruit 2.8" TFT | `Adafruit_Display` | Teensy 4.1 hardware  |
| SDL2 | `SDL_Display` | Host development |

The SDL2 driver opens a desktop window at the ILI9341 native resolution (320×240) scaled up by `config::scale` (3×, 960×720). `SDL_RenderSetLogicalSize` ensures all draw calls use the same coordinate space as the Adafruit.

## Usage

### Using as a library

Link against `luya::engine` in your `CMakeLists.txt`:

```cmake
add_subdirectory(luya)          # or use CPM, FetchContent
target_link_libraries(my_game PRIVATE luya::engine)
```

A minimal game loop:

```cpp
#include <luya/engine.h>
#include <luya/physics/world.h>

luya::Engine engine{};
luya::physics::World world({ 0.0f, -10.0f }, 10);

engine.init();

while (running) {
    world.step(1.0f / 60.0f);
    engine.tick(world);
}
```

## Overview

### Physics

The physics module is a heavily-edited port of box2d-lite that uses sequential impulse constraint solving over a fixed timestep. Dynamic allocation has been replaced with ETL fixed-size containers.

#### Bodies

A `Body` is axis-aligned and box-shaped, `width` holds the half-extents. A default-constructed body is static (infinite mass):

```cpp
physics::Body floor;
floor.position = { 0.0f, -4.0f };   // static — inv_mass = 0 by default
floor.width    = { 5.0f, 0.5f };    // 10x1 unit platform

physics::Body box;
box.set({ 0.5f, 0.5f }, 2.0f);      // 1x1 unit box, 2 kg
box.position = { 0.0f, 3.0f };
box.add_force({ 1.0f, 0.0f });       // nudge right
```

#### World

`World` holds ~~the universe~~ the body and joint lists and runs the solver:

```cpp
physics::World world({ 0.0f, -10.0f }, 10);  // gravity, solver iterations

world.add(&floor);
world.add(&box);

world.step(1.0f / 60.0f);   // advance one 60 Hz tick
```

`world.arbiters` is non-empty for every active contact pair after each `step()`. Use it to detect collision:

```cpp
if (!world.arbiters.empty()) {
    // at least one contact is active this step
}
```

#### Joints

A `Joint` maintains a fixed distance between two bodies at a world-space anchor:

```cpp
physics::Joint hinge;
hinge.set(&body_a, &body_b, { 0.0f, 1.0f });
world.add(&hinge);
```

`bias_factor` (default 0.2) controls position correction strength; `softness` (default 0.0) adds compliance.

### Renderer and sprites

The `Renderer` holds the full-screen RGB565 framebuffer. Each frame runs clear, draw, render:

```cpp
auto& renderer = engine.renderer();

renderer.clear();
renderer.add_sprite(&my_sprite, x, y);   // queue sprites before draw
// engine.tick() calls draw() + render() internally
```

Load sprites via `Storage::load_sprite()`. On the host any image format supported by stb_image works; the image is nearest-neighbour scaled to the requested dimensions and converted to RGB565:

```cpp
luya::Sprite s = engine.storage().load_sprite("hero.png", 32, 32);
```

On Teensy, `load_sprite` reads the raw binary format (uint16_t width, uint16_t height, then width×height RGB565 pixels) from the SD card.

Use `Renderer::world_to_screen()` to convert a physics body position to a pixel coordinate:

```cpp
auto [sx, sy] = renderer.world_to_screen(body.position.x, body.position.y);
renderer.add_sprite(&sprite,
    static_cast<int16_t>(sx - sprite.width  / 2),
    static_cast<int16_t>(sy - sprite.height / 2));
```

## Build

### macOS — SDL2

SDL2 is required for all host builds:

```bash
brew install sdl2
```

Then build:

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DUSE_SANITIZER="Address;Undefined" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSDL2_DIR=$(brew --prefix sdl2)/lib/cmake/SDL2
cmake --build build
./build/luya
```

---

Run the test suite:

```bash
./build/test_suite
```

### Debian Based — SDL2

Install dependencies:

```bash
sudo apt update
sudo apt install cmake ninja-build libsdl2-dev
```

Then build:

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DUSE_SANITIZER="Address;Undefined" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
./build/luya
```

Run the test suite:

```bash
./build/test_suite
```

### Windows — MinGW + SDL2

Install [MSYS2](https://www.msys2.org/), then from an **MSYS2 MinGW64** shell install dependencies:

```bash
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-SDL2
```

```bash
cmake -Bbuild -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
./build/luya.exe
```

Run the test suite:

```bash
./build/test_suite.exe
```

> **Note:** The MSVC toolchain is not supported. Use the MinGW64 shell from MSYS2, not a Visual Studio developer prompt.

### Teensy 4.1 — cross-compile

Install [Teensyduino](https://www.pjrc.com/teensy/teensyduino.html) to get `arm-none-eabi-gcc` and the Teensy libraries, then:

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-teensy41.cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To use the DMA-capable `ILI9341_t3n` display driver instead of `ILI9341_t3`, add `-DUSE_ILI9341_T3N=ON`.

If Teensyduino is not installed at the default Arduino.app path, override with:

```bash
-DTEENSYDUINO_PATH=/path/to/your/teensyduino/hardware/teensy
```

## Dependencies

- `ETLCPP` - [Embedded Template Library](https://www.etlcpp.com/)
- `box2d-lite` Heavily modified port of [box2d-lite](https://github.com/erincatto/box2d-lite) for embedded devices

Teensy libraries

- `teensy4_core` — Teensy 4.1 hardware abstraction and startup
- `ILI9341_t3`, `ILI9341_t3n` — ILI9341 TFT display drivers
- `SdFat` — SD card filesystem via built-in SDIO
- `Teensy Audio Library` — I2S audio pipeline and SGTL5000 codec

Host-only dependencies

- `SDL2` — Default display driver for host development

## Licensing

This project is dual-licensed under the **Apache License, Version 2.0** or the **GNU General Public License, Version 3.0 (or later)**.

You are free to choose the license that best fits your specific use case. For the full text of each license, please see [LICENSE.Apache-v2](LICENSE.Apache-v2) and [LICENSE-GPL-v3](LICENSE.GPL-v3).

