<div align="center">
  <img src="docs/logo/pulse2d-sleek-final-final-final.png" width="800" alt="pulse2d"> </img>
</div>


<h5 align="center">
  Teensy 4.1 2D Game Engine 🎮
</h5>


## Overview


The Teensy 4.1 is a microcontroller development board based on the NXP i.MX RT1062, an ARM Cortex-M7 running at up to 600 MHz. `pulse2d` enables you to turn the microcontroller into a 2D game platform with a display and controller, as it has hardware floating-point, a dedicated SPI bus, and a built-in SDIO SD card slot. 🎮

The project builds a sample game for desktop and the teensy hardware called `shift`.

### Demo:



https://github.com/user-attachments/assets/c17d19ef-d7f9-45c4-8563-98ffed3ee73e



## Hardware

The recommended hardware for game development with **no soldering required**:

- [Solderless breadboard](https://www.amazon.com/dp/B08Y59P6D1?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_5): Holds all components
- [Teensy 4.1](https://www.pjrc.com/store/teensy41.html): Primary microcontroller
- [ILI9341 TFT Display](https://www.pjrc.com/store/display_ili9341_touch.html): 320x240 RGB565 display via SPI
- [MicroSD card](https://www.amazon.com/dp/B0B7NV73PJ?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_4): Sprite and asset storage via the built-in SDIO slot
- [Adafruit Seesaw Gamepad QT](https://www.adafruit.com/product/5743): I2C gamepad with analog thumbstick and 6 buttons

---

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

Link against `pulse2d::pulse2d` in your `CMakeLists.txt`:

```cmake
add_subdirectory(pulse2d)          # or use CPM, FetchContent
target_link_libraries(my_game PRIVATE pulse2d::pulse2d)
```

---

### Build sample game: `shift`

The included sample game `shift` targets both SDL2 and Teensy 4.1:

### Host

```bash
# macOS
brew install sdl2

cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DUSE_SANITIZER="Address;Undefined" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSDL2_DIR=$(brew --prefix sdl2)/lib/cmake/SDL2
cmake --build build
./build/shift_game

# Ubuntu
sudo apt update
sudo apt install libsdl2-dev

cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
./build/shift_game


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

## Game Development

### DSL

The DSL is a set of macros in `pulse2d/dsl.h` inspired by the [Catch2](https://github.com/catchorg/Catch2/blob/85eb4652b46cc69c4ad7915c9fd3b009d99e9fb7/examples/120-Bdd-ScenarioGivenWhenThen.cpp#L15) library that enable development of a Teensy game. It wraps the engine, physics world, scene management, and render pipeline into a "fantasy" scripting language, without the need to understand bare-metal embedded programming.

See [the source code of the shift game](/shift/game-teensy.cc) for a full example.

A minimal game that spawns a couple of physics bodies and loads a few sprites:

```cpp
#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS

PULSE2D_START_PULSE();
PULSE2D_ENABLE_SEESAW_GAMEPAD();

PULSE2D_DEFINE_SCENE(Sample_Level, 2, 3);
PULSE2D_GAME_SCENES(Sample_Level);

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
    PULSE2D_POLL_SEESAW_GAMEPAD();

    // enable arcade-style controller movement
    SEESAW_ARCADE_DIRECTIONAL_MOVEMENT("spell", 5.22f);

    PULSE2D_ON_COLLISION()
    {
        if (!exploded)
            exploded = true;
    }

    // reset the game?
    if (SEESAW_BUTTON_INPUT(SEESAW_START))
        PULSE2D_SET_SCENE(Sample_Level);

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
    // poll for a serial connection
    PULSE2D_POLL_SERIAL_CONNECTION();
    PULSE2D_INIT(0.0f, 0.0f, 10);
    PULSE2D_SET_SCENE(Sample_Level);
}

PULSE2D_ON_GAMELOOP()
{
    PULSE2D_TICK_GAMESCENE();
}
```

#### Setup

- **`PULSE2D_START_PULSE()`** — declares the engine, physics world, and two pointers that control scene dispatch. Place this at file scope, once per game.
  ```cpp
  PULSE2D_START_PULSE();
  ```

- **`PULSE2D_DEFINE_SCENE(name, bodies, sprites[, joints])`** — declares a scene struct with fixed-size body, sprite, and optional joint pools. The provided sizes are checked at compile time against the hardware limits. The optional fourth argument sets the joint pool size (default: 0).
  ```cpp
  PULSE2D_DEFINE_SCENE(Game_Level, 4, 3);      // 4 bodies, 3 sprites
  PULSE2D_DEFINE_SCENE(Boss_Level, 8, 5, 2);   // explicit joint pool of 2
  ```

- **`PULSE2D_GAME_SCENES(...)`** — Takes a comma-separated list of all scene types used in the game.
  ```cpp
  PULSE2D_GAME_SCENES(Menu_Level, Game_Level, Boss_Level);
  ```

- **`PULSE2D_DEFINE`** — Use for any game state variables, which allocates in the correct section of memory.

  ```cpp
  PULSE2D_DEFINE bool player_dead = false;
  PULSE2D_DEFINE int score = 0;
  ```

#### Scene lifecycle

- **`PULSE2D_ON_GAMESCENE_START(scene)`** — defines the function entry for a scene. This is called automatically by `PULSE2D_SET_SCENE`. Spawn bodies and load sprites here.
  ```cpp
  PULSE2D_ON_GAMESCENE_START(Game_Level) {
      PULSE2D_SPAWN_BODY("player", { .position={0.f,0.f}, .mass=1.f });
  }
  ```

- **`PULSE2D_ON_GAMESCENE(scene)`** — defines the per-frame function for a scene. Registered as the active tick function by `PULSE2D_SET_SCENE`.
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
  PULSE2D_ON_GAMELOOP() {
    PULSE2D_TICK_GAMESCENE();
  }
  ```

To trigger a scene transition from inside a scene function, assign to `pending_transition`:

```cpp
pending_transition = []() { PULSE2D_SET_SCENE(Level_2); };
```

The transition runs at the end of the current frame, so the rest of the frame finishes cleanly first.

#### Physics and rendering

#### List of [all physics body properties](https://github.com/jahan-addison/pulse2d/blob/master/pulse2d/graphics/body.h#L95)

- **`PULSE2D_INIT(gx, gy, solver_iterations)`** — initializes the engine and physics world. `gx` and `gy` are the gravity vector components; the third argument is the solver iteration count.
  ```cpp
  PULSE2D_INIT(0.0f, -9.8f, 10);   // gravity pulls down
  PULSE2D_INIT(0.0f,  0.0f, 10);   // zero gravity
  ```

- **`PULSE2D_SPAWN_BODY(name, {...})`** — allocates a **dynamic** body in the current scene's pool, calls `set_motion()` to enable full physics simulation, and registers it with the world. The second argument is a `Body` aggregate with fields from the physics body.
  ```cpp
  PULSE2D_SPAWN_BODY("ball", {
    .position={0.f, 2.f},
    .velocity={1.f, 0.f},
    .mass=1.f
  });
  ```

- **`PULSE2D_SPAWN_STATIC_BODY(name, {...})`** — allocates a body in the current scene's pool and registers it with the world. `set_motion()` is not called, so the body is treated as an immovable obstacle by the solver.
  ```cpp
  PULSE2D_SPAWN_STATIC_BODY("floor", {
    .position={0.f, -5.f},
    .width={10.f,0.5f}
  });
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

#### Backgrounds

The background system blits full-screen sprites in layers each frame, the parallax feature scrolls each layer at its own speed. Sprites are loaded from C headers (see [`png2header`](#tools)) rather than the SD card, so they are available without any file I/O.

You can also blits a single background with no movement.

- **`PULSE2D_SET_SPRITE_FLASH(name, data_ptr, w, h)`** — registers a flash-resident sprite array as a named sprite in the current scene's sprite pool. `data_ptr` is the array symbol from the generated header. Call in `PULSE2D_ON_GAMESCENE_START`.
  ```cpp
  #include "../include/nebula-bg.h"   // generated by png2header

  PULSE2D_ON_GAMESCENE_START(Level_One) {
      PULSE2D_SET_SPRITE_FLASH(sprite_nebula, bg_1, 320, 240); // 320x240 is TFT display size
  }
  ```

- **`PULSE2D_ADD_BACKGROUND_LAYER(sprite_name, width)`** — adds a static background layer to the current scene. `width` is the image width in pixels (used to wrap the offset). Call after the corresponding `PULSE2D_SET_SPRITE_FLASH`.
  ```cpp
  PULSE2D_ADD_BACKGROUND_LAYER(sprite_nebula, 320.0f); // 320 is the TFT width
  ```

- **`PULSE2D_ADD_PARALLAX_LAYER(sprite_name, width, speed)`** — adds a parallax scroll layer to the current scene. `width` is the image width in pixels (used to wrap the offset), `speed` is pixels per second. Layers are drawn in the order they are added. Call after the corresponding `PULSE2D_SET_SPRITE_FLASH`.
  ```cpp
  PULSE2D_ADD_PARALLAX_LAYER(sprite_nebula, 320.0f, 10.0f);   // slow background
  PULSE2D_ADD_PARALLAX_LAYER(sprite_stars,  320.0f,  3.0f);   // very slow
  PULSE2D_ADD_PARALLAX_LAYER(sprite_dust,   320.0f, 65.0f);   // fast foreground
  ```

- **`PULSE2D_RENDER_BACKGROUNDS()`** — advances each layer's scroll offset and blits all layers to the framebuffer. Call before `PULSE2D_RENDER` in `PULSE2D_ON_GAMESCENE`.
  ```cpp
  PULSE2D_ON_GAMESCENE(Level_One) {
      PULSE2D_TICK_WORLD(Level_One);
      PULSE2D_POLL_SEESAW_GAMEPAD();
      PULSE2D_RENDER_BACKGROUNDS();
      PULSE2D_RENDER(active_scene);
  }
  ```

A four-layer parallax scene example:

```cpp
#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS

#include "../include/dust-bg.h"
#include "../include/nebula-bg.h"
#include "../include/planet-bg.h"
#include "../include/stars-bg.h"

PULSE2D_START_PULSE();
PULSE2D_ENABLE_SEESAW_GAMEPAD();

PULSE2D_DEFINE_SCENE(Level_One, 1, 4);   // 4 sprite slots for the backgrounds
PULSE2D_GAME_SCENES(Level_One);

PULSE2D_ON_GAMESCENE_START(Level_One)
{
    PULSE2D_SET_SPRITE_FLASH(sprite_nebula,  bg_1, 320, 240);
    PULSE2D_SET_SPRITE_FLASH(sprite_stars,   bg_2, 320, 240);
    PULSE2D_SET_SPRITE_FLASH(sprite_planets, bg_3, 320, 240);
    PULSE2D_SET_SPRITE_FLASH(sprite_dust,    bg_4, 320, 240);

    PULSE2D_ADD_PARALLAX_LAYER(sprite_nebula,  320.0f, 10.0f);
    PULSE2D_ADD_PARALLAX_LAYER(sprite_stars,   320.0f,  3.0f);
    PULSE2D_ADD_PARALLAX_LAYER(sprite_planets, 320.0f, 25.0f);
    PULSE2D_ADD_PARALLAX_LAYER(sprite_dust,    320.0f, 65.0f);
}

PULSE2D_ON_GAMESCENE(Level_One)
{
    PULSE2D_TICK_WORLD(Level_One);
    PULSE2D_POLL_SEESAW_GAMEPAD();
    PULSE2D_RENDER_BACKGROUNDS();
    PULSE2D_RENDER(active_scene);
}
```



https://github.com/user-attachments/assets/dc801a20-9eff-4baa-91d6-96b59e9bdb62



> **Memory note:** each 320×240 background is 150 KB of RGB565 data. The headers generated by `png2header` use `__attribute__((section(".progmem")))` to keep the arrays in the 8 MB QSPI flash — the Teensy 4.x linker script would otherwise copy all `.rodata` into DTCM at boot, exhausting the 512 KB data RAM immediately.

#### Collision

- **`PULSE2D_ON_COLLISION()`** — a conditional block that runs when at least one collision is active in the world.
  ```cpp
  PULSE2D_ON_COLLISION() {
    game_over = true;
  }
  ```

- **`PULSE2D_ON_COLLISION_WITH(name)`** — a conditional block that runs when a specific named arbiter is present.
  ```cpp
  PULSE2D_ON_COLLISION_WITH(wall) {
    bounce_count++;
  }
  ```

#### Engine

- **`PULSE2D_ON_GAMESTART()`** — maps to Arduino `setup()`.
  ```cpp
  PULSE2D_ON_GAMESTART() {
    Serial.begin(115200);
    PULSE2D_INIT(0.f, 0.f, 10);
  }
  ```

- **`PULSE2D_ON_GAMELOOP()`** — maps to Arduino `loop()`.
  ```cpp
  PULSE2D_ON_GAMELOOP() {
    PULSE2D_TICK_GAMESCENE();
  }
  ```

- **`PULSE2D_POLL_SERIAL_CONNECTION()`** — blocks until a serial connection is established, very useful during development for debug and initialization messages.
  ```cpp
  PULSE2D_ON_GAMESTART() {
    Serial.begin(115200);
    PULSE2D_POLL_SERIAL_CONNECTION();
    //...
  }
  ```

- **`PULSE2D_PRINT_STACKSIZE()`** — prints stack usage to serial every 300 frames. Compiled away in non-debug builds.
  ```cpp
  PULSE2D_ON_GAMESCENE(Game_Level) {
      PULSE2D_TICK_WORLD(Game_Level);
      PULSE2D_PRINT_STACKSIZE();   // prints ~every 5 s at 60 fps
      ...
  }
  ```

- **`PULSE2D_REGISTER_ETL_ERROR_HANDLER()`** — registers a Serial callback for ETL assertion failures. With `-fno-exceptions` (required by Teensyduino), ETL bounds violations are silent by default; this makes them print `[ETL ERROR] <file>:<line> <message>` to serial. Call once in `PULSE2D_ON_GAMESTART()` after `Serial.begin()`. Compiled away in non-debug builds.
  ```cpp
  PULSE2D_ON_GAMESTART() {
      Serial.begin(115200);
      PULSE2D_POLL_SERIAL_CONNECTION();
      PULSE2D_REGISTER_ETL_ERROR_HANDLER();
      PULSE2D_INIT(0.f, 0.f, 10);
  }
  ```

#### Gamepad

- **`PULSE2D_ENABLE_SEESAW_GAMEPAD()`** — declares the I2C driver and gamepad at file scope. Place this once alongside `PULSE2D_START_PULSE()`.
  ```cpp
  PULSE2D_START_PULSE();
  PULSE2D_ENABLE_SEESAW_GAMEPAD();
  ```

- **`PULSE2D_START_SEESAW_GAMEPAD()`** — initializes the I2C bus and gamepad hardware. Call once in `PULSE2D_ON_GAMESTART()`.
  ```cpp
  PULSE2D_ON_GAMESTART() {
      ...
      PULSE2D_START_SEESAW_GAMEPAD();
  }
  ```

- **`PULSE2D_POLL_SEESAW_GAMEPAD()`** — polls all inputs and brings `gamepad_state` into scope. Call at the top of each scene function.
  ```cpp
  PULSE2D_ON_GAMESCENE(Game_Level) {
      PULSE2D_TICK_WORLD(Game_Level);
      PULSE2D_POLL_SEESAW_GAMEPAD();
      ...
  }
  ```

- **`SEESAW_BUTTON_INPUT(name)`** — evaluates non-zero while the named button is held. Available names: `SEESAW_A`, `SEESAW_B`, `SEESAW_X`, `SEESAW_Y`, `SEESAW_START`, `SEESAW_SELECT`.
  ```cpp
  if (SEESAW_BUTTON_INPUT(SEESAW_A)) { fire(); }
  ```

- **`SEESAW_ARCADE_DIRECTIONAL_MOVEMENT(body_name, max_speed)`** — Profile A: The Arcade Controller (Pokémon, Zelda, Pac-Man). Sets the body's velocity directly from stick position, clamped to `max_speed`. Instant response, instant stop.
  ```cpp
  SEESAW_ARCADE_DIRECTIONAL_MOVEMENT("player", 3.0f);
  ```

- **`SEESAW_ARCADE_DIRECTIONAL_MOVEMENT_INVERTED(body_name, max_speed)`** — Profile A, inverted: Same as above with the X and Y axis inverted.
  ```cpp
  SEESAW_ARCADE_DIRECTIONAL_MOVEMENT_INVERTED("ship", 3.0f);
  ```

- **`SEESAW_DYNAMIC_DIRECTIONAL_MOVEMENT(body_name, acceleration)`** — Profile B: The Momentum Controller (Asteroids, Mario). Applies a thrust force scaled by `acceleration` each frame - velocity builds up over time.
  ```cpp
  SEESAW_DYNAMIC_DIRECTIONAL_MOVEMENT("ship", 0.8f);
  ```

- **`SEESAW_SLIDING_FRICTION_DIRECTIONAL_MOVEMENT(body_name, drag_amount)`** — Profile C: Top-Down Friction, applies linear drag each frame. Pair with `SEESAW_DYNAMIC_DIRECTIONAL_MOVEMENT` so the body doesn't slide forever.
  ```cpp
  SEESAW_DYNAMIC_DIRECTIONAL_MOVEMENT("ship", 0.8f);
  SEESAW_SLIDING_FRICTION_DIRECTIONAL_MOVEMENT("ship", 0.92f);
  ```

- **`SEESAW_DIRECTIONAL_X_INPUT()`** / **`SEESAW_DIRECTIONAL_Y_INPUT()`** — analog stick axes, normalized −1.0 to +1.0.
  ```cpp
  body.velocity.x = SEESAW_DIRECTIONAL_X_INPUT() * speed;
  ```

- **`SEESAW_DIRECTION_IS_LEFT()`** / **`SEESAW_DIRECTION_IS_RIGHT()`** / **`SEESAW_DIRECTION_IS_UP()`** / **`SEESAW_DIRECTION_IS_DOWN()`** — true when the stick is pushed more than halfway in the given direction.
  ```cpp
  if (SEESAW_DIRECTION_IS_UP()) { jump(); }
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

A minimal setup:

```cpp
PULSE2D_START_PULSE();
PULSE2D_ENABLE_SEESAW_GAMEPAD();

PULSE2D_ON_GAMESTART() {
    ...
    PULSE2D_START_SEESAW_GAMEPAD();
    PULSE2D_SET_SCENE(Game_Level);
}

PULSE2D_ON_GAMESCENE(Game_Level) {
    PULSE2D_TICK_WORLD(Game_Level);
    PULSE2D_POLL_SEESAW_GAMEPAD();

    SEESAW_ARCADE_DIRECTIONAL_MOVEMENT("player", 3.0f);

    if (SEESAW_BUTTON_INPUT(SEESAW_A)) { fire(); }
    if (SEESAW_BUTTON_INPUT(SEESAW_B)) { jump(); }

    PULSE2D_RENDER(active_scene);
}
```

---

# Tools

## Assets

Python tools for converting PNG assets are in `tools/` — both require [Pillow](https://pillow.readthedocs.io/).

- `png2bin` — converts a PNG to the raw RGB565 binary format read by `Storage::load_sprite` on Teensy. Use this for game-object sprites (ships, enemies, projectiles) that are loaded from the SD card at runtime. Transparent pixels become `0xF81F` (magenta chroma-key).

  ```bash
  tools/png2bin sprite.png sprite.bin 64 64
  ```

- `png2header` — converts a PNG to a C header containing an RGB565 pixel array for use as a full-screen background. The array is placed in `.progmem` via `__attribute__((section(".progmem")))`, which keeps it in QSPI flash on Teensy 4.x. Use `PULSE2D_SET_SPRITE_FLASH` and `PULSE2D_ADD_PARALLAX_LAYER` to register the header as a parallax layer. The output file is generated — do not edit it by hand.

  ```bash
  tools/png2header background.png include/nebula-bg.h bg_1 320 240
  ```

  Generates `bg_1_width`, `bg_1_height`, and `bg_1[320 * 240]`.

  > **Why `.progmem` and not `constexpr`?** The Teensy 4.x linker script places all `.rodata*` inside the `.data` output section, which is copied from QSPI flash into DTCM at boot. A single 320×240 background is 150 KB; four layers would overflow the 512 KB DTCM before `setup()` runs. `.progmem*` is a separate output section in the linker script that maps directly to FLASH and is never copied.

## Debug

Debugging tools (no dependencies):

- `sections` — reads a compiled ELF and prints a sorted table of every output section grouped by memory region (FLASH / DTCM / RAM / ERAM), with VMA, LMA, size, and whether the section is copied to RAM at boot or flash-only. Useful for catching oversized `.data` or `.bss` before the linker rejects the binary.

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

- `ldscript` — parses `imxrt1062_t41.ld` from the active Teensyduino install and prints the MEMORY region table and the output section → region map. The quickest way to answer "which section attribute keeps this array in flash?" without reading the raw linker script.

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

