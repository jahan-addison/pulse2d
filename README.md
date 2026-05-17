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

## Building

### C++

Link against `pulse2d::pulse2d` in your `CMakeLists.txt`:

```cmake
add_subdirectory(pulse2d)          # or use CPM, FetchContent
target_link_libraries(my_game PRIVATE pulse2d::pulse2d)
```

### NodeJS

> NodeJS bindings coming soon.

## Game Development

### DSL

The DSL is a set of macros in `pulse2d/dsl.h` inspired by the [Catch2](https://github.com/catchorg/Catch2/blob/85eb4652b46cc69c4ad7915c9fd3b009d99e9fb7/examples/120-Bdd-ScenarioGivenWhenThen.cpp#L15) library that enable development of a Teensy game. It wraps the engine, physics world, scene management, and render pipeline into a "fantasy" scripting language, without the need to understand bare-metal embedded programming.

A minimal game that spawns a couple of physics bodies and loads a few sprites:

```cpp
#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS

PULSE2D_START_PULSE();

PULSE2D_DEFINE_LEVEL(Sample_Level, 2, 3);

PULSE2D_GAME_LEVELS(Sample_Level);

PULSE2D_DEFINE bool exploded = false;

PULSE2D_ON_GAMESCENE_START(Sample_Level)
{
    PULSE2D_SPAWN_STATIC_BODY("planet",
        {
            .position = { 3.5f, 0.0f },
            .width    = { 1.0f, 1.0f }
    });

    PULSE2D_SPAWN_BODY("spell",
        {
            .position = { -5.0f, -0.1111f },
            .velocity = {  3.5f,  0.0f    },
            .width    = {  1.0f,  0.5f    },
            .mass     = 1.0f
    });

    PULSE2D_SET_SPRITE(planet_sprite, "planet.bin", 96, 96);
    PULSE2D_SET_SPRITE(spell_sprite, "spell.bin", 64, 36);
    PULSE2D_SET_SPRITE(explode_sprite, "explosion.bin", 96, 96);
}

PULSE2D_ON_GAMESCENE(Sample_Level)
{
    PULSE2D_TICK_WORLD(Sample_Level);
    PULSE2D_ON_COLLISION()
    {
        if (!exploded)
            exploded = true;
    }

    PULSE2D_PRINT_STACKSIZE();

    if (exploded)
        PULSE2D_DRAW("planet", explode_sprite);
    else
        PULSE2D_DRAW("planet", planet_sprite);

    PULSE2D_DRAW("spell", spell_sprite, 3.111f);
    PULSE2D_RENDER(active_scene);
}

PULSE2D_ON_GAMESTART()
{
    Serial.begin(115200);
    PULSE2D_POLL_SERIAL_CONNECTION();
    PULSE2D_INIT(0.0f, 0.0f, 10);
    PULSE2D_SET_SCENE(Sample_Level);
}

PULSE2D_ON_GAMELOOP()
{
    PULSE2D_TICK_GAMESCENE();
}
```

Check out the result [here](/docs/palse2d-sample-bodies.gif).

#### Setup

- **`PULSE2D_START_PULSE()`** — declares the engine, physics world, and two pointers that control scene dispatch. Place this at file scope, once per translation unit.
  ```cpp
  PULSE2D_START_PULSE();
  ```

- **`PULSE2D_DEFINE_LEVEL(name, bodies, sprites[, joints])`** — declares a scene struct with fixed-size body, sprite, and optional joint pools. Capacities are checked at compile time against the hardware limits. The optional fourth argument sets the joint pool size (default: 0).
  ```cpp
  PULSE2D_DEFINE_LEVEL(Game_Level, 4, 3);      // 4 bodies, 3 sprites
  PULSE2D_DEFINE_LEVEL(Boss_Level, 8, 5, 2);   // explicit joint pool of 2
  ```

- **`PULSE2D_GAME_LEVELS(...)`** — declares the `variant` that holds the current scene. Takes a comma-separated list of all scene types used in the game.
  ```cpp
  PULSE2D_GAME_LEVELS(Menu_Level, Game_Level, Boss_Level);
  ```

- **`PULSE2D_DEFINE`** — Use for any game state variables, allocates in the correct section of memory.

  ```cpp
  PULSE2D_DEFINE bool player_dead = false;
  PULSE2D_DEFINE int score = 0;
  ```

#### Scene lifecycle

- **`PULSE2D_ON_GAMESCENE_START(SceneName)`** — defines the function entry for a scene. Called automatically by `PULSE2D_SET_SCENE`. Spawn bodies and load sprites here.
  ```cpp
  PULSE2D_ON_GAMESCENE_START(Game_Level) {
      PULSE2D_SPAWN_BODY("player", { .position={0.f,0.f}, .mass=1.f });
  }
  ```

- **`PULSE2D_ON_GAMESCENE(SceneName)`** — defines the per-frame function for a scene. Registered as the active tick function by `PULSE2D_SET_SCENE`.
  ```cpp
  PULSE2D_ON_GAMESCENE(Game_Level) {
      PULSE2D_TICK_WORLD(Game_Level);
      PULSE2D_DRAW("player", player_sprite);
      PULSE2D_RENDER(active_scene);
  }
  ```

- **`PULSE2D_SET_SCENE(scene)`** — transitions to a scene. Clears the physics world, resets storage, resets the global body and sprite counters, and then calls the scene's entry function before registering its tick function.
  ```cpp
  PULSE2D_SET_SCENE(Game_Level);
  ```

- **`PULSE2D_TICK_GAMESCENE()`** — calls the active scene's tick function, then resolves any pending transition. This is the only call needed in the game loop.
  ```cpp
  PULSE2D_ON_GAMELOOP() { PULSE2D_TICK_GAMESCENE(); }
  ```

To trigger a scene transition from inside a scene function, assign to `pending_transition`:

```cpp
pending_transition = []() { PULSE2D_SET_SCENE(Level_2); };
```

The transition runs at the end of the current frame, so the rest of the frame finishes cleanly first.

#### Physics and rendering

- **`PULSE2D_INIT(gx, gy, solver_iterations)`** — initializes the engine and physics world. `gx` and `gy` are the gravity vector components; the third argument is the solver iteration count.
  ```cpp
  PULSE2D_INIT(0.0f, -9.8f, 10);   // gravity pulls down
  PULSE2D_INIT(0.0f,  0.0f, 10);   // zero gravity
  ```

- **`PULSE2D_SPAWN_BODY(name, {...})`** — allocates a **dynamic** body in the current scene's pool, calls `set_motion()` to enable full physics simulation, and registers it with the world. The second argument is a `Body_Descriptor` aggregate with fields `position`, `velocity`, `width`, and `mass`.
  ```cpp
  PULSE2D_SPAWN_BODY("ball", { .position={0.f,2.f}, .velocity={1.f,0.f}, .mass=1.f });
  ```

- **`PULSE2D_SPAWN_STATIC_BODY(name, {...})`** — allocates a **static** body in the current scene's pool and registers it with the world. `set_motion()` is not called, so the body is treated as an immovable obstacle by the solver.
  ```cpp
  PULSE2D_SPAWN_STATIC_BODY("floor", { .position={0.f,-5.f}, .width={10.f,0.5f} });
  ```

- **`PULSE2D_TICK_WORLD(SceneName)`** — steps the physics simulation one frame and brings `active_scene` and `renderer` into scope for the rest of the scene function. Call this at the top of `PULSE2D_ON_GAMESCENE`.
  ```cpp
  PULSE2D_ON_GAMESCENE(Game_Level) {
      PULSE2D_TICK_WORLD(Game_Level);
      // active_scene and renderer now in scope
  }
  ```

- **`PULSE2D_SET_SPRITE(name, path, w, h)`** — loads a raw sprite file from the SD card into the current scene's sprite pool. `path` is relative to the SD root; `w` and `h` are pixel dimensions.
  ```cpp
  PULSE2D_SET_SPRITE(hero_sprite, "hero.bin", 32, 32);
  ```

- **`PULSE2D_DRAW(body_name, sprite_name[, angle_rad])`** — projects a body's world-space position to screen coordinates and queues the sprite for rendering. An optional third argument sets a fixed rotation in radians. Requires `active_scene` and `renderer` in scope (after `PULSE2D_TICK_WORLD`).
  ```cpp
  PULSE2D_DRAW("planet", planet_sprite);           // no rotation
  PULSE2D_DRAW("comet",  comet_sprite, 1.5708f);   // fixed 90° rotation
  ```

- **`PULSE2D_RENDER(active_scene)`** — flushes the renderer's sprite queue to the display.
  ```cpp
  PULSE2D_RENDER(active_scene);
  ```

- **`PULSE2D_GET_BODY(name)`** — returns a reference to a named body from `active_scene`. Available after `PULSE2D_TICK_WORLD`.
  ```cpp
  auto& ship = PULSE2D_GET_BODY("ship");
  ship.velocity.x += thrust;
  ```

#### Collision

- **`PULSE2D_ON_COLLISION()`** — a conditional block that runs when at least one collision is active in the world.
  ```cpp
  PULSE2D_ON_COLLISION() { game_over = true; }
  ```

- **`PULSE2D_ON_COLLISION_WITH(name)`** — a conditional block that runs when a specific named arbiter is present.
  ```cpp
  PULSE2D_ON_COLLISION_WITH(wall) { bounce_count++; }
  ```

#### Utilities

- **`PULSE2D_ON_GAMESTART()`** — maps to Arduino `setup()`.
  ```cpp
  PULSE2D_ON_GAMESTART() { Serial.begin(115200); PULSE2D_INIT(0.f, 0.f, 10); }
  ```

- **`PULSE2D_ON_GAMELOOP()`** — maps to Arduino `loop()`.
  ```cpp
  PULSE2D_ON_GAMELOOP() { PULSE2D_TICK_GAMESCENE(); }
  ```

- **`PULSE2D_POLL_SERIAL_CONNECTION()`** — blocks until a serial connection is established. Useful during development.
  ```cpp
  PULSE2D_ON_GAMESTART() { Serial.begin(115200); PULSE2D_POLL_SERIAL_CONNECTION(); ... }
  ```

- **`PULSE2D_PRINT_STACKSIZE()`** — prints stack usage to serial every 300 frames. Compiled away in non-debug builds.
  ```cpp
  PULSE2D_ON_GAMESCENE(Game_Level) {
      PULSE2D_TICK_WORLD(Game_Level);
      PULSE2D_PRINT_STACKSIZE();   // prints ~every 5 s at 60 fps
      ...
  }
  ```

---

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

---

### Sample game

The included sample game `shift` targets both SDL2 and Teensy 4.1:

### Host

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSDL2_DIR=$(brew --prefix sdl2)/lib/cmake/SDL2
cmake --build build
./build/shift_game
```

### Teensy

```bash
make -f Makefile.teensy -j
make -f Makefile.teensy flash
```

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

Load sprites via `Storage::load_sprite()`. On the host any image format supported by stb_image works. The image is nearest-neighbour scaled to the requested dimensions and converted to RGB565:

On Teensy, `load_sprite` reads the raw binary format (`uint16_t` width, `uint16_t` height, then `width x height` RGB565 pixels) from the SD card.

* TODO

The storage component will also enable loading of sounds and audio files.

## Audio

* TODO

## Physics

The physics component is a port of [box2d-lite](https://github.com/erincatto/box2d-lite) modified for embedded use: dynamic allocation replaced with fixed-size containers, all math in single-precision float, and the solver tuned for the Teensy 4.1's Cortex-M7.


For more details, see the [physics readme](pulse2d/graphics/readme.md).


## Renderer

The `Renderer` holds the full-screen RGB565 framebuffer for razterization and blitting. Each frame runs clear, draw, and render.


## Dependencies

Host dependencies are fetched automatically via [CPM](https://github.com/cpm-cmake/CPM.cmake).

- [`ETLCPP`](https://www.etlcpp.com/) - Embedded Template Library
- `box2d-lite` - Heavily modified port of [box2d-lite](https://github.com/erincatto/box2d-lite) for embedded devices
- `doctest` - Test framework
- `stb` - Image loading for the host storage backend
- `SDL2` - Display driver for host development

## License

MIT License

