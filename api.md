# API Reference

Game development in pulse2d is split into two layers. The **Internal DSL** provides the structural skeleton: type aliases, lifecycle hooks, scene declarations, and animation blueprints. The **Core API** provides `Runtime<Scenes...>`, which owns the engine, physics world, and active scene and exposes all game actions as methods.

Include both headers:

```cpp
#include PULSE2D_HEADER   // expands to "pulse2d/core.h", pulls in API, DSL
#include PULSE2D_GRAPHICS // expands to "pulse2d/graphics/all.h"
```

---

## Table of Contents

- [Internal DSL](#internal-dsl)
  - [Type Aliases](#type-aliases)
  - [Math](#math)
  - [Game State, Setup](#game-state-setup)
  - [Scenes](#scenes)
  - [Gamepad Input (DSL)](#gamepad-input-dsl)
  - [Animation (DSL)](#animation-dsl)
  - [Coordinates](#coordinates)
  - [Debug, Utilities](#debug-utilities)
- [Core API - Runtime](#core-api---runtime)
  - [Initialization](#initialization)
  - [Engine](#engine)
  - [Bodies](#bodies)
  - [Sprites, Rendering](#sprites-rendering)
  - [Backgrounds](#backgrounds)
  - [Animations (Runtime)](#animations-runtime)
  - [Kinematic Pools](#kinematic-pools)
  - [Collision](#collision)
  - [Gamepad Controls (Runtime)](#gamepad-controls-runtime)
- [Complete Example](#complete-example)
- [See Also](#see-also)

---

## Internal DSL

The Internal DSL is in `pulse2d/dsl.h` and provides the structural skeleton of a game: type aliases, lifecycle entry points, scene declarations, animation blueprints, and debug helpers. These are macros and free functions that do not require the engine or physics world at the call site.

---

### Type Aliases

Short names for engine types used throughout game code:

```cpp
using pulse2d_body    = pulse2d::graphics::Body;
using pulse2d_world   = pulse2d::graphics::World;
using pulse2d_arbiter = pulse2d::graphics::Arbiter;
using pulse2d_joint   = pulse2d::graphics::Joint;
```

Portable primitive aliases - useful for pool lambda parameters and state variables:

```cpp
using p_bool = bool;
using p_float = float;

using p_uint32 = uint32_t;
using p_uint16 = uint16_t;
using p_uint8  = uint8_t;
using p_int32  = int32_t;
using p_int16  = int16_t;
using p_int8   = int8_t;
```

---

### Math

`pulse2d_math` is an alias for `pulse2d::graphics::math`, which provides `Vec2`, `Mat22`, and random number helpers.

`pulse2d_math::random()` returns a float in `[-1, 1]`. `pulse2d_math::random(lo, hi)` returns a float in `[lo, hi]`. Both use the hardware RNG on Teensy and `arc4random` on host:

```cpp
// Spawn a bullet with a small random vertical spread
my_game.spawn("laser_ammo",
    250,
    ship.position.x + 1.0f,
    ship.position.y + pulse2d_math::random(-0.3f, 0.3f),
    30.0f,
    0.0f);

// Random directional kick on spawn
float angle = pulse2d_math::random(-0.5f, 0.5f);
pulse2d_math::Vec2 dir = { 1.0f, angle };
```

Explicit narrowing casts - use these when passing arithmetic results to APIs that take a specific integer width:

```cpp
to_uint32(x)   // static_cast<uint32_t>(x)
to_uint16(x)   // static_cast<uint16_t>(x)
to_uint8(x)    // static_cast<uint8_t>(x)
to_int32(x)    // static_cast<int32_t>(x)
to_int16(x)    // static_cast<int16_t>(x)
to_int8(x)     // static_cast<int8_t>(x)
```

Use `to_int16()` when doing arithmetic on screen coordinates before passing them to `my_game.play_vfx()`, since integer promotion widens the result:

```cpp
auto coords = get_body_coordinates(laser_object);
my_game.play_vfx("explosion", to_int16(coords.x + 8), to_int16(coords.y - 8));
```

---

### Game State, Setup

#### PULSE_DEFINE

```cpp
PULSE_DEFINE type variable_name = initial_value;
```

Allocates a static variable in the correct memory section for game state. Use this for any persistent game variables.

**Scope:** `global`

```cpp
PULSE_DEFINE bool game_over = false;
PULSE_DEFINE int score = 0;
PULSE_DEFINE float player_health = 100.0f;
```

---

#### PULSE_HARDWARE_DEFINE

```cpp
PULSE_HARDWARE_DEFINE(Type) variable_name;
```

Declares a hardware-deferred type for late initialization. Used internally. You typically don't need this directly.

**Scope:** `global`

---

#### PULSE2D_START_PULSE

```cpp
PULSE2D_START_PULSE();
```

Declares the two function pointers that drive scene dispatch: `pending_transition` and `active_scene_fn`. Place this at file scope, once per game.

**Scope:** `global`

```cpp
PULSE2D_START_PULSE();
PULSE_INIT_GAME(my_game, Level_One, Level_Two, Main_Menu);
```

---

#### PULSE_INIT_GAME

```cpp
PULSE_INIT_GAME(game_name, Scene1, Scene2, ...);
```

Creates the `Runtime<Scenes...>` instance that owns the engine, physics world, and active scene variant. `game_name` becomes a global variable you use to call all Core API methods.

**Scope:** `global`

```cpp
PULSE2D_START_PULSE();
PULSE_INIT_GAME(my_game, Space_Shooter);
```

After this, `my_game.init()`, `my_game.draw()`, `my_game.spawn()`, etc. are available.

---

#### PULSE_ON_GAMESTART / PULSE_ON_GAMELOOP

```cpp
PULSE_ON_GAMESTART() {
    // initialization - maps to Arduino setup()
}

PULSE_ON_GAMELOOP() {
    // per-frame - maps to Arduino loop()
}
```

Map to Arduino `setup()` and `loop()`. The game loop body is almost always just `PULSE2D_TICK_GAMESCENE()`.

**Scope:** `global`

```cpp
PULSE_ON_GAMESTART() {
    Serial.begin(115200);
    pulse_register_etl_error_handler();
    my_game.init(0.0f, 0.0f, 10);
    start_seesaw_gamepad();
    PULSE_SET_SCENE(my_game, Main_Menu);
}

PULSE_ON_GAMELOOP() {
    PULSE2D_TICK_GAMESCENE();
}
```

---

### Scenes

Scenes are the organizational unit for game levels, menus, and states. Each scene has its own pools of bodies, sprites, joints, and animations.

---

#### PULSE_DEFINE_SCENE

```cpp
PULSE_DEFINE_SCENE(scene_name, max_bodies, max_sprites);
PULSE_DEFINE_SCENE(scene_name, max_bodies, max_sprites, max_joints);
```

Declares a scene struct with fixed-size pools. Sizes are checked at compile time against hardware limits. The optional fourth argument sets the joint pool size (default: 0).

**Scope:** `global`

```cpp
PULSE_DEFINE_SCENE(Main_Menu, 2, 5);
PULSE_DEFINE_SCENE(Game_Level, 10, 8);
PULSE_DEFINE_SCENE(Boss_Fight, 15, 12, 4);  // 4 joints
```

---

#### PULSE_SET_SCENE

```cpp
PULSE_SET_SCENE(game, scene_name);
```

Transitions to a scene:
1. Clears the physics world
2. Resets the storage system
3. Emplaces the new scene into `game.current_scene`
4. Calls the scene's entry function (`PULSE_ON_GAMESCENE_START`)
5. Registers the scene's tick function

**Scope:** `PULSE_ON_GAMESTART`, `PULSE_ON_GAMESCENE`

```cpp
PULSE_SET_SCENE(my_game, Game_Level);
```

---

#### PULSE_DEFER_SCENE

```cpp
PULSE_DEFER_SCENE(scene_name);
```

Defers a scene transition to the end of the current frame tick. Safe to call from inside a scene function.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
PULSE_ON_GAMESCENE(Level_One) {
    // ...
    if (player_dead) {
        PULSE_DEFER_SCENE(Game_Over);
    }
}
```

---

#### PULSE_ON_GAMESCENE_START

```cpp
PULSE_ON_GAMESCENE_START(scene_name) {
    // initialization code
}
```

Defines the entry function for a scene. Called once automatically by `PULSE_SET_SCENE`. This is where you load sprites, create bodies, and initialize pools.

**Scope:** `global`

```cpp
PULSE_ON_GAMESCENE_START(Game_Level) {
    my_game.set_sprite("player_sprite", "player.bin", 32, 32);
    my_game.set_controlled_body("player", {
        .position = { 0.0f, 0.0f },
        .width    = { 0.5f, 0.5f }
    });
    my_game.set_static_body("floor", {
        .position = { 0.0f, -5.0f },
        .width    = { 10.0f, 0.5f }
    });
}
```

---

#### PULSE_ON_GAMESCENE

```cpp
PULSE_ON_GAMESCENE(scene_name) {
    // per-frame logic
}
```

Defines the per-frame function for a scene. Registered as the active tick function by `PULSE_SET_SCENE` and called every frame by `PULSE2D_TICK_GAMESCENE`.

**Scope:** `global`

```cpp
PULSE_ON_GAMESCENE(Game_Level) {
    my_game.tick();
    PULSE_POLL_SEESAW_GAMEPAD();

    my_game.set_arcade_directional_control("player", 3.0f);
    my_game.draw("player", "player_sprite");
    my_game.render();
}
```

---

#### PULSE2D_TICK_GAMESCENE

```cpp
PULSE2D_TICK_GAMESCENE();
```

Calls the active scene's tick function, then resolves any pending transition. This is the only call needed in `PULSE_ON_GAMELOOP`.

**Scope:** `PULSE_ON_GAMELOOP`

```cpp
PULSE_ON_GAMELOOP() {
    PULSE2D_TICK_GAMESCENE();
}
```

---

### Gamepad Input (DSL)

The gamepad system supports the [Adafruit Seesaw Gamepad QT](https://www.adafruit.com/product/5743) with analog thumbstick and 6 buttons over I2C. The hardware globals (`driver`, `pad`) are declared automatically when `PULSE2D_HEADER` is included, unless `PULSE2D_DISABLE_GAMEPAD` is defined before inclusion.

---

#### start_seesaw_gamepad

```cpp
start_seesaw_gamepad();
```

Initializes the I2C bus and gamepad hardware. Call once in `PULSE_ON_GAMESTART`.

**Scope:** `PULSE_ON_GAMESTART`

```cpp
PULSE_ON_GAMESTART() {
    Serial.begin(115200);
    my_game.init(0.0f, 0.0f, 10);
    start_seesaw_gamepad();
    PULSE_SET_SCENE(my_game, Main_Menu);
}
```

---

#### PULSE_POLL_SEESAW_GAMEPAD

```cpp
PULSE_POLL_SEESAW_GAMEPAD();
```

Polls all inputs for the current frame and brings `gamepad_state` into scope for the rest of the scene function.

**Scope:** `PULSE_ON_GAMESCENE`

---

#### Button Constants

```cpp
SEESAW_A
SEESAW_B
SEESAW_X
SEESAW_Y
SEESAW_START
SEESAW_SELECT
```

---

#### SEESAW_BUTTON_INPUT

```cpp
SEESAW_BUTTON_INPUT(button_name)
```

Evaluates non-zero while the named button is held. Requires `PULSE_POLL_SEESAW_GAMEPAD` to have been called.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
if (SEESAW_BUTTON_INPUT(SEESAW_A)) { fire(); }
if (SEESAW_BUTTON_INPUT(SEESAW_START)) { pause_game(); }
```

---

#### Analog Stick

```cpp
float x = SEESAW_DIRECTIONAL_X_INPUT();  // -1.0 to +1.0
float y = SEESAW_DIRECTIONAL_Y_INPUT();  // -1.0 to +1.0
```

Raw analog stick axes, normalized from −1.0 to +1.0.

```cpp
// Manual velocity from raw stick
pulse2d_body& ship = my_game.get_body("ship");
ship.set_velocity({
    SEESAW_DIRECTIONAL_X_INPUT() * 5.0f,
    SEESAW_DIRECTIONAL_Y_INPUT() * 5.0f
});
```

---

#### Direction Helpers

```cpp
SEESAW_DIRECTION_IS_LEFT()   // stick_x < -0.5
SEESAW_DIRECTION_IS_RIGHT()  // stick_x > +0.5
SEESAW_DIRECTION_IS_UP()     // stick_y < -0.5
SEESAW_DIRECTION_IS_DOWN()   // stick_y > +0.5
```

Boolean helpers that return true when the stick is pushed more than halfway in the given direction.

```cpp
if (SEESAW_DIRECTION_IS_LEFT() || SEESAW_DIRECTION_IS_RIGHT()) {
    register_animation(player_anim, anim_walk);
} else {
    register_animation(player_anim, anim_idle);
}
```

---

### Animation (DSL)

The DSL provides compile-time animation blueprints and persistent animator instances. These are the _definition_ side; the _playback_ side is on the Runtime (see [Animations (Runtime)](#animations-runtime)).

---

#### PULSE_DEFINE_ANIMATOR

```cpp
PULSE_DEFINE_ANIMATOR(animator_name);
```

Declares a named `Sprite_Animator` at file scope. The instance persists across frames and scenes.

**Scope:** `global`

```cpp
PULSE2D_START_PULSE();
PULSE_INIT_GAME(my_game, Platformer);

PULSE_DEFINE_ANIMATOR(player_anim);
```

---

#### PULSE_ANIMATION_DEFINITION

```cpp
PULSE_ANIMATION_DEFINITION(name, sheet_ptr, frame_width, frame_height, total_frames, fps);
```

Defines an immutable animation blueprint as a `static constexpr Animation_Def`. The `time_per_frame` value (`1.0f / fps`) is computed at compile time.

**Scope:** `global`

```cpp
#include "../include/player-anim.h"  // idle_frames[], walk_frames[]

PULSE_ANIMATION_DEFINITION(anim_idle, idle_frames, 32, 48, 4, 8);
PULSE_ANIMATION_DEFINITION(anim_walk, walk_frames, 32, 48, 6, 12);
```

---

#### register_animation

```cpp
register_animation(animator_inst, anim_def);
```

Loads an animation definition into a running animator. Resets the accumulator and plays from frame 0 on the next `my_game.tick_animation()` call.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
if (SEESAW_DIRECTION_IS_LEFT() || SEESAW_DIRECTION_IS_RIGHT()) {
    register_animation(player_anim, anim_walk);
} else {
    register_animation(player_anim, anim_idle);
}
```

---

### Coordinates

#### get_body_coordinates

```cpp
auto coords = get_body_coordinates(body_ptr);
// coords.x and coords.y are int16_t pixel coordinates
```

Projects a physics body's world-space position to screen pixel coordinates. Returns a `Renderer::Screen` struct with `int16_t x` and `int16_t y`. Use as an expression - assign to `auto` and access the fields directly.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
// Play VFX at a body's screen position
auto coords = get_body_coordinates(&my_game.get_body("enemy"));
my_game.play_vfx("explosion", coords.x, coords.y);

// With arithmetic offset - use to_int16() to avoid integer promotion
auto coords = get_body_coordinates(laser_ptr);
my_game.play_vfx("laser_hit", to_int16(coords.x + 12), to_int16(coords.y - 8));
```

---

### Debug, Utilities

#### pulse_print_stacksize

```cpp
pulse_print_stacksize();
```

Prints stack usage to serial every 300 frames (~5 seconds at 60 fps). Compiled away in non-debug builds.

**Scope:** `PULSE_ON_GAMESCENE`

```
stack used: 8192 bytes
stack used: 8256 bytes
```

---

#### pulse_register_etl_error_handler

```cpp
pulse_register_etl_error_handler();
```

Registers a Serial callback for ETL assertion failures. With `-fno-exceptions` (required by Teensyduino), ETL bounds violations are silent by default. This makes them visible:

```
[ETL] Error in scene.h:42 with 'map full'
```

Call once in `PULSE_ON_GAMESTART()` after `Serial.begin()`. Compiled away in non-debug builds.

---

## Core API - Runtime

`Runtime<Scenes...>` is a template struct declared in `pulse2d/api.h`. It owns the engine, physics world, and active scene, and exposes all game actions as methods. Instantiate it with `PULSE_INIT_GAME`:

```cpp
PULSE2D_START_PULSE();
PULSE_INIT_GAME(my_game, Level_One, Level_Two, Boss_Fight);
```

This creates:
- `my_game.engine` - hardware-deferred `Pulse2d` engine instance
- `my_game.world` - hardware-deferred physics `World` instance
- `Runtime<...>::current_scene` - `std::variant<std::monostate, Level_One, Level_Two, Boss_Fight>`

All methods that operate on named objects (bodies, sprites, pools) use `execute_scene` internally, which visits the active scene variant. If no scene is active (`std::monostate`), calls are silently skipped.

---

### Initialization

#### init

```cpp
my_game.init(float gravity_x, float gravity_y, int solver_iterations);
```

Constructs the engine and physics world in-place and calls hardware init. Call once in `PULSE_ON_GAMESTART`.

**Parameters:**
- `gravity_x` - horizontal gravity (typically `0.0f`)
- `gravity_y` - vertical gravity (`0.0f` for space, `-9.8f` for platformers)
- `solver_iterations` - physics solver iteration count (10 is a good default)

```cpp
PULSE_ON_GAMESTART() {
    Serial.begin(115200);
    my_game.init(0.0f, 0.0f, 10);        // zero gravity (space shooter)
    my_game.init(0.0f, -9.8f, 10);       // platformer gravity
}
```

---

### Engine

#### tick

```cpp
my_game.tick();
```

Steps the physics simulation one frame (`world->step(PULSE)`). Call at the top of every `PULSE_ON_GAMESCENE`.

**Scope:** `PULSE_ON_GAMESCENE`

---

#### render

```cpp
my_game.render();
```

Flushes the renderer's sprite queue to the display. Call once at the end of every `PULSE_ON_GAMESCENE`, after all `draw` calls.

**Scope:** `PULSE_ON_GAMESCENE`

---

#### render_backgrounds

```cpp
my_game.render_backgrounds();
```

Advances each layer's scroll offset and blits all background layers to the framebuffer. **Must be called before any other draw calls** - sprites are rendered in FIFO order.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
PULSE_ON_GAMESCENE(Space_Level) {
    my_game.tick();
    my_game.render_backgrounds();  // always first

    my_game.draw("ship", "ship_sprite");
    my_game.render();
}
```

---

### Bodies

Bodies are the physical objects in your game. There are three types:

1. **Static** - Immovable obstacles (walls, floors, platforms)
2. **Controlled** - Player-driven objects; set velocity directly
3. **Dynamic** - Fully simulated with active physics

All three take a `Body_Descriptor`:

```cpp
{
    .position = { x, y },      // Vec2: initial position
    .velocity = { vx, vy },    // Vec2: initial velocity
    .force    = { fx, fy },    // Vec2: accumulated force
    .width    = { w, h },      // Vec2: half-extents
    .mass     = 1.0f,          // float: 0.0f = infinite mass (static/controlled)
    .friction = 0.2f,          // float: coefficient of friction
    .rotation = 0.0f           // float: rotation in radians
}
```

---

#### set_static_body

```cpp
my_game.set_static_body("name", Body_Descriptor{});
```

Allocates an immovable body and registers it with the physics world.

**Scope:** `PULSE_ON_GAMESCENE_START`

```cpp
my_game.set_static_body("floor", {
    .position = { 0.0f, -5.0f },
    .width    = { 10.0f, 0.5f }
});
my_game.set_static_body("left_wall", {
    .position = { -6.0f, 0.0f },
    .width    = { 0.5f, 8.0f }
});
```

---

#### set_controlled_body

```cpp
my_game.set_controlled_body("name", Body_Descriptor{});
```

Allocates a body for player or externally driven objects. `mass = 0` (infinite) so it ignores forces - move it by setting velocity directly.

**Scope:** `PULSE_ON_GAMESCENE_START`

```cpp
my_game.set_controlled_body("player", {
    .position = { 0.0f, 0.0f },
    .width    = { 0.5f, 0.75f }
});
```

---

#### set_dynamic_body

```cpp
my_game.set_dynamic_body("name", Body_Descriptor{});
```

Allocates a fully simulated body with active physics.

**Scope:** `PULSE_ON_GAMESCENE_START`

```cpp
my_game.set_dynamic_body("ball", {
    .position = { 0.0f, 5.0f },
    .velocity = { 2.0f, 0.0f },
    .width    = { 0.5f, 0.5f },
    .mass     = 1.0f
});
```

---

#### get_body

```cpp
pulse2d_body& body = my_game.get_body("name");
```

Returns a reference to a named body from the current scene.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
pulse2d_body& player = my_game.get_body("player");
player.apply_force({ 0.0f, 20.0f });

if (player.position.y < -10.0f) {
    // fell off the map
}
```

---

### Sprites, Rendering

---

#### set_sprite

```cpp
my_game.set_sprite("name", "path/to/file.bin", width, height);
```

Loads a raw RGB565 sprite from the SD card into the current scene's sprite pool.

**Scope:** `PULSE_ON_GAMESCENE_START`

```cpp
my_game.set_sprite("ship_sprite", "ship.bin", 48, 48);
my_game.set_sprite("enemy_sprite", "sprites/enemy.bin", 48, 48);
```

---

#### set_sprite_flash

```cpp
my_game.set_sprite_flash("name", data_array, width, height);
```

Registers a flash-resident sprite array as a named sprite. Use this for backgrounds and large assets stored in QSPI flash (generated by `png2header`).

**Scope:** `PULSE_ON_GAMESCENE_START`

```cpp
#include "../include/nebula-bg.h"

PULSE_ON_GAMESCENE_START(Level_One) {
    my_game.set_sprite_flash("bg_nebula", bg_1, 320, 240);
}
```

---

#### draw

```cpp
my_game.draw("body_name", "sprite_name");
my_game.draw("body_name", "sprite_name", rotation_radians);
```

Projects a body's world-space position to screen coordinates and queues the sprite for rendering. An optional third argument sets a fixed rotation in radians.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
my_game.draw("player", "player_sprite");
my_game.draw("ship", "ship_sprite", 1.5708f);  // 90° rotation
```

---

#### draw_body

```cpp
my_game.draw_body(body_ptr, "sprite_name");
my_game.draw_body(body_ptr, "sprite_name", rotation_radians);
```

Like `draw`, but takes a body pointer instead of a name. Use inside `render_pool` lambdas where you already have a pointer.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
my_game.render_pool("bullet_pool", [&](pulse2d_body* bullet) {
    my_game.draw_body(bullet, "bullet_sprite");
});
```

---

### Backgrounds

Background layers are drawn in the order they are added. Always call `my_game.render_backgrounds()` before any `draw` calls in the scene function.

---

#### add_parallax_layer

```cpp
my_game.add_parallax_layer("sprite_name", width, speed);
```

Adds a parallax scrolling layer. `width` is the image width in pixels used to wrap the scroll offset. `speed` is pixels per second.

**Scope:** `PULSE_ON_GAMESCENE_START`

```cpp
PULSE_ON_GAMESCENE_START(Space_Level) {
    my_game.set_sprite_flash("bg_nebula", nebula_data, 320, 240);
    my_game.set_sprite_flash("bg_stars",  stars_data,  320, 240);
    my_game.set_sprite_flash("bg_dust",   dust_data,   320, 240);

    my_game.add_parallax_layer("bg_nebula", 320.0f, 10.0f);   // slow
    my_game.add_parallax_layer("bg_stars",  320.0f, 3.0f);    // very slow
    my_game.add_parallax_layer("bg_dust",   320.0f, 65.0f);   // fast
}
```

---

#### add_background_layer

```cpp
my_game.add_background_layer("sprite_name", width);
```

Adds a static (non-scrolling) background layer.

**Scope:** `PULSE_ON_GAMESCENE_START`

```cpp
my_game.add_background_layer("menu_bg", 320.0f);
```

---

### Animations (Runtime)

Two animation systems serve different purposes. **Persistent animations** drive looped character states - idle, walk, jump - by mutating a sprite's frame pointer each tick. **VFX one-shots** fire and forget: play once and remove automatically, suited for explosions and impacts.

---

#### tick_animation (persistent)

```cpp
my_game.tick_animation(animator_inst, "sprite_name");
```

Advances the animator's frame accumulator and writes the current frame's pixel pointer into the named sprite. Call once per frame, after input and before `render`.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
// Declare at file scope (DSL)
PULSE_DEFINE_ANIMATOR(player_anim);
PULSE_ANIMATION_DEFINITION(anim_idle, idle_frames, 32, 48, 4, 8);
PULSE_ANIMATION_DEFINITION(anim_walk, walk_frames, 32, 48, 6, 12);

// In scene function
if (SEESAW_DIRECTION_IS_LEFT() || SEESAW_DIRECTION_IS_RIGHT()) {
    register_animation(player_anim, anim_walk);
} else {
    register_animation(player_anim, anim_idle);
}

my_game.tick_animation(player_anim, "player_sprite");
my_game.draw("player", "player_sprite");
my_game.render();
```

---

#### register_vfx

```cpp
my_game.register_vfx("anim_name", data_ptr, width, height, frames, fps);
```

Registers a VFX animation definition in the current scene's animation manager. Call once in `PULSE_ON_GAMESCENE_START`.

**Scope:** `PULSE_ON_GAMESCENE_START`

```cpp
#include "../include/explosion-anim.h"

PULSE_ON_GAMESCENE_START(Game_Level) {
    my_game.register_vfx("explosion", explosion_frames, 64, 64, 8, 12.0f);
}
```

---

#### play_vfx (spawn instance)

```cpp
my_game.play_vfx("anim_name", pos_x, pos_y);
```

Spawns a VFX animation at the given screen pixel coordinates. Plays once and is removed automatically. Silently dropped if the queue is full.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
auto coords = get_body_coordinates(&my_game.get_body("enemy"));
my_game.play_vfx("explosion", coords.x, coords.y);

// Fixed position
my_game.play_vfx("explosion", 160, 120);
```

---

#### play_vfx (advance all)

```cpp
my_game.play_vfx();
```

Advances and draws all active VFX animations. Call after drawing game objects, but before `render()`.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
my_game.draw("player", "player_sprite");
my_game.draw("enemy",  "enemy_sprite");
my_game.play_vfx();   // draw VFX on top
my_game.render();
```

---

### Kinematic Pools

Kinematic pools provide pre-allocated object pools for temporary entities like projectiles, particles, and powerups.

---

#### init_pool

```cpp
my_game.init_pool("pool_name", {
    .width = { 0.2f, 0.1f }
});
```

Initializes a named pool with a body descriptor template. Call once in `PULSE_ON_GAMESCENE_START`.

**Scope:** `PULSE_ON_GAMESCENE_START`

```cpp
PULSE_ON_GAMESCENE_START(Shooter) {
    my_game.init_pool("bullets", {
        .width    = { 0.2f, 0.1f },
        .friction = 0.0f
    });
}
```

---

#### spawn

```cpp
my_game.spawn("pool_name", delay_ms, pos_x, pos_y, vel_x, vel_y);
```

Spawns an object from the pool. Rate-limited by `delay_ms` - the call is ignored if fewer than `delay_ms` milliseconds have elapsed since the last spawn from this pool.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
if (SEESAW_BUTTON_INPUT(SEESAW_A)) {
    pulse2d_body& ship = my_game.get_body("ship");
    my_game.spawn("bullets",
        250,                                   // 250ms between shots
        ship.position.x + 0.6f, ship.position.y,
        8.0f, 0.0f);
}
```

---

#### despawn

```cpp
my_game.despawn("pool_name", body_ptr);
```

Releases a pooled body and returns its memory to the pool. Removes it from the physics world.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
my_game.render_pool("bullets", [&](pulse2d_body* bullet) {
    if (bullet->position.x > 8.0f) {
        my_game.despawn("bullets", bullet);
    }
});
```

---

#### render_pool

```cpp
my_game.render_pool("pool_name", [&](pulse2d_body* body) {
    // update and draw logic
});
```

Iterates over all active objects in a pool. Iterates in reverse to allow safe despawning during iteration.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
my_game.render_pool("bullets", [&](pulse2d_body* bullet) {
    if (bullet->position.x > 8.0f || bullet->position.x < -8.0f) {
        my_game.despawn("bullets", bullet);
        return;
    }
    my_game.draw_body(bullet, "bullet_sprite");
});
```

---

### Collision

Collision detection is driven by the physics world's arbiter map. All three methods work on the arbiter map populated after `my_game.tick()`.

---

#### objects_collided

```cpp
if (my_game.objects_collided()) { ... }
```

Returns `true` when at least one collision is active in the physics world.

**Scope:** `PULSE_ON_GAMESCENE`

---

#### on_collision_with

```cpp
my_game.on_collision_with("body_name", [&]() {
    // ...
});
```

Iterates all active arbiters and calls the action when either body in a pair matches the named body. Use for singleton bodies.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
my_game.on_collision_with("enemy", [&]() {
    if (!enemy_hit) {
        enemy_hit = true;
        health -= 10;
    }
});

my_game.on_collision_with("powerup", [&]() {
    score += 100;
    PULSE_SET_SCENE(my_game, Next_Level);
});
```

---

#### on_collision_with_body

```cpp
my_game.on_collision_with_body(body_ptr, [&](pulse2d_body* other) {
    // other is the body that collided with body_ptr
});
```

Like `on_collision_with`, but matches by pointer. Use inside `render_pool` lambdas where a pointer is already in scope. The action receives the *other* body in the collision pair.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
my_game.render_pool("bullets", [&](pulse2d_body* bullet) {
    my_game.on_collision_with_body(bullet, [&](pulse2d_body* other) {
        my_game.despawn("bullets", bullet);
        score += 10;
    });

    if (bullet->position.x > 6.0f) {
        my_game.despawn("bullets", bullet);
    } else {
        my_game.draw_body(bullet, "bullet_sprite");
    }
});
```

---

#### on_collision

```cpp
my_game.on_collision(body_ptr_a, body_ptr_b, [&]() {
    // fires when a and b are in the same arbiter
});
```

Fires the action when a single arbiter holds exactly both pointers (in either slot). Use inside `render_pool` to detect a specific bullet-vs-enemy collision.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
my_game.render_pool("lasers", [&](pulse2d_body* laser) {
    pulse2d_body& meteor = my_game.get_body("meteor");

    my_game.on_collision(laser, &meteor, [&] {
        my_game.despawn("lasers", laser);
        score += 10;
    });

    if (laser->position.x > 6.67f) {
        my_game.despawn("lasers", laser);
    } else {
        my_game.draw_body(laser, "laser_sprite");
    }
});
```

---

### Gamepad Controls (Runtime)

Runtime gamepad methods apply movement profiles to a named body using the current gamepad state. Three profiles cover most 2D game genres.

---

#### set_arcade_directional_control

```cpp
my_game.set_arcade_directional_control("body_name", max_speed);
my_game.set_arcade_directional_control("body_name", max_speed, vertical_only, horizontal_only);
```

**Profile A: Arcade Controller** (Pokémon, Zelda, Pac-Man)

Sets velocity directly from stick position for instant response and instant stop. Optional boolean arguments restrict movement to one axis.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
my_game.set_arcade_directional_control("player", 3.0f);                     // both axes
my_game.set_arcade_directional_control("player", 3.0f, true,  false);       // vertical only
my_game.set_arcade_directional_control("player", 3.0f, false, true);        // horizontal only
```

---

#### set_arcade_directional_inverted_control

```cpp
my_game.set_arcade_directional_inverted_control("body_name", max_speed);
my_game.set_arcade_directional_inverted_control("body_name", max_speed, vertical_only, horizontal_only);
```

Same as `set_arcade_directional_control`, but with inverted axes.

```cpp
my_game.set_arcade_directional_inverted_control("ship", 4.0f);
```

---

#### set_dynamic_directional_control

```cpp
my_game.set_dynamic_directional_control("body_name", acceleration);
```

**Profile B: Momentum Controller** (Asteroids, Mario)

Applies a thrust force each frame scaled by `acceleration`. Velocity builds up over time - pair with `set_sliding_friction_directional_control` to prevent infinite sliding.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
my_game.set_dynamic_directional_control("ship", 0.8f);
my_game.set_sliding_friction_directional_control("ship", 0.92f);
```

---

#### set_sliding_friction_directional_control

```cpp
my_game.set_sliding_friction_directional_control("body_name", drag_amount);
```

**Profile C: Top-Down Friction**

Applies linear drag to the body each frame. `drag_amount` is a multiplier (< 1.0 for braking effect, e.g. 0.9).

**Scope:** `PULSE_ON_GAMESCENE`

---

## Complete Example

A complete Space Shooter demonstrating most features:

```cpp
#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS
#include "../include/explosion-anim.h"
#include "../include/stars-bg.h"

// Scene declaration
PULSE_DEFINE_SCENE(Space_Shooter, 20, 6);

// Runtime instance - owns engine, world, and current_scene
PULSE2D_START_PULSE();
PULSE_INIT_GAME(my_game, Space_Shooter);

// Persistent state
PULSE_DEFINE int score = 0;
PULSE_DEFINE bool enemy_hit = false;
PULSE_DEFINE uint32_t cooldown = 0;

PULSE_ON_GAMESCENE_START(Space_Shooter) {
    // Flash sprites
    my_game.set_sprite_flash("bg_stars", stars_bg, 320, 240);

    // SD card sprites
    my_game.set_sprite("ship_sprite",   "ship.bin",   48, 48);
    my_game.set_sprite("enemy_sprite",  "enemy.bin",  48, 48);
    my_game.set_sprite("bullet_sprite", "bullet.bin", 12,  8);

    // Scrolling background
    my_game.add_parallax_layer("bg_stars", 320.0f, 15.0f);

    // VFX animation definition
    my_game.register_vfx("explosion", explosion_frames, 64, 64, 8, 12.0f);

    // Bullet pool
    my_game.init_pool("bullets", {
        .width    = { 0.15f, 0.08f },
        .friction = 0.0f
    });

    // Player ship (controlled)
    my_game.set_controlled_body("ship_object", {
        .position = { -4.0f, 0.0f },
        .width    = { 0.5f, 0.5f }
    });

    // Enemy (dynamic)
    my_game.set_dynamic_body("enemy_object", {
        .position = { 3.0f, 0.0f },
        .mass     = 1.0f,
        .width    = { 0.6f, 0.6f }
    });
}

PULSE_ON_GAMESCENE(Space_Shooter) {
    my_game.tick();
    PULSE_POLL_SEESAW_GAMEPAD();

    my_game.render_backgrounds();

    my_game.set_arcade_directional_control("ship_object", 3.5f);

    pulse2d_body& ship = my_game.get_body("ship_object");

    // Fire bullets
    if (cooldown > 0) cooldown--;
    if (SEESAW_BUTTON_INPUT(SEESAW_A) && cooldown == 0) {
        my_game.spawn("bullets",
            100,
            ship.position.x + 0.6f, ship.position.y,
            8.0f, 0.0f);
        cooldown = 10;
    }

    // Update live bullets
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

    // Reset
    if (SEESAW_BUTTON_INPUT(SEESAW_START)) {
        PULSE_SET_SCENE(my_game, Space_Shooter);
    }

    // Draw objects
    my_game.draw("ship_object", "ship_sprite");
    if (!enemy_hit) {
        my_game.draw("enemy_object", "enemy_sprite");
    }

    my_game.play_vfx();   // advance VFX
    my_game.render();
}

PULSE_ON_GAMESTART() {
    Serial.begin(115200);
    pulse_register_etl_error_handler();
    my_game.init(0.0f, 0.0f, 10);
    start_seesaw_gamepad();
    PULSE_SET_SCENE(my_game, Space_Shooter);
}

PULSE_ON_GAMELOOP() {
    PULSE2D_TICK_GAMESCENE();
}
```

---

## See Also

- [Physics README](pulse2d/graphics/readme.md) - Physics engine, bodies, sensors
- [Blog series](https://soliloq.uy/tag/pulse2d/) - Embedded development blog
