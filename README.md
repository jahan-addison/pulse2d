<div align="center">
  <img src="docs/logo/pulse2d-sleek-final-final-final.png" width="800" alt="pulse2d"> </img>
</div>


<h5 align="center">
  Teensy 4.1 2D Game Engine 🎮
</h5>


## Overview


The Teensy 4.1 is a microcontroller development board based on the NXP i.MX RT1062, an ARM Cortex-M7 running at up to 600 MHz. `pulse2d` enables you to turn the microcontroller into a 2D game platform with a display and controller, as it has hardware floating-point, a dedicated SPI bus, and a built-in SDIO SD card slot. 🎮

The pilot game, [asterisk](https://github.com/jahan-addison/asterisk), is a feature complete space-shooter using the pulse2d engine. It is **recommended as a starting point for development of new games**.

Check out the [blog series](https://soliloq.uy/tag/pulse2d/)!

### Demo:

![gif](/docs/demo/Asterisk%20Game%20Demo%20V1-GIF.gif)

## Hardware

The recommended hardware for game development with **no soldering required**:

- [Solderless breadboard](https://www.amazon.com/dp/B08Y59P6D1?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_5): Holds all components
- [Teensy 4.1](https://www.pjrc.com/store/teensy41.html): Primary microcontroller
- [ILI9341 TFT Display](https://www.pjrc.com/store/display_ili9341_touch.html): 320x240 RGB565 display via SPI
- [MicroSD card](https://www.amazon.com/dp/B0B7NV73PJ?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_4): Sprite and asset storage via the built-in SDIO slot
- [Seesaw Gamepad QT](https://www.adafruit.com/product/5743): I2C gamepad with analog thumbstick and 6 buttons

---

## Requirements

### Arduino IDE and Teensyduino

Building for Teensy 4.1 requires the Arduino IDE with the Teensy board support package (Teensyduino). Follow the installation instructions at:

https://www.pjrc.com/teensy/td_download.html

This installs the Teensy core, libraries, and linker scripts into your local Arduino package directory. **You do not need to use the Arduino IDE**, `cmake` and the provided `Makefile.teensy` will find the libraries automatically.

## Game Development

Game development in pulse2d is organized into two layers:

* **Internal DSL** - macros for the game's structural skeleton: type aliases, lifecycle hooks (`PULSE_ON_GAMESTART`, `PULSE_ON_GAMESCENE`), scene declarations, animation blueprints, and debug helpers.

* **Core API** - the `Runtime<Scenes...>` struct, which owns the engine, physics world, and active scene. All game actions (draw, spawn, collide, animate, etc.) are methods on this struct.

📖 [See the full API reference here](api.md)

A game that demonstrates most features:

```cpp
#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS
#include "../include/explosion-anim.h"
#include "../include/stars-bg.h"

PULSE2D_START_PULSE();

PULSE_DEFINE_SCENE(Space_Shooter, 20, 6);
PULSE_INIT_GAME(my_game, Space_Shooter);

PULSE_DEFINE int score = 0;
PULSE_DEFINE int cooldown = 0;
PULSE_DEFINE bool enemy_hit = false;

PULSE_ON_GAMESCENE_START(Space_Shooter)
{
    my_game.set_sprite_flash("bg_stars", stars_bg, 320, 240);
    my_game.set_sprite("ship_sprite", "ship.bin", 48, 48);
    my_game.set_sprite("enemy_sprite", "enemy.bin", 48, 48);
    my_game.set_sprite("bullet_sprite", "bullet.bin", 12, 8);

    my_game.add_parallax_layer("bg_stars", 320.0f, 15.0f);

    my_game.register_vfx("explosion", explosion_frames, 64, 64, 8, 12.0f);

    my_game.init_pool("bullets",
        {
            .width = { 0.15f, 0.08f }
    });

    my_game.set_controlled_body("ship_object",
        {
            .position = { -4.0f, 0.0f },
              .width = { 0.5f,  0.5f }
    });
    my_game.set_dynamic_body("enemy_object",
        {
            .position = { 3.0f, 0.0f },
            .mass = 1.0f,
            .width = { 0.6f, 0.6f }
    });
}

PULSE_ON_GAMESCENE(Space_Shooter)
{
    my_game.tick();

    PULSE_POLL_SEESAW_GAMEPAD();

    my_game.render_backgrounds();
    my_game.set_arcade_directional_control("ship_object", 3.5f);

    pulse2d_body& ship = my_game.get_body("ship_object");

    if (cooldown > 0)
        cooldown--;

    if (SEESAW_BUTTON_INPUT(SEESAW_A) && cooldown == 0) {
        my_game.spawn("bullets",
            100,
            ship.position.x + 0.6f,
            ship.position.y,
            8.0f,
            0.0f);
        cooldown = 10;
    }

    my_game.render_pool("bullets", [&](pulse2d_body* bullet) {
        pulse2d_body& enemy = my_game.get_body("enemy_object");

        if (bullet->position.x > 6.0f) {
            my_game.despawn("bullets", bullet);
        } else {
            my_game.draw_body(bullet, "bullet_sprite");
        }

        my_game.on_collision(bullet, &enemy, [&] {
            if (!enemy_hit) {
                enemy_hit = true;
                auto coords = get_body_coordinates(&enemy);
                my_game.play_vfx("explosion", coords.x, coords.y);
                score += 100;
            }
            my_game.despawn("bullets", bullet);
        });
    });

    if (SEESAW_BUTTON_INPUT(SEESAW_START)) {
        PULSE_SET_SCENE(my_game, Space_Shooter);
    }

    my_game.draw("ship_object", "ship_sprite");

    if (!enemy_hit) {
        my_game.draw("enemy_object", "enemy_sprite");
    }

    my_game.tick_vfx();
    my_game.render();
}

PULSE_ON_GAMESTART()
{
    Serial.begin(115200);
    pulse_register_etl_error_handler();

    my_game.init(0.0f, 0.0f, 10);

    PULSE_ENABLE_SEESAW_GAMEPAD();

    PULSE_SET_SCENE(my_game, Space_Shooter);
}

PULSE_ON_GAMELOOP()
{
    PULSE_TICK_GAMESCENE();
}
```

#### Features

- Scene management - organize game states as scenes with isolated body and sprite pools
- Physics management - static, controlled, and dynamic objects with collision detection
- Animations - spritesheet animations for VFX and persistent looping with automatic frame advancement
- Kinematic pools - pre-allocated object pools for bullets, particles, powerups
- Parallax backgrounds - multi-layer scrolling backgrounds from flash memory
- Gamepad profiles - arcade (instant), momentum (acceleration), and friction movement
- Debug tools - stack usage tracking and ETL error reporting

See [api.md](api.md) for the complete reference with detailed examples.

---

## Building

Install the ARM bare-metal toolchain:

```bash
# macOS
brew install --cask gcc-arm-embedded
export PATH="/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin:$PATH"

# Ubuntu
sudo apt install gcc-arm-none-eabi
```

Link against `pulse2d::pulse2d` in your `CMakeLists.txt`:

```cmake
add_subdirectory(pulse2d)          # or use CPM, FetchContent
target_link_libraries(my_game PRIVATE pulse2d::pulse2d)
```

---

### Build sample game:

The included sample game targets both SDL2 and Teensy 4.1:

### Host

```bash
# macOS
brew install sdl2

cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DUSE_SANITIZER="Address;Undefined" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSDL2_DIR=$(brew --prefix sdl2)/lib/cmake/SDL2
cmake --build build
./build/sample_game

# Ubuntu
sudo apt update
sudo apt install libsdl2-dev

cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
./build/sample_game


```

### Teensy

You can use the `Makefile.teensy` to build and flash the sample game:

```bash
make -f Makefile.teensy -j                      # build
make -f Makefile.teensy clean                   # remove build-teensy/
make -f Makefile.teensy flash                   # flash with teensy_loader_cli
make -f Makefile.teensy asm                     # compile game sources to ARM assembly
make -f Makefile.teensy sections                # section map: region, VMA, size for every output section
make -f Makefile.teensy sections SECTIONS_ALL=1 # same, including small sections

make -f Makefile.teensy ldscript                # print the linker script memory regions and section -> region table
```

---

### Build your own game

Set three variables and include `Makefile.teensy` from your own Makefile:

```makefile
# my_game/Makefile
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

`TEENSY_HW` is auto-detected from the Arduino package directory, see [Makefile.teensy](Makefile.teensy) for all configurable variables.

#### Note: Check the [tools](#tools) section for additional asset and debugging tools during game development:

```bash
make asm
make sections
make sections SECTIONS_ALL=1
make ldscript
```

---

# Drivers

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
* [Gamepad](#gamepad): `pulse2d::gamepad::`
  - Seesaw Gamepad QT I2C driver

## Display

On Teensy, the display driver targets the [PJRC ILI9341 TFT](https://www.pjrc.com/store/display_ili9341_touch.html), driven by the `ILI9341_t3` library. On host and local development, the driver opens an SDL2 window at the same logical resolution scaled up by `pulse2d::config::scale`.

## Storage

Load sprites via `Storage::load_sprite()`. On the host any image format supported by stb_image works. The image is nearest-neighbour scaled to the requested dimensions and converted to RGB565:

On Teensy, `load_sprite` reads the raw binary format (`uint16_t` width, `uint16_t` height, then `width x height` RGB565 pixels) from the SD card.

## Audio

* TODO

## Physics

The physics component is a port of [box2d-lite](https://github.com/erincatto/box2d-lite) modified for embedded use: dynamic allocation replaced with fixed-size containers, all math in single-precision float, and the solver tuned for the Teensy 4.1's Cortex-M7.

For more details, see the [physics readme](pulse2d/graphics/readme.md).

## Renderer

The `Renderer` holds the full-screen RGB565 framebuffer for razterization and blitting. Each frame runs clear, draw, and render.

## Gamepad

The gamepad driver targets the [Adafruit Seesaw Gamepad QT](https://www.adafruit.com/product/5743) over I2C. The DSL wraps setup, polling, and input.

---

# Tools

## Assets

Python tools for converting PNG assets are in `tools/` - both require [Pillow](https://pillow.readthedocs.io/).

- `png2bin` - converts a PNG to the raw RGB565 binary format read by `Storage::load_sprite` on Teensy. Use this for game-object sprites (ships, enemies, projectiles) that are loaded from the SD card at runtime. Transparent pixels become `0xF81F` (magenta chroma-key).

  ```bash
  tools/png2bin sprite.png sprite.bin 64 64
  ```

- `png2header` - converts a PNG to a C header containing an RGB565 pixel array for use as a full-screen background. Use `PULSE2D_SPRITE_FLASH` and `PULSE2D_ADD_PARALLAX_LAYER` to register the header as a parallax layer. The output file is generated - do not edit it by hand.

  ```bash
  tools/png2header background.png include/nebula-bg.h bg_1 320 240
  ```

  Generates `bg_1_width`, `bg_1_height`, and `bg_1[320 * 240]`.

- `animation2header` - convert multiple PNGs to a C header containing a contiguous RGB565 pixel array for use as spritesheets.

  ```bash
  tools/animation2header include/laser-anim.h laser_anim 54 32 laser-sprite1.png laser-sprite2.png laser-sprite3.png
  ```

  Generates `laser_anim_width`, `laser_anim_height`, and `laser_anim[54 * 32 * 3]`.

## Debug

Debugging tools (no dependencies):

- `sections` - reads a compiled ELF and prints a sorted table of every output section grouped by memory region (FLASH / DTCM / RAM / ERAM), with VMA, LMA, size, and whether the section is copied to RAM at boot or flash-only. Useful for catching oversized `.data` or `.bss` before the linker rejects the binary.

  ```bash
  tools/sections build-teensy/asterisk.elf
  tools/sections build-teensy/asterisk.elf --all
  tools/sections build-teensy/asterisk.elf --min-bytes 1024
  # or via make:
  make sections
  make sections SECTIONS_ALL=1
  ```

![img](/docs/tool-sections.png)

---

- `ldscript` - parses `imxrt1062_t41.ld` from the active Teensyduino install and prints the MEMORY region table and the output section → region map. The quickest way to answer "which section attribute keeps this array in flash?" without reading the raw linker script.

  ```bash
  tools/ldscript
  tools/ldscript --ld /path/to/imxrt1062_t41.ld   # explicit path
  # or via make:
  make ldscript
  ```

![img](/docs/tool-ldscript.png)

## Dependencies

Host dependencies are fetched automatically via [CPM](https://github.com/cpm-cmake/CPM.cmake).

- [`ETLCPP`](https://www.etlcpp.com/) - Embedded Template Library
- `box2d-lite` - Heavily modified port of [box2d-lite](https://github.com/erincatto/box2d-lite) for embedded devices
- `doctest` - Test framework
- `stb` - Image loading for the host storage backend
- `SDL2` - Display driver for host development

## License

MIT License

