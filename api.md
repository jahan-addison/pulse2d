# API Reference

Game development in Pulse2D is split into two layers: The **Internal DSL** provides the structural skeleton, i.e. lifecycle hooks, scene declarations, type aliases, and animation blueprints. The **Core API** provides `Runtime<Scenes...>`, which owns the engine, physics world, and active scene and exposes all game actions as methods.

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
  - [State Machines](#state-machines)
  - [Game State, Setup](#game-state-setup)
  - [Scenes](#scenes)
    - [PULSE_SCENE_FN](#pulse_scene_fn)
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
  - [Text](#text)
  - [Animations (Runtime)](#animations-runtime)
  - [Kinematic Pools](#kinematic-pools)
  - [Collision](#collision)
  - [Gamepad Controls (Runtime)](#gamepad-controls-runtime)
- [Levels](#levels)
  - [State](#state-1)
  - [Functions](#functions)
  - [draw\_fn pass-through](#draw_fn-pass-through)
  - [on\_reset callback](#on_reset-callback)
  - [Wiring it up](#wiring-it-up)
- [Complete Example](#complete-example)
- [See Also](#see-also)

---

## Internal DSL

The Internal DSL is in `pulse2d/dsl.h` and provides the structural skeleton of a game: lifecycle entry points, scene declarations, type aliases, animation blueprints, and debug helpers. These are macros and free functions that do not require the engine or physics world.

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
using p_ui32 = uint32_t;
using p_ui16 = uint16_t;
using p_ui8  = uint8_t;
using p_i32  = int32_t;
using p_i16  = int16_t;
using p_i8   = int8_t;
```

---

### Math

`pulse2d_math` is an alias for `pulse2d::graphics::math`, which provides `Vec2`, `Mat22`, and random number helpers.

`pulse2d_math::random(lo, hi)` returns a float in `[lo, hi]`. On Teensy both use `rand()`; on host both use `arc4random`:

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

For gameplay randomness on Teensy, **prefer `trng_random`** - it seeds a PCG32 engine from the i.MX RT1062 hardware TRNG via the Entropy library, which produces better statistical quality than `rand()`. Call `init_trng_engine_random()` once in `PULSE_ON_GAMESTART`, then use `trng_random(lo, hi)` anywhere in the game loop:

```cpp
PULSE_ON_GAMESTART()
{
    Serial.begin(115200);
    pulse2d_math::init_trng_engine_random();
    // ...
}

PULSE_ON_GAMESCENE(Level_One)
{
    // place an asteroid at a random y position
    body.set_position({ 6.0f, pulse2d_math::trng_random(-2.5f, 2.5f) });
}
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

`pulse2d_util` is an alias for `pulse2d::util`. It exposes the `overload` pattern for `std::visit`-style variant dispatch:

```cpp
std::visit(pulse2d_util::overload{
    [](asteriods::L_State& s)  { /* handle large */  },
    [](asteriods::XL_State& s) { /* handle extra large */ },
}, obj.state);
```

---

### State Machines

`sml` is an alias for `boost::sml`, available in all game code via `#include PULSE2D_HEADER`.

[boost/sml](https://github.com/boost-ext/sml) is a header-only, zero-allocation state machine library. It compiles the transition table into a jump table at compile time - no heap, no virtual dispatch, no RTTI. Those properties make it a natural fit for the Teensy 4.1.

Define events, states, and a transition table in a plain struct, then wrap it in `sml::sm<>`:

```cpp
// Events
struct Spawn  {};
struct Impact {};
struct Crash  {};

// States (tag types)
struct Spawned {};
struct Live    {};
struct Erased  {};
struct Crashed {};

// Transition table
struct asteroid_sm
{
    auto operator()() const
    {
        using namespace sml;
        return make_transition_table(
            *state<Spawned> + event<Spawn>  = state<Live>,
             state<Live>    + event<Impact> = state<Erased>,
             state<Live>    + event<Crash>  = state<Crashed>,
             state<Spawned> + event<Crash>  = state<Crashed>
        );
    }
};
```

Instantiate with `sml::sm<asteroid_sm>` and dispatch events via `process_event`. Query state with `is`:

```cpp
PULSE_DEFINE sml::sm<asteroid_sm> asteroid_state{};

// in PULSE_ON_GAMESCENE_START
asteroid_state.process_event(Spawn{});

// in PULSE_ON_GAMESCENE
if (asteroid_state.is(sml::state<Live>)) {
    asterisk.draw("meteor_object_1", "meteor_1m_sprite");
}

// on laser collision
asteroid_state.process_event(Impact{});
```

`sml::sm<>` holds only the current state index - no dynamic allocation. For multiple independent objects use an array of machines:

```cpp
PULSE_DEFINE etl::array<sml::sm<asteroid_sm>, 4> asteroids{};
```

Guards and actions can be lambdas captured by value - keep them stateless or capture a pointer to external state:

```cpp
struct asteroid_sm
{
    bool& player_hit;

    auto operator()() const
    {
        using namespace sml;
        auto on_crash = [this] { player_hit = true; };

        return make_transition_table(
            *state<Spawned> + event<Spawn>  = state<Live>,
             state<Live>    + event<Crash>  / on_crash = state<Crashed>
        );
    }
};
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

Declares the two function pointers that control scene dispatch: `pending_transition` and `active_scene_fn`. In addition, it also exposes the runtime namespace. Place this at file scope, once per game.

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

#### PULSE_ON_GAMESTART, PULSE_ON_GAMELOOP

```cpp
PULSE_ON_GAMESTART() {
    // initialization - maps to Arduino setup()
}

PULSE_ON_GAMELOOP() {
    // per-frame - maps to Arduino loop()
}
```

Map to Arduino `setup()` and `loop()`. The game loop body is almost always just `PULSE_TICK_GAMESCENE()`.

**Scope:** `global`

```cpp
PULSE_ON_GAMESTART() {
    Serial.begin(115200);
    pulse_register_etl_error_handler();
    my_game.init(0.0f, 0.0f, 10);
    PULSE_ENABLE_SEESAW_GAMEPAD();
    PULSE_SET_SCENE(my_game, Main_Menu);
}

PULSE_ON_GAMELOOP() {
    PULSE_TICK_GAMESCENE();
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

#### PULSE_SCENE_FN

```cpp
PULSE_SCENE_FN void function_name(pulse2d_scene_runtime<Scenes...>& game, ...);
```

Declares a template function constrained to valid scene types (`pulse2d::Scene`). The type parameter pack is always named `Scenes` and is deduced from the `Runtime<Scenes...>&` argument at the call site - no explicit template argument needed.

Use this for level or scene utility functions that should work with any scene without being hardcoded to a specific one. The constraint catches misuse at the call site if a non-scene type is passed.

**Scope:** `global`

```cpp
// scenes/levels/level_one.h
namespace scenes::levels::level_one {

PULSE_SCENE_FN void setup_walls(pulse2d_scene_runtime<Scenes...>& game)
{
    game.set_static_body("top_wall",    { .position = { 0.0f,  4.5f }, .width = { 20.0f, 0.5f } })
        .set_static_body("bottom_wall", { .position = { 0.0f, -4.5f }, .width = { 20.0f, 0.5f } });
}

PULSE_SCENE_FN void setup_background(pulse2d_scene_runtime<Scenes...>& game)
{
    game.set_background_sprite("bg_stars", stars_data, 320, 240)
        .add_parallax_layer("bg_stars", 320.0f, 20.0f);
}

} // namespace scenes::levels::level_one
```

```cpp
// src/game.cc
namespace level_one = scenes::levels::level_one;

PULSE_ON_GAMESCENE_START(Level_One) {
    level_one::setup_walls(my_game);
    level_one::setup_background(my_game);
}
```

`PULSE_SCENE_FN` fixes the template parameter name as `Scenes`. Functions that reference it in their signature or body must use that exact name.

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

**Scope:** `PULSE_ON_GAMESTART`

Safe to call from `PULSE_ON_GAMESTART` before the game loop starts. Do not call directly from inside `PULSE_ON_GAMESCENE`. The transition takes effect immediately, destroying the current scene while the tick function is still on the call stack. Any code that runs after the call (animation ticks, `render()`) will execute against the new scene's empty pools. Use `PULSE_DEFER_SCENE` instead.

```cpp
PULSE_ON_GAMESTART() {
    my_game.init(0.0f, 0.0f, 10);
    PULSE_SET_SCENE(my_game, Main_Menu);
}
```

---

#### PULSE_DEFER_SCENE

```cpp
PULSE_DEFER_SCENE(game, scene_name);
```

Defers a scene transition to the end of the current frame tick. The current scene function runs to completion (including `render()`) before the transition executes. This is the correct way to transition from inside a scene tick or a callback passed to a scene function.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
// Direct use in a scene function:
PULSE_ON_GAMESCENE(Level_One) {
    // ...
    if (player_dead) {
        PULSE_DEFER_SCENE(my_game, Game_Over);
    }
}

// Inside a callback passed to a PULSE_SCENE_FN:
PULSE_ON_GAMESCENE(Level_One) {
    level_one::on_tick(my_game, ship, [] {
        PULSE_DEFER_SCENE(my_game, Game_Over);
    });
    my_game.tick_vfx();
    my_game.render(); // still runs against Level_One; transition fires after
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
// scenes/levels/game_level.h
namespace scenes::levels::game_level {

PULSE_SCENE_FN void on_start(pulse2d_scene_runtime<Scenes...>& game)
{
    game
        .set_sprite("player_sprite", "player.bin")
        .set_controlled_body("player", {
            .position = { 0.0f, 0.0f },
            .width    = { 0.5f, 0.5f }
        })
        .set_static_body("floor", {
            .position = { 0.0f, -5.0f },
            .width    = { 10.0f, 0.5f }
        });
}

} // namespace scenes::levels::game_level
```

```cpp
// src/game.cc
namespace game_level = scenes::levels::game_level;

PULSE_ON_GAMESCENE_START(Game_Level) {
    game_level::on_start(my_game);
}
```

---

#### PULSE_ON_GAMESCENE

```cpp
PULSE_ON_GAMESCENE(scene_name) {
    // per-frame logic
}
```

Defines the per-frame function for a scene. Registered as the active tick function by `PULSE_SET_SCENE` and called every frame by `PULSE_TICK_GAMESCENE`.

**Scope:** `global`

```cpp
// scenes/levels/game_level.h
namespace scenes::levels::game_level {

PULSE_SCENE_FN void on_tick(pulse2d_scene_runtime<Scenes...>& game)
{
    PULSE_POLL_SEESAW_GAMEPAD();

    game.set_arcade_directional_control("player", 3.0f);
    game.draw("player", "player_sprite");
}

} // namespace scenes::levels::game_level
```

```cpp
// src/game.cc
namespace game_level = scenes::levels::game_level;

PULSE_ON_GAMESCENE(Game_Level) {
    my_game.tick();
    game_level::on_tick(my_game);
    my_game.render();
}
```

---

#### PULSE_TICK_GAMESCENE

```cpp
PULSE_TICK_GAMESCENE();
```

Calls the active scene's tick function, then resolves any pending transition. This is the only call needed in `PULSE_ON_GAMELOOP`.

**Scope:** `PULSE_ON_GAMELOOP`

```cpp
PULSE_ON_GAMELOOP() {
    PULSE_TICK_GAMESCENE();
}
```

---

### Gamepad Input (DSL)

The gamepad system supports the [Adafruit Seesaw Gamepad QT](https://www.adafruit.com/product/5743) with analog thumbstick and 6 buttons over I2C.

---

#### PULSE_ENABLE_SEESAW_GAMEPAD

```cpp
PULSE_ENABLE_SEESAW_GAMEPAD();
```

Initializes the I2C bus and gamepad hardware. Call once in `PULSE_ON_GAMESTART`.

**Scope:** `PULSE_ON_GAMESTART`

```cpp
PULSE_ON_GAMESTART() {
    Serial.begin(115200);
    my_game.init(0.0f, 0.0f, 10);
    PULSE_ENABLE_SEESAW_GAMEPAD();
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


Note that you can use the [animation2header](/tools/animation2header) tool to build an animation sheet.

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
#include "../include/player-idle-anim.h"  // idle_frames[],
#include "../include/player-walk-anim.h"  // walk_frames[],

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

#### px_to_units

```cpp
auto size = px_to_units(px_w, px_h);
auto size = px_to_units(px_w, px_h, px_per_unit);
```

Converts pixel dimensions to physics world units. Returns a `pulse2d::graphics::math::Vec2`. The third argument defaults to `pulse2d::config::pixels_per_unit` (30.0).

Use it when setting body dimensions from pixel-measured sprite sizes:

```cpp
asterisk.set_dynamic_body("meteor",
    {
        .position = { 6.0f, 1.5f },
        .velocity = { -7.5f, 0.0f },
        .width    = px_to_units(65.0f, 65.0f),
        .mass     = 1.0f,
        .is_sensor = true
    });
```

**Scope:** `PULSE_ON_GAMESCENE_START`, `PULSE_ON_GAMESCENE`

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

#### PULSE_POLL_SERIAL_CONNECTION

```cpp
PULSE_POLL_SERIAL_CONNECTION();
```

Blocks until the USB serial connection is established, then prints a confirmation line. Use this in `PULSE_ON_GAMESTART()` when you need serial output to be visible from the very first line - without it, output emitted before the host opens the port is silently dropped.

**Scope:** `PULSE_ON_GAMESTART`

```cpp
PULSE_ON_GAMESTART() {
    Serial.begin(115200);
    PULSE_POLL_SERIAL_CONNECTION();
    pulse_register_etl_error_handler();
    my_game.init(0.0f, 0.0f, 10);
    PULSE_ENABLE_SEESAW_GAMEPAD();
    PULSE_SET_SCENE(my_game, Level_One);
}
```

Output on connect:

```
[DEBUG] setup: serial OK
```

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

#### show_debug_rect

```cpp
my_game.show_debug_rect();
```

Enables axis-aligned bounding box outlines for every body in the scene. Background rendering must be turned off to see them - with backgrounds active the rects are drawn underneath and invisible.

Call once in `PULSE_ON_GAMESTART` after `init()`. The flag persists for the lifetime of the session.

**Scope:** `PULSE_ON_GAMESTART`

```cpp
PULSE_ON_GAMESTART() {
    Serial.begin(115200);
    my_game.init(0.0f, 0.0f, 10);
    my_game.show_debug_rect();       // enable body outlines
    PULSE_ENABLE_SEESAW_GAMEPAD();
    PULSE_SET_SCENE(my_game, Level_One);
}

// In the scene, comment out render_backgrounds so the rects are visible:
PULSE_ON_GAMESCENE(Level_One) {
    my_game.tick();
    PULSE_POLL_SEESAW_GAMEPAD();
    // my_game.render_backgrounds();
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
    .width    = { w, h },      // Vec2: full width and height
    .mass     = 1.0f,          // float: omit or 0.0f on controlled bodies defaults to 1.0f
    .friction = 0.2f,          // float: coefficient of friction
    .rotation = 0.0f           // float: rotation in radians
}
```

Static bodies are infinite mass by design (no `set_motion` applied). Controlled bodies call `set_motion()` automatically and default `mass` to `1.0f` if unspecified or zero - this allows them to collide correctly against static walls.

---

#### set_static_body

```cpp
my_game.set_static_body("name", Body_Descriptor{});
```

Allocates an immovable body and registers it with the physics world.

**Scope:** `PULSE_ON_GAMESCENE_START`

```cpp
my_game
    .set_static_body("floor", {
        .position = { 0.0f, -5.0f },
        .width    = { 10.0f, 0.5f }
    })
    .set_static_body("left_wall", {
        .position = { -6.0f, 0.0f },
        .width    = { 0.5f, 8.0f }
    });
```

---

#### set_controlled_body

```cpp
my_game.set_controlled_body("name", Body_Descriptor{});
```

Allocates a body for player or externally driven objects. `set_motion()` is called automatically so the body has a finite mass and responds correctly to static wall collisions. If `mass` is unspecified or zero it defaults to `1.0f`. Move it by setting velocity directly via the gamepad control profiles.

**Scope:** `PULSE_ON_GAMESCENE_START`

```cpp
my_game.set_controlled_body("ship", {
    .position = { -4.0f, 0.0f },
    .width    = { 1.0f, 1.0f }
    // mass defaults to 1.0f
});

// explicit mass:
my_game.set_controlled_body("ship", {
    .position = { -4.0f, 0.0f },
    .width    = { 1.0f, 1.0f },
    .mass     = 2.0f
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

#### Body field setters

Every field on `pulse2d_body` has a chainable setter generated by `BODY_FIELD_BUILDER`. Each setter assigns the field and returns `*this`. Use them for in-place mutation during gameplay without a full descriptor reinit:

```cpp
body.set_position(Vec2)
body.set_velocity(Vec2)
body.set_width(Vec2)
body.set_rotation(float)
body.set_angular_velocity(float)
body.set_mass(float)
body.set_friction(float)
body.set_is_sensor(bool)
```

Typical use - teleport a body into position before it becomes the active target, or shrink its collision box on hit:

```cpp
auto& as = asterisk.get_body("meteor_object_2");

// move off-screen body to a new spawn point
as.set_position({ 6.0f, pulse2d_math::trng_random(-2.5f, 2.5f) });

// shrink collision box to match the smaller sprite
as.set_width(px_to_units(25.0f, 22.0f));

// chain both
as.set_position({ 6.0f, 0.0f }).set_width({ 0.0f, 0.0f });
```

**Scope:** `PULSE_ON_GAMESCENE`

---

### Sprites, Rendering

There are two sprite types:

- `set_sprite` - loaded from the SD card at scene start. Limited by `MAX_LOADED_SPRITES` (default 12).
- `set_sprite_embedded` / `set_background_sprite` - compiled into the binary. Limited by `MAX_EMBEDDED_SPRITES` (default 64).

Both types share the same name lookup - `draw`, `draw_body`, `tick_animation`, and background layers all work the same regardless of source.

---

#### set_sprite

```cpp
my_game.set_sprite("name", "path/to/file.bin");
my_game.set_sprite("name", "path/to/file.bin", width, height);
```

Loads a raw RGB565 sprite from the SD card into the current scene's sprite pool. The `.bin` format encodes dimensions in the first four bytes, so the no-dimension form is the default. Pass explicit dimensions to validate that the file header matches - on host, they scale the image instead.

**Scope:** `PULSE_ON_GAMESCENE_START`

```cpp
my_game
    .set_sprite("ship_sprite",  "ship.bin")
    .set_sprite("enemy_sprite", "sprites/enemy.bin");
```

---

#### set_sprite_embedded

```cpp
my_game.set_sprite_embedded("name", data_array, width, height);
```

Registers a compiled sprite array as a named sprite in the embedded pool. Use for any asset compiled into the binary - animation sheets, large art, or any additional sprites from `png2header` output.

**Scope:** `PULSE_ON_GAMESCENE_START`

```cpp
#include "../include/asteroid-sheet.h"

PULSE_ON_GAMESCENE_START(Level_One) {
    my_game.set_sprite_embedded("asteroid_large", asteroid_l, 65, 58);
}
```

---

#### set_background_sprite

```cpp
my_game.set_background_sprite("name", data_array, width, height);
```

Alias for `set_sprite_embedded`, conventional for parallax and static background layers.

**Scope:** `PULSE_ON_GAMESCENE_START`

```cpp
#include "../include/nebula-bg.h"

PULSE_ON_GAMESCENE_START(Level_One) {
    my_game.set_background_sprite("bg_nebula", bg_1, 320, 240);
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

#### draw_sprite

```cpp
my_game.draw_sprite("sprite_name", x, y);
```

Writes a named sprite directly to the screen at pixel coordinates, bypassing body lookup and world-space projection. Use for HUD elements, score displays, and fixed-position overlays that have no associated physics body.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
my_game.draw_sprite("health_bar", 4, 220);
my_game.draw_sprite("crosshair",  static_cast<int16_t>(cursor_x), static_cast<int16_t>(cursor_y));
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
    my_game
        .set_background_sprite("bg_nebula", nebula_data, 320, 240)
        .set_background_sprite("bg_stars",  stars_data,  320, 240)
        .set_background_sprite("bg_dust",   dust_data,   320, 240)
        .add_parallax_layer("bg_nebula", 320.0f, 10.0f)   // slow
        .add_parallax_layer("bg_stars",  320.0f,  3.0f)   // very slow
        .add_parallax_layer("bg_dust",   320.0f, 65.0f);  // fast
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

### Text

Text is queued in the same pass as sprites and flushed during `render()`, compositing on top of all sprites. Call `draw_text` anywhere in `PULSE_ON_GAMESCENE` before `render()`. Works on both Teensy and the SDL2 host path.

For custom fonts, supply a `Renderer::Font_Def` pointing to a column-major 1bpp glyph table produced by `font2bytes` or a compatible tool. The engine ships no custom font data.

---

#### draw_text

```cpp
my_game.draw_text(text, x, y, color);
my_game.draw_text(text, x, y, color, size);
my_game.draw_text(text, x, y, color, font_def);
my_game.draw_text(text, x, y, color, font_def, size);
```

Queues `text` to draw at pixel coordinates `(x, y)`. Flushed during `render()` after all sprites. `size` is a float scale multiplier (1.0 = 5x7 px, 1.5 = 7x10 px, 2.0 = 10x14 px). Fractional values are rendered with nearest-neighbor sampling. `'\n'` advances to the next line. The custom overload takes a `Renderer::Font_Def` (see below).

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
PULSE_ON_GAMESCENE(Level_One) {
    my_game.tick();
    my_game.render_backgrounds();
    // ... game logic ...

    static uint16_t white  = pulse2d::Renderer::text_color(255, 255, 255);
    static uint16_t yellow = pulse2d::Renderer::text_color(255, 220,   0);
    my_game.draw_text("SCORE: 0", 4, 4,    white);
    my_game.draw_text("LIVES: 3", 4, 14,   white);
    my_game.draw_text("PAUSED",   112, 110, yellow, 2);

    my_game.render();
}
```

---

#### draw_text with a custom font

A custom font is a column-major 1bpp bitmap table produced by `font2bytes` or any tool that emits the same layout (one byte per column, bit 0 = top row). Keep the table in flash with `PULSE2D_FLASHMEM`.

```cpp
// myfont.h - generated by font2bytes, first char 0x20, cell 6x8
#pragma once
#include <cstdint>
#include <pulse2d/util.h>

PULSE2D_FLASHMEM static const uint8_t my_font_data[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ' ' 0x20
    // ... remaining glyphs ...
};

inline constexpr pulse2d::Renderer::Font_Def my_font {
    .data       = my_font_data,
    .cell_w     = 6,
    .cell_h     = 8,
    .first_char = 0x20,
};
```

```cpp
#include <myfont.h>

PULSE_ON_GAMESCENE(HUD) {
    // ...
    my_game.draw_text("SCORE", 4, 4, white, my_font);
    my_game.draw_text("LIVES", 4, 14, white, my_font, 2);
}
```

---

#### text_color

```cpp
uint16_t color = pulse2d::Renderer::text_color(r, g, b);
```

Packs 8-bit RGB components into an RGB565 value. Compute once at scene scope and reuse; not per frame.

```cpp
static uint16_t white  = pulse2d::Renderer::text_color(255, 255, 255);
static uint16_t yellow = pulse2d::Renderer::text_color(255, 220,   0);
```

---

### Animations (Runtime)

There are two different animation systems available:

* **Persistent animations** control looped character states - idle, walk, jump - by mutating a sprite's frame pointer each tick.
* **VFX one-shots** fire and forget: play once and remove automatically, suited for explosions and impacts.

---

#### tick_animation (persistent)

```cpp
my_game.tick_animation(animator_inst, "sprite_name");
```

Advances the animator's frame accumulator and writes the current frame's pixel pointer into the named sprite. Call once per frame, after input and before `render`.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
// Declare at file scope
PULSE_DEFINE_ANIMATOR(player_anim);
PULSE_ANIMATION_DEFINITION(anim_idle, idle_frames, 32, 48, 4, 8);
PULSE_ANIMATION_DEFINITION(anim_walk, walk_frames, 32, 48, 6, 12);

// In scene function:
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

#### tick_vfx

```cpp
my_game.tick_vfx();
```

Advances and draws all active VFX animations. Call after drawing game objects, but before `render()`.

**Scope:** `PULSE_ON_GAMESCENE`

```cpp
my_game.draw("player", "player_sprite");
my_game.draw("enemy",  "enemy_sprite");
my_game.tick_vfx();   // draw VFX on top
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
    PULSE_DEFER_SCENE(my_game, Next_Level);
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

Static bodies (walls, floors, platforms) registered with `set_static_body` interact correctly with all three profiles - the controlled or dynamic body will collide and stop against them without tunnelling or passing through:

```cpp
PULSE_ON_GAMESCENE_START(Level_One) {
    my_game
        .set_controlled_body("ship", {
            .position = { -4.0f, 0.0f },
            .width    = { 1.0f,  1.0f },
            .mass     = 1.0f
        })
        .set_static_body("top_wall", {
            .position = { 0.0f,  4.5f },
            .width    = { 20.0f, 0.5f }
        })
        .set_static_body("bottom_wall", {
            .position = { 0.0f, -4.5f },
            .width    = { 20.0f, 0.5f }
        });
}

PULSE_ON_GAMESCENE(Level_One) {
    my_game.tick();
    PULSE_POLL_SEESAW_GAMEPAD();
    my_game.set_arcade_directional_control("ship", 5.0f);
    my_game.draw("ship", "ship_sprite");
    my_game.render();
}
```

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

## Levels

Level-specific code lives in a dedicated namespace in its own header. The namespace owns a `State` struct, all mutable level variables, and `PULSE_SCENE_FN` functions for startup and the per-frame tick. The main game translation unit includes the header and wires the functions into `PULSE_ON_GAMESCENE_START` / `PULSE_ON_GAMESCENE`.

This pattern keeps level code self-contained, avoids translation unit-global sprawl, and lets the compiler enforce that level functions are only called with a valid scene type.

---

### State

All mutable level variables go in a `State` struct. Declare it in the level namespace, then use `PULSE_DEFINE_SCENE_STATE` to create a single static instance named `state`. Access it from the game translation unit as `level_one::state`.

```cpp
// scenes/levels/level_one.h
namespace scenes::levels::level_one {

struct State {
    int   current_enemy  = 0;
    float speed_ratio    = 1.0f;
    bool  level_complete = false;
};

PULSE_DEFINE_SCENE_STATE(State);
```

`PULSE_DEFINE_SCENE_STATE(State)` expands to `static State state{}`, zero-initializing every member with static storage duration - the same guarantees as any other `PULSE_DEFINE` variable.

---

### Functions

Level functions use `PULSE_SCENE_FN` and accept a `pulse2d_scene_runtime<Scenes...>&` as their first parameter. They live in the same namespace as the state and call it directly. `state` is namespace-scoped static storage:

```cpp
// Startup: called once from PULSE_ON_GAMESCENE_START
PULSE_SCENE_FN void on_level_start(pulse2d_scene_runtime<Scenes...>& game)
{
    // set up backgrounds, walls, sprites, bodies...
    game.set_sprite("enemy_sprite", "enemy.bin")
        .set_dynamic_body("enemy_1",
            { .position = { 20.0f, 0.0f }, .velocity = { 0.0f, 0.0f },
              .width    = px_to_units(32.0f, 32.0f), .mass = 1.0f });

    state.speed_ratio    = 1.0f;
    state.current_enemy  = 0;
    state.level_complete = false;
}

// Tick: called every frame from PULSE_ON_GAMESCENE
PULSE_SCENE_FN void on_level_tick(pulse2d_scene_runtime<Scenes...>& game,
    pulse2d_body& player,
    void (*on_reset)())
{
    PULSE_POLL_SEESAW_GAMEPAD();

    if (SEESAW_BUTTON_INPUT(SEESAW_START)) {
        on_reset();
    }
    // per-frame logic using state.current_enemy, state.speed_ratio, etc.
}
```

`PULSE_SCENE_FN` fixes the template parameter pack as `Scenes`, so the `pulse2d_scene_runtime<Scenes...>&` in the signature refers to the same deduced type with no explicit template argument needed at the call site.

---

### draw\_fn pass-through

`pulse2d::state::Draw_Fn` is a non-capturing function pointer (`void (*)(pulse2d_body*, const char*)`). Lambdas stored in this type cannot close over local variables, including the `Runtime<Scenes...>&` parameter.

The solution: accept the draw function as a parameter to `on_level_start`, store it in `state.draw` (which has static storage duration), then copy `state.draw` directly into any config struct that needs it. Lambdas that later *read* from `state.draw` (or from `state` in general) remain non-capturing because they reference namespace-scoped static storage - not the parameter:

```cpp
struct State {
    // ...
    pulse2d::state::Draw_Fn draw = nullptr;
};
PULSE_DEFINE_SCENE_STATE(State);

PULSE_SCENE_FN void on_level_start(pulse2d_scene_runtime<Scenes...>& game,
    pulse2d::state::Draw_Fn draw_fn)
{
    state.draw = draw_fn;   // store in static; lambdas below stay non-capturing

    my_manager.add({
        .draw = state.draw, // copied from static - not capturing game or draw_fn
    });
}
```

In the main translation unit, the draw lambda references the concrete global runtime - it is non-capturing there:

```cpp
PULSE_ON_GAMESCENE_START(Level_One) {
    level_one::on_level_start(my_game,
        [](pulse2d_body* b, const char* s) { my_game.draw_body(b, s); });
}
```

---

### on\_reset callback

`PULSE_DEFER_SCENE` (and `PULSE_SET_SCENE` underneath it) uses token-pasting to construct function names (`pulse2d_scene_enter_Level_One`). Inside a template function the second argument must be a literal token, not a template parameter - `PULSE_DEFER_SCENE(game, Scenes)` would paste `Scenes` literally, not the actual scene name, and fail to resolve.

Pass an `on_reset` callback instead. The caller in the concrete scene function provides the lambda, where `PULSE_DEFER_SCENE` has the concrete name:

```cpp
// level header - on_reset abstracts the scene transition
PULSE_SCENE_FN void on_level_tick(pulse2d_scene_runtime<Scenes...>& game,
    pulse2d_body& player,
    void (*on_reset)())
{
    PULSE_POLL_SEESAW_GAMEPAD();
    if (SEESAW_BUTTON_INPUT(SEESAW_START)) {
        state.current_enemy  = 0;
        state.level_complete = false;
        on_reset();
        return;
    }
}
```

```cpp
// game translation unit - PULSE_DEFER_SCENE used with the concrete name
PULSE_ON_GAMESCENE(Level_One) {
    level_one::on_level_tick(my_game, ship, [] {
        PULSE_DEFER_SCENE(my_game, Level_One);
    });
}
```

Similarly, `PULSE_POLL_SEESAW_GAMEPAD()` declares a local `gamepad_state` variable - `SEESAW_BUTTON_INPUT` expands to `gamepad_state.buttons & NAME`. Place the poll at the top of `on_level_tick`, not in the caller, so the variable is in scope when the button macros expand.

---

### Wiring it up

A complete level header and its connection to the game translation unit:

```cpp
// scenes/levels/level_one.h
#pragma once

#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS

void set_player_ship(); // forward decl for functions defined in game translation unit

namespace scenes::levels::level_one {

struct State {
    int   current_enemy = 0;
    float speed_ratio   = 1.0f;
    pulse2d::state::Draw_Fn draw = nullptr;
};

PULSE_DEFINE_SCENE_STATE(State);

PULSE_SCENE_FN void on_level_start(pulse2d_scene_runtime<Scenes...>& game,
    pulse2d::state::Draw_Fn draw_fn)
{
    state.draw = draw_fn;
    state.current_enemy = 0;
    state.speed_ratio   = 1.0f;

    set_player_ship();

    game.set_sprite("enemy_sprite", "enemy.bin")
        .set_dynamic_body("enemy_1",
            { .position = { 20.0f, 0.0f }, .velocity = { -8.0f * state.speed_ratio, 0.0f },
              .width    = px_to_units(32.0f, 32.0f), .mass = 1.0f, .is_sensor = true });
}

PULSE_SCENE_FN void on_level_tick(pulse2d_scene_runtime<Scenes...>& game,
    pulse2d_body& player,
    void (*on_reset)())
{
    PULSE_POLL_SEESAW_GAMEPAD();

    game.set_arcade_directional_inverted_control("player", 12.0f, true);

    if (SEESAW_BUTTON_INPUT(SEESAW_START)) {
        on_reset();
        return;
    }

    game.draw("player", "player_sprite");
    game.draw("enemy_1", "enemy_sprite");
}

} // namespace scenes::levels::level_one
```

```cpp
// src/game.cc
#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS
#include <scenes/levels/level_one.h>

namespace level_one = scenes::levels::level_one;

PULSE2D_START_PULSE();
PULSE_DEFINE_SCENE(Level_One, 4, 6);
PULSE_INIT_GAME(my_game, Level_One);

void set_player_ship()
{
    my_game
        .set_sprite("player_sprite", "ship.bin")
        .set_controlled_body("player",
            { .position = { -4.0f, 0.0f }, .velocity = { 0.0f, 0.0f },
              .width    = { 1.0f, 1.0f }, .mass = 1.0f });
}

PULSE_ON_GAMESCENE_START(Level_One)
{
    level_one::on_level_start(my_game,
        [](pulse2d_body* b, const char* s) { my_game.draw_body(b, s); });
}

PULSE_ON_GAMESCENE(Level_One)
{
    my_game.tick();
    my_game.render_backgrounds();

    pulse2d_body& player = my_game.get_body("player");

    level_one::on_level_tick(my_game, player, [] {
        PULSE_DEFER_SCENE(my_game, Level_One);
    });

    my_game.tick_vfx();
    my_game.render();
}

PULSE_ON_GAMESTART()
{
    my_game.init(0.0f, -10.0f, 10);
    PULSE_ENABLE_SEESAW_GAMEPAD();
    PULSE_SET_SCENE(my_game, Level_One);
}

PULSE_ON_GAMELOOP()
{
    PULSE_TICK_GAMESCENE();
}
```

---

## Complete Example

A complete Space Shooter demonstrating most features:

```cpp
// scenes/levels/space_shooter.h
#pragma once

#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS
#include "../include/explosion-anim.h"
#include "../include/stars-bg.h"

namespace scenes::levels::space_shooter {

struct State {
    int      score     = 0;
    bool     enemy_hit = false;
    uint32_t cooldown  = 0;
};

PULSE_DEFINE_SCENE_STATE(State);

PULSE_SCENE_FN void on_start(pulse2d_scene_runtime<Scenes...>& game)
{
    state = {};

    game
        .set_background_sprite("bg_stars",   stars_bg, 320, 240)
        .set_sprite("ship_sprite",       "ship.bin")
        .set_sprite("enemy_sprite",      "enemy.bin")
        .set_sprite("bullet_sprite",     "bullet.bin")
        .add_parallax_layer("bg_stars",  320.0f, 15.0f)
        .register_vfx("explosion", explosion_frames, 64, 64, 8, 12.0f)
        .init_pool("bullets", {
            .width    = { 0.15f, 0.08f },
            .friction = 0.0f
        })
        .set_controlled_body("ship_object", {
            .position = { -4.0f, 0.0f },
            .width    = { 0.5f,  0.5f }
        })
        .set_dynamic_body("enemy_object", {
            .position = { 3.0f, 0.0f },
            .mass     = 1.0f,
            .width    = { 0.6f, 0.6f }
        });
}

PULSE_SCENE_FN void on_tick(pulse2d_scene_runtime<Scenes...>& game,
    pulse2d_body& ship,
    void (*on_reset)())
{
    PULSE_POLL_SEESAW_GAMEPAD();

    game.set_arcade_directional_control("ship_object", 3.5f);

    if (state.cooldown > 0) state.cooldown--;
    if (SEESAW_BUTTON_INPUT(SEESAW_A) && state.cooldown == 0) {
        game.spawn("bullets", 100,
            ship.position.x + 0.6f, ship.position.y,
            8.0f, 0.0f);
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
    PULSE_ENABLE_SEESAW_GAMEPAD();
    PULSE_SET_SCENE(my_game, Space_Shooter);
}

PULSE_ON_GAMELOOP()
{
    PULSE_TICK_GAMESCENE();
}
```

---

## See Also

- [Physics README](pulse2d/graphics/readme.md) - Physics engine, bodies, sensors
- [Blog series](https://soliloq.uy/tag/pulse2d/) - Embedded development blog
