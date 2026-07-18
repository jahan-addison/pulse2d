# Overview

<div class="pulse-intro">
  <p>A 2D game engine for the Teensy 4.1. Games are organized into scenes with physics bodies, sprite pools, animations, and entity state machines, all with a clean API and DSL.</p>
  <video poster="assets/demo-poster.jpg" autoplay muted loop playsinline>
    <source src="https://jahan-aws-cdn.s3.us-east-1.amazonaws.com/Asterisk+Game+Demo+V6/demo.mp4" type="video/mp4">
  </video>
</div>

## Quick Start

Include both engine headers at the top of your game file:

```cpp
#include PULSE2D_HEADER   // expands to "pulse2d/core.h", pulls in the API and DSL
#include PULSE2D_GRAPHICS // expands to "pulse2d/graphics/all.h"
```

Initialize the game, declare a scene, and the Arduino entry points:

```cpp
PULSE2D_START_PULSE();
PULSE_DEFINE_SCENE(My_Scene, 8, 12);
PULSE_INIT_GAME(my_game, My_Scene);

PULSE_ON_GAMESTART()
{
    my_game.init(0.0f, 0.0f, 10);
    PULSE_ENABLE_SEESAW_GAMEPAD();
    PULSE_SET_SCENE(my_game, My_Scene);
}

PULSE_ON_GAMELOOP()
{
    PULSE_TICK_GAMESCENE();
}
```

Then build your game with the provided Makefile.teensy:

```make
PULSE2D_ROOT = external/pulse2d
GAME_SRCS    = src/game.cc
GAME_NAME    = Game

include $(PULSE2D_ROOT)/Makefile.teensy
```

See the [API Reference](runtime.md) for a complete description of all macros and runtime methods.

Check out the pilot game [asterisk](https://github.com/jahan-addison/asterisk) for a feature-complete example.

## Features

- **Scene management** - organize game states as scenes with isolated body and sprite pools
- **Physics management** - static, controlled, and dynamic objects with collision detection
- **Sprite rendering** - SD card and embedded sprites with RGB565 output to an ILI9341 display
- **Parallax backgrounds** - multiple scrolling layers composited in draw order
- **Kinematic pools** - pre-allocated object pools for projectiles and particles
- **Text** - Text writing API via `glcdfont.h` with extensive color selection
- **Animation system** - persistent frame animators and one-shot VFX
- **Gamepad input** - Adafruit Seesaw QT over I2C with three movement profiles
- **State machines** - boost.sml with zero allocation for complex entity behavior
- **Audio** - SGTL5000 codec with two independent SFX channels and looping music

## Hardware

Pulse2D targets the Teensy 4.1 with:

- [Teensy 4.1](https://www.pjrc.com/store/teensy41.html) - 600 MHz ARM Cortex-M7, 512 kB RAM, 8 MB flash
- [ILI9341 TFT Display](https://www.pjrc.com/store/display_ili9341_touch.html) - 320x240 RGB565 via SPI
- [Seesaw Gamepad QT](https://www.adafruit.com/product/5743) - I2C gamepad with thumbstick and 6 buttons
- [MicroSD card](https://www.amazon.com/dp/B0B7NV73PJ) - sprite storage via the built-in SDIO slot

## Building

See the [README](https://github.com/jahan-addison/pulse2d#building) for toolchain requirements and build instructions.
