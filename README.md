<div align="center">
  <img src="images/logo/pulse2d-logo.png" width="800" alt="pulse2d"> </img>
</div>


<h5 align="center">
  Teensy 4.1 2D Game Engine 🎮
</h5>


## Overview

Pulse2D is a 2D game engine for the Teensy 4.1. Games are organized into scenes with physics bodies, sprite pools, animations, and entity state machines - all with a clean API and DSL primitives.

The pilot game, [asterisk](https://github.com/jahan-addison/asterisk), is a feature-complete space shooter built with Pulse2D. It is **recommended as a starting point for development of new games**.

Check out the [blog series](https://soliloq.uy/tag/pulse2d/)!

### Demo:


<div align="center">
  <img src="images/demo/Asterisk Game Demo V5.gif" width="800" alt="pulse2d"> </img>
</div>

## Hardware

The supported hardware for game development with **no soldering required**:

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

Game development in Pulse2D is organized into two layers:

* **Internal DSL** - macros for the game's structural skeleton: type aliases, lifecycle hooks (`PULSE_ON_GAMESTART`, `PULSE_ON_GAMESCENE`), scene declarations, animation blueprints, and debug helpers.

* **Core API** - the `Runtime<Scenes...>` struct, which owns the engine, physics world, and active scene. All game actions (draw, spawn, collide, animate, etc.) are methods on this struct.

* 📖 [See the full API reference here](api.md)
* 📖 [Platform and memory guide](platform.md)

A game that demonstrates most features:

```cpp
// scenes/levels/space_shooter.h
#pragma once

#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS
#include <assets/explosion-anim.h>
#include <assets/stars-bg.h>
#include <audio/music.h>
#include <audio/laser-sfx.h>
#include <audio/explosion-sfx.h>

namespace scenes::levels::space_shooter {

struct State {
    int  score     = 0;
    int  cooldown  = 0;
    bool enemy_hit = false;
};

PULSE_DEFINE_SCENE_STATE(State);

PULSE_SCENE_FN void on_start(pulse2d_scene_runtime<Scenes...>& game)
{
    state = {};

    game
        .set_background_sprite("bg_stars", stars_bg, 320, 240)

        .set_sprite("ship_sprite", "ship.bin")
        .set_sprite("enemy_sprite", "enemy.bin")
        .set_sprite("bullet_sprite", "bullet.bin")

        .add_parallax_layer("bg_stars", 320.0f, 15.0f)
        .register_vfx("explosion",      explosion_frames, 64, 64, 8, 12.0f)
        .init_pool("bullets",           { .width = { 0.15f, 0.08f } })

        .set_controlled_body("ship_object", {
            .position = { -4.0f, 0.0f },
            .width    = { 0.5f,  0.5f }
        })
        .set_dynamic_body("enemy_object", {
            .position = { 3.0f, 0.0f },
            .mass     = 1.0f,
            .width    = { 0.6f, 0.6f }
        });

    game.play_music(AudioBackgroundMusic);
}

PULSE_SCENE_FN void on_tick(pulse2d_scene_runtime<Scenes...>& game,
    pulse2d_body& ship,
    void (*on_reset)())
{
    PULSE_POLL_SEESAW_GAMEPAD();

    game.tick_audio();
    game.set_arcade_directional_control("ship_object", 3.5f);

    if (state.cooldown > 0)
      state.cooldown--;

    if (SEESAW_BUTTON_INPUT(SEESAW_A) and state.cooldown == 0) {
        game.spawn("bullets", 100,
            ship.position.x + 0.6f, ship.position.y,
            8.0f, 0.0f);
        game.play_sfx(AudioLaserSfx);
        state.cooldown = 10;
    }

    game.render_pool("bullets", [&](pulse2d_body* bullet) {
        pulse2d_body& enemy = game.get_body("enemy_object");

        if (bullet->position.x > 6.0f) {
            game.despawn("bullets", bullet);
        } else {
            game.draw_body(bullet, "bullet_sprite");
        }

        game.on_collision(bullet, &enemy, [&] {
            if (!state.enemy_hit) {
                state.enemy_hit = true;
                auto coords = get_body_coordinates(&enemy);
                game.play_vfx("explosion", coords.x, coords.y);
                game.play_sfx2(AudioExplosionSfx);
                state.score += 100;
            }
            game.despawn("bullets", bullet);
        });
    });

    if (SEESAW_BUTTON_INPUT(SEESAW_START)) {
        on_reset();
        return;
    }

    game.draw("ship_object", "ship_sprite");
    if (!state.enemy_hit)
        game.draw("enemy_object", "enemy_sprite");
}

} // namespace scenes::levels::space_shooter
```

```cpp
// src/game.cc
#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS
#include <scenes/levels/space_shooter.h>

namespace space_shooter = scenes::levels::space_shooter;

PULSE2D_START_PULSE();
PULSE_DEFINE_SCENE(Space_Shooter, 20, 6);
PULSE_INIT_GAME(my_game, Space_Shooter);

PULSE_ON_GAMESCENE_START(Space_Shooter)
{
    space_shooter::on_start(my_game);
}

PULSE_ON_GAMESCENE(Space_Shooter)
{
    my_game.tick();
    my_game.render_backgrounds();

    pulse2d_body& ship = my_game.get_body("ship_object");

    space_shooter::on_tick(my_game, ship, [] {
        PULSE_DEFER_SCENE(my_game, Space_Shooter);
    });

    my_game.tick_vfx();
    my_game.render();
}

PULSE_ON_GAMESTART()
{
    Serial.begin(115200);
    pulse_register_etl_error_handler();
    my_game.init(0.0f, 0.0f, 10);
    my_game.enable_audio(); // audio shield is available
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
- Audio - looping background music and two independent SFX channels via the SGTL5000 codec
- Text - Text writing API via `glcdfont.h` with extensive color selection
- Animations - spritesheet animations for VFX and persistent looping with automatic frame advancement
- Kinematic pools - pre-allocated object pools for bullets, particles, powerups
- Parallax backgrounds - multi-layer scrolling backgrounds from flash memory
- Gamepad profiles - arcade (instant), momentum (acceleration), and friction movement
- Entity state machines - `Entity_Controller<SM, Data, Config>` wraps `boost/sml` for enemies, pickups, and any object with lifecycle behaviour
- [Debug tools](/platform.md) - stack usage tracking and ETL error reporting

See [api.md](api.md) for the complete reference with detailed examples.

---

### Physics Engine

A port of [box2d-lite](https://github.com/erincatto/box2d-lite) adapted for fixed-size allocation, single-precision float, and a two-stage AABB broad phase. See the [physics readme](pulse2d/graphics/readme.md) for details.

### State Machines

The state machine library [boost/sml](https://boost-ext.github.io/sml/) is available with an additional `Entity_Controller<SM, Data, Config>`, which owns a state machine (SM), its mutable data, and its per-instance config as a single stack object. sml injects `Data` and `Config` into action and guard lambdas by parameter type.

```cpp
// Events
struct Activate_Event {};
struct Hit_Event       {};
struct Render_Event    {};

// States
struct Idle   {};
struct Active {};
struct Dead   {};

// Per-instance mutable state
struct Enemy_Data { p_ui8 hp = 3; };

// Per-instance config (body ptr, sprite name, draw callback)
struct Enemy_Config {
    pulse2d::graphics::Body* body   = nullptr;
    const char*              sprite = nullptr;
    pulse2d::state::Draw_Fn  draw   = nullptr; // non-capturing fn ptr
};

struct enemy_sm {
    auto operator()() const {
        using namespace sml;

        auto will_die  = [](Enemy_Data const& d) { return d.hp <= 1; };
        auto on_hit    = [](Enemy_Data& d) { d.hp--; };
        auto on_render = [](Enemy_Config const& cfg) { cfg.draw(cfg.body, cfg.sprite); };
        auto on_death  = [](Enemy_Config const& cfg) { cfg.body->set_width({ 0.0f, 0.0f }); };

        return make_transition_table(
            *state<Idle>   + event<Activate_Event>                              = state<Active>,
             state<Active> + event<Render_Event>              / on_render,
             state<Active> + event<Hit_Event> [ will_die]  / on_hit             = state<Dead>,
             state<Active> + event<Hit_Event> [!will_die]  / on_hit,
             state<Dead>   + on_entry<_>                   / on_death
        );
    }
};

using Enemy = pulse2d_state::Entity_Controller<enemy_sm, Enemy_Data, Enemy_Config>;
```

```cpp
// scenes/levels/level_one.h
namespace scenes::levels::level_one {
// ...
PULSE_DEFINE Enemy enemy{};

PULSE_SCENE_FN void on_start(pulse2d_scene_runtime<Scenes...>& game,
    pulse2d::state::Draw_Fn draw_fn)
{
    enemy.configure({
        .body   = &game.get_body("enemy_object"),
        .sprite = "enemy_sprite",
        .draw   = draw_fn,
    });
    enemy.dispatch(Activate_Event{});
}

PULSE_SCENE_FN void on_tick(pulse2d_scene_runtime<Scenes...>& game)
{
    enemy.dispatch(Render_Event{});

    if (SEESAW_BUTTON_INPUT(SEESAW_A))
        enemy.dispatch(Hit_Event{});

    if (enemy.is<Dead>()) { /* respawn, score, etc. */ }
}
// ...
} // namespace scenes::levels::level_one
```

```cpp
// src/game.cc
namespace level_one = scenes::levels::level_one;

PULSE_ON_GAMESCENE_START(Level_One) {
    level_one::on_start(my_game,
        [](pulse2d_body* body, const char* sprite) { my_game.draw_body(body, sprite); });
}
```

`reset()` tears down and reconstructs the SM in-place with no heap allocation, so entities can be reused across rounds.

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

### Build:

### Host (tests)

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DUSE_SANITIZER="Address;Undefined" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

### Teensy

You can use the `Makefile.teensy` to build and flash:

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

# Tools

## Assets

Python tools for converting PNG assets are in `tools/`. The first three require [Pillow](https://pillow.readthedocs.io/).

- `png2bin` - converts a PNG to the raw RGB565 binary format read by `Storage::load_sprite` on Teensy. Use this for sprites (ships, enemies, projectiles) loaded from the SD card at runtime. Transparent pixels become `0xF81F` (magenta chroma-key).

  ```bash
  tools/png2bin sprite.png sprite.bin 64 64
  ```

- `png2header` - converts a PNG to a C header containing a flat RGB565 pixel array for use as a parallax background layer. Register the output with `set_background_sprite` and `add_parallax_layer`. Do not edit the generated file by hand.

  ```bash
  tools/png2header background.png include/nebula-bg.h bg_1 320 240
  ```

  Generates `bg_1_width`, `bg_1_height`, and `bg_1[320 * 240]`.

- `animation2header` - converts multiple PNGs to a C header containing a contiguous RGB565 pixel array for use as a spritesheet. Pass frames in order; the array is laid out sequentially.

  ```bash
  tools/animation2header include/laser-anim.h laser_anim 54 32 laser-sprite1.png laser-sprite2.png laser-sprite3.png
  ```

  Generates `laser_anim_width`, `laser_anim_height`, and `laser_anim[54 * 32 * 3]`.

- `imghelper` - validates `.bin` sprite files produced by `png2bin`. Checks declared dimensions, pixel count against `MAX_SPRITE_PIXELS` (96×96 = 9,216 pixels), and whether the file size matches the header. Accepts one or more files or a directory; exits 1 on failure.

  ```bash
  tools/imghelper sprite.bin
  tools/imghelper assets/
  ```

## Debug

Debugging tools:

- `sections` - parses a compiled ELF and prints every allocatable section grouped by memory region (FLASH, DTCM, RAM, ERAM, ITCM), with VMA, LMA, size in bytes, and load type (FLASH-ONLY, LOAD, or NOLOAD). Each region header shows total capacity and bytes / KiB in use. Regions that exceed their recommended static limit are printed in red with a warning line at the top.

  ```bash
  tools/sections build-teensy/asterisk.elf
  tools/sections build-teensy/asterisk.elf --all        # include sections < 64 bytes
  tools/sections build-teensy/asterisk.elf --min-bytes 1024
  # or via make:
  make sections
  make sections SECTIONS_ALL=1
  ```

  Run this after any build that changes pool sizes or static data. The linker only rejects when a region is fully exhausted - `sections` lets you catch pressure before it becomes a crash.

<img width="1266" height="854" alt="image" src="https://github.com/user-attachments/assets/458971e1-0fbf-47d1-9afe-db388e86afc5" />


---

- `ldscript` - parses `imxrt1062_t41.ld` from the active Teensyduino install and prints the MEMORY region table and the output section -> region map. The fastest way to answer "which section attribute keeps this array in flash?" without reading the raw linker script.

  ```bash
  tools/ldscript
  tools/ldscript --ld /path/to/imxrt1062_t41.ld   # explicit path
  # or via make:
  make ldscript
  ```

![img](/images/tool-ldscript.png)

## Dependencies

Host dependencies are fetched automatically via [CPM](https://github.com/cpm-cmake/CPM.cmake).

- [`ETLCPP`](https://www.etlcpp.com/) - Embedded Template Library
- [`boost/sml`](https://boost-ext.github.io/sml/) - State Machine Library
- `box2d-lite` - Heavily modified port of [box2d-lite](https://github.com/erincatto/box2d-lite) for embedded devices
- `doctest` - Test framework
- `stb` - Image loading for the host storage backend

## License

MIT License

