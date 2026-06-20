# DSL Documentation

The pulse2d DSL is a set of macros in [`pulse2d/dsl.h`](pulse2d/dsl.h) inspired by the [Catch2](https://github.com/catchorg/Catch2) library that enables development of Teensy games. It wraps the engine, physics world, scene management, animations, object pools, and render pipeline into a declarative "fantasy" scripting language, without the need to understand bare-metal embedded programming.

---

## Table of Contents

- [Game State, Setup](#game-state--setup)
  - [PULSE2D_START_PULSE](#pulse2d_start_pulse)
  - [PULSE2D_INIT](#pulse2d_init)
  - [PULSE2D_DEFINE](#pulse2d_define)
  - [PULSE2D_HARDWARE_DEFINE](#pulse2d_hardware_define)
- [Scenes](#scenes)
  - [PULSE2D_DEFINE_SCENE](#pulse2d_define_scene)
  - [PULSE2D_GAME_SCENES](#pulse2d_game_scenes)
  - [PULSE2D_SET_SCENE](#pulse2d_set_scene)
  - [PULSE2D_DEFER_SCENE](#pulse2d_defer_scene)
  - [PULSE2D_ON_GAMESCENE_START](#pulse2d_on_gamescene_start)
  - [PULSE2D_ON_GAMESCENE](#pulse2d_on_gamescene)
  - [PULSE2D_TICK_GAMESCENE](#pulse2d_tick_gamescene)
  - [PULSE2D_TICK_WORLD](#pulse2d_tick_world)
- [Physics, Bodies](#physics--bodies)
  - [PULSE2D_STATIC_BODY](#pulse2d_static_body)
  - [PULSE2D_CONTROLLED_BODY](#pulse2d_controlled_body)
  - [PULSE2D_DYNAMIC_BODY](#pulse2d_dynamic_body)
  - [PULSE2D_GET_BODY](#pulse2d_get_body)
  - [Body Properties](#body-properties)
- [Sprites, Rendering](#sprites--rendering)
  - [PULSE2D_SPRITE](#pulse2d_sprite)
  - [PULSE2D_SPRITE_FLASH](#pulse2d_sprite_flash)
  - [PULSE2D_BODY_COORDINATES](#pulse2d_body_coordinates)
  - [PULSE2D_DRAW](#pulse2d_draw)
  - [PULSE2D_DRAW_BODY](#pulse2d_draw_body)
  - [PULSE2D_RENDER](#pulse2d_render)
- [Backgrounds, Parallax](#backgrounds--parallax)
  - [PULSE2D_ADD_BACKGROUND_LAYER](#pulse2d_add_background_layer)
  - [PULSE2D_ADD_PARALLAX_LAYER](#pulse2d_add_parallax_layer)
  - [PULSE2D_RENDER_BACKGROUNDS](#pulse2d_render_backgrounds)
- [Animations](#animations)
  - [Persistent Animations](#persistent-animations)
  - [PULSE2D_DEFINE_ANIMATOR](#pulse2d_define_animator)
  - [PULSE2D_ANIMATION_DEFINITION](#pulse2d_animation_definition)
  - [PULSE2D_SET_ANIMATION](#pulse2d_set_animation)
  - [PULSE2D_TICK_ANIMATION](#pulse2d_tick_animation)
  - [VFX, One-shot](#vfx-one-shot)
  - [PULSE2D_DEFINE_VFX](#pulse2d_define_vfx)
  - [PULSE2D_PLAY_VFX](#pulse2d_play_vfx)
  - [PULSE2D_TICK_VFX](#pulse2d_tick_vfx)
- [Kinematic Pools](#kinematic-pools)
  - [PULSE2D_INIT_POOL](#pulse2d_init_pool)
  - [PULSE2D_SPAWN](#pulse2d_spawn)
  - [PULSE2D_DESPAWN](#pulse2d_despawn)
  - [PULSE2D_RENDER_POOL](#pulse2d_render_pool)
- [Collision](#collision)
  - [PULSE2D_ON_COLLISION](#pulse2d_on_collision)
  - [PULSE2D_ON_COLLISION_WITH](#pulse2d_on_collision_with)
  - [PULSE2D_ON_COLLISION_WITH_BODY](#pulse2d_on_collision_with_body)
- [Gamepad Input](#gamepad-input)
  - [PULSE2D_ENABLE_SEESAW_GAMEPAD](#pulse2d_enable_seesaw_gamepad)
  - [PULSE2D_START_SEESAW_GAMEPAD](#pulse2d_start_seesaw_gamepad)
  - [PULSE2D_POLL_SEESAW_GAMEPAD](#pulse2d_poll_seesaw_gamepad)
  - [SEESAW_BUTTON_INPUT](#seesaw_button_input)
  - [SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL](#seesaw_set_arcade_directional_control)
  - [SEESAW_SET_ARCADE_DIRECTIONAL_INVERTED_CONTROL](#seesaw_set_arcade_directional_inverted_control)
  - [SEESAW_SETDYNAMIC_DIRECTIONAL_CONTROL](#seesaw_set_dynamic_directional_control)
  - [SEESAW_SET_SLIDING_FRICTION_DIRECTIONAL_CONTROL](#seesaw_set_sliding_friction_directional_control)
  - [Analog Stick Input](#analog-stick-input)
  - [Direction Helpers](#direction-helpers)
- [Engine Lifecycle](#engine-lifecycle)
  - [PULSE2D_ON_GAMESTART](#pulse2d_on_gamestart)
  - [PULSE2D_ON_GAMELOOP](#pulse2d_on_gameloop)
  - [PULSE2D_TICK_PULSE](#pulse2d_tick_pulse)
- [Debug, Utilities](#debug--utilities)
  - [PULSE2D_PRINT_STACKSIZE](#pulse2d_print_stacksize)
  - [PULSE2D_REGISTER_ETL_ERROR_HANDLER](#pulse2d_register_etl_error_handler)

---

## Game State, Setup

### PULSE2D_START_PULSE

```cpp
PULSE2D_START_PULSE();
```

Declares the engine, physics world, and two function pointers that control scene dispatch. Place this at file scope, once per game.

It creates:

- `engine` - Hardware-deferred pulse2d engine instance
- `world` - Hardware-deferred physics world instance
- `pending_transition` - Scene transition function pointer
- `active_scene_fn` - Current scene tick function pointer
- `PULSE` - Constant frame delta (1/60th second)

**Scope:** `global`

**Example:**
```cpp
#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS

PULSE2D_START_PULSE();
PULSE2D_ENABLE_SEESAW_GAMEPAD();

// ... rest of game
```

---

### PULSE2D_INIT

```cpp
PULSE2D_INIT(gravity_x, gravity_y, solver_iterations);
```

Initializes the engine and physics world. The first two arguments are the gravity vector components (typically `0.0f, 0.0f` for space or `0.0f, -9.8f` for platformers). The third argument is the solver iteration count (10 is a good default).

**Scope:** `PULSE2D_ON_GAMESTART`

**Parameters:**
- `gravity_x` - Horizontal gravity component
- `gravity_y` - Vertical gravity component
- `solver_iterations` - Physics solver iteration count (higher = more accurate, slower)

**Example:**
```cpp
PULSE2D_ON_GAMESTART() {
    Serial.begin(115200);
    PULSE2D_INIT(0.0f, 0.0f, 10);      // zero gravity
    PULSE2D_INIT(0.0f, -9.8f, 10);     // platformer gravity
}
```

---

### PULSE2D_DEFINE

```cpp
PULSE2D_DEFINE type variable_name = initial_value;
```

Allocates a static variable in the correct memory section for game state. Use this for any persistent game variables.

**Scope:** `global`

**Example:**
```cpp
PULSE2D_DEFINE bool game_over = false;
PULSE2D_DEFINE int score = 0;
PULSE2D_DEFINE float player_health = 100.0f;
```

---

### PULSE2D_HARDWARE_DEFINE

```cpp
PULSE2D_HARDWARE_DEFINE(Type) variable_name;
```

Declares a hardware-deferred type for late initialization. Used internally by `PULSE2D_START_PULSE()`. You typically don't need to use this directly unless you're creating custom hardware wrappers.

**Scope:** `global`

---

## Scenes

Scenes are the organizational unit for game levels, menus, and states. Each scene has its own pools of bodies, sprites, and joints.

### PULSE2D_DEFINE_SCENE

```cpp
PULSE2D_DEFINE_SCENE(scene_name, max_bodies, max_sprites);
PULSE2D_DEFINE_SCENE(scene_name, max_bodies, max_sprites, max_joints);
```

Declares a scene struct with fixed-size pools. The sizes are checked at compile time against hardware limits. The optional fourth argument sets the joint pool size (default: 0).

**Scope:** `global`

**Parameters:**
- `scene_name` - Identifier for the scene (becomes a struct name)
- `max_bodies` - Maximum physics bodies (checked against `MAX_PHYSICS_BODIES`)
- `max_sprites` - Maximum loaded sprites (checked against `MAX_LOADED_SPRITES`)
- `max_joints` - Optional joint pool size (default: 0)

**Example:**
```cpp
PULSE2D_DEFINE_SCENE(Main_Menu, 2, 5);         // 2 bodies, 5 sprites
PULSE2D_DEFINE_SCENE(Game_Level, 10, 8);       // 10 bodies, 8 sprites
PULSE2D_DEFINE_SCENE(Boss_Fight, 15, 12, 4);   // 15 bodies, 12 sprites, 4 joints
```

---

### PULSE2D_GAME_SCENES

```cpp
PULSE2D_GAME_SCENES(Scene1, Scene2, ...);
```

Declares a `std::variant` that holds all scene types used in the game. This creates the `current_scene` global variable.

**Scope:** `global`

**Example:**
```cpp
PULSE2D_GAME_SCENES(Main_Menu, Game_Level, Boss_Fight);
```

---

### PULSE2D_SET_SCENE

```cpp
PULSE2D_SET_SCENE(scene_name);
```

Transitions to a scene, this:

1. Clears the physics world
2. Resets the storage system
3. Emplaces the new scene into `current_scene`
4. Calls the scene's entry function (`PULSE2D_ON_GAMESCENE_START`)
5. Registers the scene's tick function

**Scope:** `PULSE2D_ON_GAMESTART`, `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
PULSE2D_SET_SCENE(Game_Level);

// From inside a scene, defer the transition:
if (player_dead) {
    PULSE2D_DEFER_SCENE(Game_Over);
}
```

---

### PULSE2D_DEFER_SCENE

```cpp
PULSE2D_DEFER_SCENE(scene_name)
```

Defer scene transition to another scene on next scene tick.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
PULSE2D_ON_GAMESCENE(Level_Two)
{
    // should we reset?
    if (player.position.x > 5.5f or SEESAW_BUTTON_INPUT(SEESAW_START)) {
        PULSE2D_DEFER_SCENE(Level_One);
    }
}
```

---

### PULSE2D_ON_GAMESCENE_START

```cpp
PULSE2D_ON_GAMESCENE_START(scene_name) {
    // Initialization code
}
```

Defines the entry function for a scene. Called once automatically by `PULSE2D_SET_SCENE`. This is where you spawn bodies, load sprites, and set up the scene.

**Scope:** `global`

**Example:**
```cpp
PULSE2D_ON_GAMESCENE_START(Game_Level) {
    PULSE2D_STATIC_BODY(floor_object, {
        .position = { 0.0f, -5.0f },
        .width = { 10.0f, 0.5f }
    });

    PULSE2D_DYNAMIC_BODY(player_object, {
        .position = { 0.0f, 2.0f },
        .mass = 1.0f
    });

    PULSE2D_SPRITE(player_sprite, "player.bin", 32, 32);
}
```

---

### PULSE2D_ON_GAMESCENE

```cpp
PULSE2D_ON_GAMESCENE(Scene_Name) {
    // Per-frame logic
}
```

Defines the per-frame function for a scene. This is registered as the active tick function by `PULSE2D_SET_SCENE` and called every frame by `PULSE2D_TICK_GAMESCENE`.

**Scope:** `global`

**Example:**
```cpp
PULSE2D_ON_GAMESCENE(Game_Level) {
    PULSE2D_TICK_WORLD(Game_Level);
    PULSE2D_POLL_SEESAW_GAMEPAD();

    SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL(player_object, 3.0f);

    PULSE2D_DRAW(player_object, player_sprite);
    PULSE2D_RENDER(active_scene);
}
```

---

### PULSE2D_TICK_GAMESCENE

```cpp
PULSE2D_TICK_GAMESCENE();
```

Calls the active scene's tick function, then resolves any pending transition. This is the only call needed in `PULSE2D_ON_GAMELOOP`.

**Scope:** `PULSE2D_ON_GAMELOOP`

**Example:**
```cpp
PULSE2D_ON_GAMELOOP() {
    PULSE2D_TICK_GAMESCENE();
}
```

---

### PULSE2D_TICK_WORLD

```cpp
PULSE2D_TICK_WORLD(scene_name);
```

Steps the physics simulation one frame and brings `active_scene` and `renderer` into scope for the rest of the scene function. Call this at the top of every `PULSE2D_ON_GAMESCENE`.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
PULSE2D_ON_GAMESCENE(Game_Level) {
    PULSE2D_TICK_WORLD(Game_Level);
    // active_scene and renderer are now in scope
    auto& player = PULSE2D_GET_BODY(player_object);
    player.apply_force({ 0.0f, 10.0f });
}
```

---

## Physics, Bodies

Bodies are the physical objects in your game. There are three types:

1. Fixed - Immovable obstacles (walls, floors, platforms)
2. Controlled - Player-controlled objects with `mass = 0` (infinite mass, but can be moved by setting velocity directly)
3. Dynamic - Fully simulated objects with active physics

### PULSE2D_STATIC_BODY

```cpp
PULSE2D_STATIC_BODY(object_name, {
    // Body descriptor
});
```

Allocates an immovable body in the current scene's pool and registers it with the physics world. Use for walls, floors, and static obstacles.

**Scope:** `PULSE2D_ON_GAMESCENE_START`

**Example:**
```cpp
PULSE2D_STATIC_BODY(floor, {
    .position = { 0.0f, -5.0f },
    .width = { 10.0f, 0.5f }
});

PULSE2D_STATIC_BODY(left_wall, {
    .position = { -6.0f, 0.0f },
    .width = { 0.5f, 8.0f }
});
```

---

### PULSE2D_CONTROLLED_BODY

```cpp
PULSE2D_CONTROLLED_BODY(object_name, {
    // Body descriptor with mass = 0
});
```

Allocates a player-controlled body with `mass = 0` (infinite mass). The body doesn't respond to forces, but you can set its velocity directly for responsive control.

**Scope:** `PULSE2D_ON_GAMESCENE_START`

**Example:**
```cpp
PULSE2D_CONTROLLED_BODY(player, {
    .position = { 0.0f, 0.0f },
    .velocity = { 0.0f, 0.0f },
    .width = { 1.0f, 1.0f },
    .mass = 1.0f  // Set to 0.0f by the macro
});
```

---

### PULSE2D_DYNAMIC_BODY

```cpp
PULSE2D_DYNAMIC_BODY(object_name, {
    // Body descriptor with mass > 0
});
```

Allocates a dynamic body with active, in-motion physics and registers it with the world. The body responds to forces, gravity, and collisions.

**Scope:** `PULSE2D_ON_GAMESCENE_START`

**Example:**
```cpp
PULSE2D_DYNAMIC_BODY(ball, {
    .position = { 0.0f, 5.0f },
    .velocity = { 2.0f, 0.0f },
    .width = { 0.5f, 0.5f },
    .mass = 1.0f
});
```

---

### PULSE2D_GET_BODY

```cpp
auto& body_ref = PULSE2D_GET_BODY(object_name);
```

Returns a reference to a named body from the active scene. Available after `PULSE2D_TICK_WORLD`.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
PULSE2D_ON_GAMESCENE(Game_Level) {
    PULSE2D_TICK_WORLD(Game_Level);

    auto& player = PULSE2D_GET_BODY(player_object);
    player.set_velocity({ 3.0f, 0.0f });

    if (player.position.y < -10.0f) {
        // player fell off the map
    }
}
```

---

### Body Properties

All body descriptors support these fields (see [`pulse2d/graphics/body.h`](pulse2d/graphics/body.h) for details):

```cpp
{
    .position = { x, y },           // Vec2: initial position
    .velocity = { vx, vy },         // Vec2: initial velocity
    .force = { fx, fy },            // Vec2: accumulated force
    .width = { w, h },              // Vec2: half-extents (half-width, half-height)
    .mass = 1.0f,                   // float: mass (0.0f = infinite)
    .friction = 0.2f,               // float: coefficient of friction
    .rotation = 0.0f                // float: rotation in radians
}
```

**Example:**
```cpp
PULSE2D_DYNAMIC_BODY(crate, {
    .position = { 2.0f, 3.0f },
    .width = { 0.5f, 0.5f },
    .mass = 2.0f,
    .friction = 0.8f
});
```

---

## Sprites, Rendering

### PULSE2D_SPRITE

```cpp
PULSE2D_SPRITE(sprite_name, "path/to/file.bin", width, height);
```

Loads a raw RGB565 sprite from the SD card into the current scene's sprite pool. The path is relative to the SD card root.

**Scope:** `PULSE2D_ON_GAMESCENE_START`

**Example:**
```cpp
PULSE2D_SPRITE(player_sprite, "player.bin", 32, 32);
PULSE2D_SPRITE(enemy_sprite, "sprites/enemy.bin", 48, 48);
```

---

### PULSE2D_SPRITE_FLASH

```cpp
PULSE2D_SPRITE_FLASH(sprite_name, data_array, width, height);
```

Registers a flash-resident sprite array (from a C header generated by `png2header`) as a named sprite in the current scene's sprite pool. Use this for backgrounds and large assets that stay in QSPI flash.

**Scope:** `PULSE2D_ON_GAMESCENE_START`

**Example:**
```cpp
#include "../include/nebula-bg.h"

PULSE2D_ON_GAMESCENE_START(Level_One) {
    PULSE2D_SPRITE_FLASH(sprite_nebula, bg_1, 320, 240);
}
```

---

### PULSE2D_DRAW

```cpp
PULSE2D_DRAW(body_name, sprite_name);
PULSE2D_DRAW(body_name, sprite_name, rotation_radians);
```

Projects a body's world-space position to screen coordinates and queues the sprite for rendering. An optional third argument sets a fixed rotation in radians.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Important:** Pass identifiers, not string literals. The macro stringifies them.

**Example:**
```cpp
PULSE2D_DRAW(player_object, player_sprite);
PULSE2D_DRAW(ship_object, ship_sprite, 1.5708f);  // 90° rotation
```

```cpp
// WRONG - do not use string literals
PULSE2D_DRAW("player", "player_sprite");

// CORRECT - use identifiers
PULSE2D_DRAW(player_object, player_sprite);
```

---

### PULSE2D_DRAW_BODY

```cpp
PULSE2D_DRAW_BODY(body_pointer, sprite_name);
PULSE2D_DRAW_BODY(body_pointer, sprite_name, rotation_radians);
```

Same as `PULSE2D_DRAW`, but takes a body pointer instead of a name. Useful when drawing pooled objects.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
PULSE2D_RENDER_POOL(gun_ammo,
    [](auto* bullet) {
        PULSE2D_DRAW_BODY(bullet, bullet_sprite);
    }
);
```

---

### PULSE2D_BODY_COORDINATES

```cpp
PULSE2D_BODY_COORDINATES(player_object);
```

Get the projected `x, y` pixel coordinates of a physics body on the screen.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
// Play at a body's position
auto& enemy = PULSE2D_GET_BODY(enemy_object);
auto [sx, sy] = PULSE2D_BODY_COORDINATES(enemy);
PULSE2D_PLAY_VFX(explosion, sx, sy);
```

---

### PULSE2D_RENDER

```cpp
PULSE2D_RENDER(active_scene);
```

Flushes the renderer's sprite queue to the display. Call once at the end of your scene function.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
PULSE2D_ON_GAMESCENE(Game_Level) {
    PULSE2D_TICK_WORLD(Game_Level);
    PULSE2D_DRAW(player_object, player_sprite);
    PULSE2D_DRAW(enemy_object, enemy_sprite);
    PULSE2D_RENDER(active_scene);
}
```

---

## Backgrounds, Parallax

The background system renders full-screen sprites in layers. Parallax layers scroll at different speeds to create depth.

### PULSE2D_ADD_BACKGROUND_LAYER

```cpp
PULSE2D_ADD_BACKGROUND_LAYER(sprite_name, width);
```

Adds a static (non-scrolling) background layer to the current scene. Call after `PULSE2D_SPRITE_FLASH`.

**Scope:** `PULSE2D_ON_GAMESCENE_START`

**Example:**
```cpp
PULSE2D_ON_GAMESCENE_START(Menu_Screen) {
    PULSE2D_SPRITE_FLASH(menu_bg, menu_bg_data, 320, 240);
    PULSE2D_ADD_BACKGROUND_LAYER(menu_bg, 320.0f);
}
```

---

### PULSE2D_ADD_PARALLAX_LAYER

```cpp
PULSE2D_ADD_PARALLAX_LAYER(sprite_name, width, scroll_speed);
```

Adds a parallax scrolling layer to the current scene. `width` is the image width in pixels (used to wrap the offset), `scroll_speed` is pixels per second. Layers are drawn in the order they're added.

**Scope:** `PULSE2D_ON_GAMESCENE_START`

**Example:**
```cpp
PULSE2D_ON_GAMESCENE_START(Space_Level) {
    PULSE2D_SPRITE_FLASH(sprite_nebula, bg_1, 320, 240);
    PULSE2D_SPRITE_FLASH(sprite_stars, bg_2, 320, 240);
    PULSE2D_SPRITE_FLASH(sprite_dust, bg_3, 320, 240);

    PULSE2D_ADD_PARALLAX_LAYER(sprite_nebula, 320.0f, 10.0f);  // slow
    PULSE2D_ADD_PARALLAX_LAYER(sprite_stars, 320.0f, 3.0f);    // very slow
    PULSE2D_ADD_PARALLAX_LAYER(sprite_dust, 320.0f, 65.0f);    // fast
}
```

---

### PULSE2D_RENDER_BACKGROUNDS

```cpp
PULSE2D_RENDER_BACKGROUNDS();
```

#### Warning: Sprites are FIFO, always call PULSE2D_RENDER_BACKGROUNDS before drawing any other sprites

Advances each layer's scroll offset and blits all layers to the framebuffer. Call before `PULSE2D_RENDER` in your scene function.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
PULSE2D_ON_GAMESCENE(Space_Level) {
    PULSE2D_TICK_WORLD(Space_Level);
    PULSE2D_POLL_SEESAW_GAMEPAD();

    PULSE2D_RENDER_BACKGROUNDS();  // draw backgrounds first

    PULSE2D_DRAW(ship_object, ship_sprite);
    PULSE2D_RENDER(active_scene);
}
```

---

## Animations

Two animation systems serve different purposes. **Persistent animations** drive looped character and enemy states - idle, walk, jump - by mutating a sprite's frame pointer each tick. **VFX, one-shot animations** fire and forget: they play once and are removed automatically, suited for explosions, impacts, and pickup effects.

---

### Persistent Animations

A `Sprite_Animator` instance holds the current playback position and a reference to the active `Animation_Def` blueprint. Declare both at file scope and drive the animator each frame with `PULSE2D_TICK_ANIMATION`. Swap the definition mid-game with `PULSE2D_SET_ANIMATION` to change the active clip without touching the sprite pool.

### PULSE2D_DEFINE_ANIMATOR

```cpp
PULSE2D_DEFINE_ANIMATOR(animator_name);
```

Declares a named `Sprite_Animator` at file scope. The instance persists across frames and scenes. Declare it alongside your other global state, after `PULSE2D_START_PULSE()`.

**Scope:** `global`

**Example:**
```cpp
PULSE2D_START_PULSE();
PULSE2D_ENABLE_SEESAW_GAMEPAD();

PULSE2D_DEFINE_ANIMATOR(player_animator);
```

---

### PULSE2D_ANIMATION_DEFINITION

```cpp
PULSE2D_ANIMATION_DEFINITION(name, sheet_ptr, frame_width, frame_height, total_frames, fps);
```

Defines an immutable animation blueprint as a `static constexpr Animation_Def`. The `time_per_frame` value (`1.0f / fps`) is computed at compile time. Declare definitions at file scope so they're available everywhere.

**Scope:** `global`

**Parameters:**
- `name` - Identifier for this animation definition
- `sheet_ptr` - Pointer to the flash sprite sheet array (from `animation2header`)
- `frame_width` - Width of each frame in pixels
- `frame_height` - Height of each frame in pixels
- `total_frames` - Number of frames in the animation
- `fps` - Playback rate in frames per second

**Example:**
```cpp
#include "../include/player-anim.h"  // idle_frames[], walk_frames[], jump_frames[]

PULSE2D_ANIMATION_DEFINITION(anim_idle, idle_frames, 32, 48, 4, 8);
PULSE2D_ANIMATION_DEFINITION(anim_walk, walk_frames, 32, 48, 6, 12);
PULSE2D_ANIMATION_DEFINITION(anim_jump, jump_frames, 32, 48, 3, 10);
```

---

### PULSE2D_SET_ANIMATION

```cpp
PULSE2D_SET_ANIMATION(animator_inst, anim_def);
```

Loads an animation definition into a running animator. The animator resets its accumulator and begins playing `anim_def` from frame 0 on the next `PULSE2D_TICK_ANIMATION` call. Call this whenever the character's state changes.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
if (SEESAW_DIRECTION_IS_LEFT() || SEESAW_DIRECTION_IS_RIGHT()) {
    PULSE2D_SET_ANIMATION(player_animator, anim_walk);
} else {
    PULSE2D_SET_ANIMATION(player_animator, anim_idle);
}
```

---

### PULSE2D_TICK_ANIMATION

```cpp
PULSE2D_TICK_ANIMATION(animator_inst, sprite_name);
```

Advances the animator's frame accumulator and writes the current frame's pixel pointer directly into the named sprite. Call once per frame inside `PULSE2D_ON_GAMESCENE`, after input handling and before `PULSE2D_RENDER`.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
PULSE2D_TICK_ANIMATION(player_animator, player_sprite);
PULSE2D_DRAW(player_object, player_sprite);
PULSE2D_RENDER(active_scene);
```

**Complete persistent animation example:**
```cpp
#include "../include/player-anim.h"

PULSE2D_DEFINE_SCENE(Platformer, 5, 3);
PULSE2D_GAME_SCENES(Platformer);

PULSE2D_DEFINE_ANIMATOR(player_animator);

PULSE2D_ANIMATION_DEFINITION(anim_idle, idle_frames, 32, 48, 4, 8);
PULSE2D_ANIMATION_DEFINITION(anim_walk, walk_frames, 32, 48, 6, 12);

PULSE2D_ON_GAMESCENE_START(Platformer) {
    PULSE2D_SPRITE_FLASH(player_sprite, idle_frames, 32, 48);
    PULSE2D_CONTROLLED_BODY(player_object, {
        .position = { -3.0f, 0.0f },
        .width = { 0.5f, 0.75f }
    });

    PULSE2D_SET_ANIMATION(player_animator, anim_idle);
}

PULSE2D_ON_GAMESCENE(Platformer) {
    PULSE2D_TICK_WORLD(Platformer);
    PULSE2D_POLL_SEESAW_GAMEPAD();

    SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL(player_object, 3.0f);

    if (SEESAW_DIRECTION_IS_LEFT() || SEESAW_DIRECTION_IS_RIGHT()) {
        PULSE2D_SET_ANIMATION(player_animator, anim_walk);
    } else {
        PULSE2D_SET_ANIMATION(player_animator, anim_idle);
    }

    PULSE2D_TICK_ANIMATION(player_animator, player_sprite);
    PULSE2D_DRAW(player_object, player_sprite);
    PULSE2D_RENDER(active_scene);
}
```

---

### VFX, One-shot

VFX animations are registered once in `PULSE2D_ON_GAMESCENE_START`, then triggered at any screen position with `PULSE2D_PLAY_VFX`. Each instance plays once and is automatically removed when complete. If the animation queue is full, the request is silently dropped.

### PULSE2D_DEFINE_VFX

```cpp
PULSE2D_DEFINE_VFX(anim_name, data_ptr, frame_width, frame_height, total_frames, fps);
```

Registers a VFX animation definition in the current scene's animation manager. The sprite sheet is a linear array of frames in flash memory (generated by `animation2header`).

**Scope:** `PULSE2D_ON_GAMESCENE_START`

**Parameters:**
- `anim_name` - Identifier for the animation
- `data_ptr` - Pointer to the flash sprite sheet array
- `frame_width` - Width of each frame in pixels
- `frame_height` - Height of each frame in pixels
- `total_frames` - Number of frames in the animation
- `fps` - Frames per second playback rate

**Example:**
```cpp
#include "../include/explosion-anim.h"  // explosion_frames[8][64*64]

PULSE2D_ON_GAMESCENE_START(Game_Level) {
    PULSE2D_DEFINE_VFX(explosion, explosion_frames, 64, 64, 8, 12);
}
```

---

### PULSE2D_PLAY_VFX

```cpp
PULSE2D_PLAY_VFX(anim_name, x, y);
```

Spawns a new VFX animation instance at the given screen coordinates. Plays once and is automatically removed when complete. If the animation queue is full, the request is silently dropped.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
// Play at a body's position on any collision
if (PULSE2D_OBJECTS_COLLIDED()) {
    auto& enemy = PULSE2D_GET_BODY(enemy_object);
    auto [sx, sy] = PULSE2D_BODY_COORDINATES((&enemy));
    PULSE2D_PLAY_VFX(explosion, sx, sy);
}

// Or at a fixed screen position
PULSE2D_PLAY_VFX(explosion, 160, 120);  // center of screen
```

---

### PULSE2D_TICK_VFX

```cpp
PULSE2D_TICK_VFX();
```

Advances and draws all active VFX animations. Call after rendering game objects but before `PULSE2D_RENDER`.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
PULSE2D_ON_GAMESCENE(Game_Level) {
    PULSE2D_TICK_WORLD(Game_Level);
    PULSE2D_POLL_SEESAW_GAMEPAD();

    PULSE2D_DRAW(player_object, player_sprite);
    PULSE2D_DRAW(enemy_object, enemy_sprite);

    PULSE2D_TICK_VFX();  // draw VFX on top

    PULSE2D_RENDER(active_scene);
}
```

**Complete VFX example:**
```cpp
#include "../include/explosion-anim.h"

PULSE2D_DEFINE_SCENE(Shooter, 10, 5);
PULSE2D_GAME_SCENES(Shooter);

PULSE2D_ON_GAMESCENE_START(Shooter) {
    PULSE2D_DEFINE_VFX(explosion, explosion_frames, 64, 64, 8, 12);
    PULSE2D_CONTROLLED_BODY(player, {
        .position = { -3.0f, 0.0f },
        .width = { 0.5f, 0.5f }
    });
    PULSE2D_DYNAMIC_BODY(enemy, {
        .position = { 3.0f, 0.0f },
        .mass = 1.0f
    });
    PULSE2D_SPRITE(enemy_sprite, "enemy.bin", 48, 48);
}

PULSE2D_DEFINE bool enemy_destroyed = false;

PULSE2D_ON_GAMESCENE(Shooter) {
    PULSE2D_TICK_WORLD(Shooter);
    PULSE2D_POLL_SEESAW_GAMEPAD();

    auto& player_body = PULSE2D_GET_BODY(player);
    auto& enemy_body  = PULSE2D_GET_BODY(enemy);

    SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL(player, 4.0f);

    PULSE2D_ON_COLLISION(&player_body, &enemy_body, [&] {
        if (!enemy_destroyed) {
            enemy_destroyed = true;
            auto [sx, sy] = PULSE2D_BODY_COORDINATES((&enemy_body));
            PULSE2D_PLAY_VFX(explosion, sx, sy);
        }
    });

    if (!enemy_destroyed) {
        PULSE2D_DRAW(enemy, enemy_sprite);
    }

    PULSE2D_TICK_VFX();
    PULSE2D_RENDER(active_scene);
}
```

---

## Kinematic Pools

Kinematic pools provide pre-allocated object pools for temporary entities like projectiles, particles, and powerups. The pool manager is part of every scene and allows you to create named pool instances with different templates.

### PULSE2D_INIT_POOL

```cpp
PULSE2D_INIT_POOL(pool_name, {
    // Body descriptor
});
```

Initializes a named pool instance in the current scene with a body descriptor template. The descriptor defines the baseline properties for all objects spawned from this pool. The macro automatically sets `mass = 0.0f` to ensure controlled (non-physics) behavior.

**Scope:** `PULSE2D_ON_GAMESCENE_START`

**Example:**
```cpp
PULSE2D_ON_GAMESCENE_START(Shooter) {
    // Create a bullet pool
    PULSE2D_INIT_POOL(bullet_pool, {
        .width = { 0.2f, 0.1f },
        .friction = 0.0f
    });

    // Create a particle pool
    PULSE2D_INIT_POOL(particle_pool, {
        .width = { 0.05f, 0.05f }
    });
}
```

---

### PULSE2D_SPAWN

```cpp
PULSE2D_SPAWN(pool_name, delay, x, y, vx, vy);
```

Spawns an object from the named pool. The body is initialized with the pool's descriptor template, then its position and velocity are set, and it's added to the physics world. The delay parameter is a "delay" timer in ms between spawns.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Parameters:**
- `pool_name` - The pool initialized with `PULSE2D_INIT_POOL`
- `delay` - The delay time between spawns in `ms`
- `x, y` - Initial position
- `vx, vy` - Initial velocity

**Example:**
```cpp
PULSE2D_ON_GAMESCENE(Shooter) {
    PULSE2D_TICK_WORLD(Shooter);
    PULSE2D_POLL_SEESAW_GAMEPAD();

    if (SEESAW_BUTTON_INPUT(SEESAW_A)) {
        auto& ship = PULSE2D_GET_BODY(ship_object);

        // Fire bullet from ship position
        PULSE2D_SPAWN(bullet_pool,
            250,                               // 250ms delay between each bullet
            ship.position.x, ship.position.y,  // position
            5.0f, 0.0f);                       // velocity
    }

    // Update and render bullets each frame
    PULSE2D_RENDER_POOL(bullet_pool, [&](auto* bullet) {
        if (bullet.position.x > 10.0f) {
            PULSE2D_DESPAWN(bullet_pool, bullet);
        } else {
            PULSE2D_DRAW_BODY(bullet, bullet_sprite);
        }
    });
}
```

---

### PULSE2D_DESPAWN

```cpp
PULSE2D_DESPAWN(pool_name, body_ptr);
```

Safely releases a pooled object and returns its memory to the named pool. The object is removed from the physics world and marked as available for reuse.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Parameters:**
- `pool_name` - The pool to return the object to
- `body_ptr` - A pointer to the body to despawn

---

### PULSE2D_RENDER_POOL

```cpp
PULSE2D_RENDER_POOL(pool_name,
    [](auto* body) {
        // Update, draw, or despawn logic
    }
);
```

Iterates over all active objects in a pool and executes an action for each. The lambda receives a pointer to each active body. Iterates backwards to allow safe despawning during iteration.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Parameters:**
- `pool_name` - The pool initialized with `PULSE2D_INIT_POOL`
- `action` - Closure that receives a `Body*` for each active object

**Example:**
```cpp
// Update and render all active bullets
PULSE2D_RENDER_POOL(bullet_pool, [&](auto* bullet) {
    // Check bounds and despawn off-screen bullets
    if (bullet.position.x > 10.0f || bullet.position.x < -10.0f) {
        PULSE2D_DESPAWN(bullet_pool, bullet);
        return;
    }

    // Draw bullets still in bounds
    PULSE2D_DRAW_BODY(bullet, bullet_sprite);
});
```

#### A complete Projectile example

```cpp
PULSE2D_DEFINE_SCENE(Shooter, 20, 3);  // 20 bodies for player + enemies + bullets
PULSE2D_GAME_SCENES(Shooter);

PULSE2D_ON_GAMESCENE_START(Shooter) {
    // Initialize bullet pool
    PULSE2D_INIT_POOL(bullet_pool, {
        .width = { 0.2f, 0.1f }
    });

    PULSE2D_CONTROLLED_BODY(ship, {
        .position = { -4.0f, 0.0f },
        .width = { 0.5f, 0.5f }
    });

    PULSE2D_SPRITE(ship_sprite, "ship.bin", 48, 48);
    PULSE2D_SPRITE(bullet_sprite, "bullet.bin", 16, 8);
}

PULSE2D_DEFINE uint32_t fire_cooldown = 0;

PULSE2D_ON_GAMESCENE(Shooter) {
    PULSE2D_TICK_WORLD(Shooter);
    PULSE2D_POLL_SEESAW_GAMEPAD();

    auto& ship = PULSE2D_GET_BODY(ship);
    SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL(ship, 3.0f);

    // Fire bullets
    if (fire_cooldown > 0) fire_cooldown--;

    if (SEESAW_BUTTON_INPUT(SEESAW_A) && fire_cooldown == 0) {
        PULSE2D_SPAWN(bullet_pool,
            250,
            ship.position.x + 0.6f, ship.position.y,
            8.0f, 0.0f);
        fire_cooldown = 15;  // 15 frames = 0.25s @ 60fps
    }

    // Update and draw all active bullets
    PULSE2D_RENDER_POOL(bullet_pool, [&](auto* bullet) {
        if (bullet.position.x > 8.0f) {
            PULSE2D_DESPAWN(bullet_pool, bullet);
        } else {
            PULSE2D_DRAW_BODY(bullet, bullet_sprite);
        }
    });

    PULSE2D_DRAW(ship, ship_sprite);
    PULSE2D_RENDER(active_scene);
}
```

---

## Collision

### PULSE2D_ON_COLLISION

```cpp
PULSE2D_ON_COLLISION(body_ptr_a, body_ptr_b, action);
```

Fires `action` when a single arbiter holds exactly both `body_ptr_a` and `body_ptr_b`. Use this inside `PULSE2D_RENDER_POOL` when you have a pointer to a pooled object and want to detect its collision with a specific named body. `action` is a callable — typically a lambda.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Parameters:**
- `body_ptr_a` — first body pointer
- `body_ptr_b` — second body pointer
- `action` — callable invoked when both pointers appear in the same arbiter

**Example:**
```cpp
PULSE2D_RENDER_POOL(laser_ammo, [&](auto* laser_object) {
    auto& meteor = PULSE2D_GET_BODY(meteor_object);

    if (laser_object->position.x > 6.67f) {
        PULSE2D_DESPAWN(laser_ammo, laser_object);
    } else {
        PULSE2D_DRAW_BODY(laser_object, laser_sprite);
    }

    PULSE2D_ON_COLLISION(laser_object, &meteor, [&] {
        PULSE2D_DESPAWN(laser_ammo, laser_object);
        score += 10;
    });
});
```

---

### PULSE2D_ON_COLLISION_WITH

```cpp
PULSE2D_ON_COLLISION_WITH(body_name, action);
```

Iterates all active arbiters and calls `action` for each one where either body in the pair matches `body_name`. The matching arbiter is erased after the action fires, so each collision is handled once per frame. `action` is a callable — typically a lambda.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
PULSE2D_ON_COLLISION_WITH(enemy_object, [&]() {
    if (!enemy_hit) {
        enemy_hit = true;
        health -= 10;
    }
});

PULSE2D_ON_COLLISION_WITH(powerup_object, [&]() {
    score += 100;
    PULSE2D_SET_SCENE(Level_One);
});
```

---

### PULSE2D_ON_COLLISION_WITH_BODY

```cpp
PULSE2D_ON_COLLISION_WITH_BODY(body_ptr, action);
```

Like `PULSE2D_ON_COLLISION_WITH`, but matches by body pointer instead of name. Use this inside `PULSE2D_RENDER_POOL` lambdas where you already have a pointer to the active object and want to detect any collision it is involved in regardless of the other body.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
PULSE2D_RENDER_POOL(bullet_pool, [&](auto* bullet) {
    PULSE2D_ON_COLLISION_WITH_BODY(bullet, [&]() {
        PULSE2D_DESPAWN(bullet_pool, bullet);
        score += 10;
    });

    if (bullet->position.x > 6.0f) {
        PULSE2D_DESPAWN(bullet_pool, bullet);
    } else {
        PULSE2D_DRAW_BODY(bullet, bullet_sprite);
    }
});
```

---

## Gamepad Input

The gamepad system supports the [Adafruit Seesaw Gamepad QT](https://www.adafruit.com/product/5743) with analog thumbstick and 6 buttons over I2C.

### PULSE2D_ENABLE_SEESAW_GAMEPAD

```cpp
PULSE2D_ENABLE_SEESAW_GAMEPAD();
```

Declares the I2C driver and gamepad at file scope. Place this once alongside `PULSE2D_START_PULSE()`.

**Scope:** `global`

**Example:**
```cpp
PULSE2D_START_PULSE();
PULSE2D_ENABLE_SEESAW_GAMEPAD();
```

---

### PULSE2D_START_SEESAW_GAMEPAD

```cpp
PULSE2D_START_SEESAW_GAMEPAD();
```

Initializes the I2C bus and gamepad hardware. Call once in `PULSE2D_ON_GAMESTART()`.

**Scope:** `PULSE2D_ON_GAMESTART`

**Example:**
```cpp
PULSE2D_ON_GAMESTART() {
    Serial.begin(115200);
    PULSE2D_INIT(0.0f, 0.0f, 10);
    PULSE2D_START_SEESAW_GAMEPAD();
    PULSE2D_SET_SCENE(Main_Menu);
}
```

---

### PULSE2D_POLL_SEESAW_GAMEPAD

```cpp
PULSE2D_POLL_SEESAW_GAMEPAD();
```

Polls all inputs and brings `gamepad_state` into scope for the rest of the scene function. Call at the top of each scene function.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
PULSE2D_ON_GAMESCENE(Game_Level) {
    PULSE2D_TICK_WORLD(Game_Level);
    PULSE2D_POLL_SEESAW_GAMEPAD();

    // gamepad_state is now available
    if (SEESAW_BUTTON_INPUT(SEESAW_A)) {
        fire();
    }
}
```

---

### SEESAW_BUTTON_INPUT

```cpp
SEESAW_BUTTON_INPUT(button_name)
```

Evaluates non-zero while the named button is held. Available button constants:
- `SEESAW_A`
- `SEESAW_B`
- `SEESAW_X`
- `SEESAW_Y`
- `SEESAW_START`
- `SEESAW_SELECT`

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
if (SEESAW_BUTTON_INPUT(SEESAW_A)) { jump(); }
if (SEESAW_BUTTON_INPUT(SEESAW_B)) { shoot(); }
if (SEESAW_BUTTON_INPUT(SEESAW_START)) { pause(); }
```

---

### SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL

```cpp
SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL(body_name, max_speed);
SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL(body_name, max_speed, vertical_only, horizontal_only);
```

**Profile A: The Arcade Controller** (Pokémon, Zelda, Pac-Man)

Sets the body's velocity directly from the stick position for instant response and instant stop. Optional third and fourth boolean arguments enable vertical-only or horizontal-only movement.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL(player, 3.0f);                    // both axes
SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL(player, 3.0f, true, false);       // vertical only
SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL(player, 3.0f, false, true);       // horizontal only
```

---

### SEESAW_SET_ARCADE_DIRECTIONAL_INVERTED_CONTROL

```cpp
SEESAW_SET_ARCADE_DIRECTIONAL_INVERTED_CONTROL(body_name, max_speed);
SEESAW_SET_ARCADE_DIRECTIONAL_INVERTED_CONTROL(body_name, max_speed, vertical_only, horizontal_only);
```

Same as `SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL`, but with inverted axes.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
SEESAW_SET_ARCADE_DIRECTIONAL_INVERTED_CONTROL(ship, 4.0f);
```

---

### SEESAW_SETDYNAMIC_DIRECTIONAL_CONTROL

```cpp
SEESAW_SETDYNAMIC_DIRECTIONAL_CONTROL(body_name, acceleration);
```

**Profile B: The Momentum Controller** (Asteroids, Mario)

Applies a thrust force scaled by `acceleration` each frame. Velocity builds up over time, creating momentum-based movement.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
SEESAW_SETDYNAMIC_DIRECTIONAL_CONTROL(ship, 0.8f);
```

---

### SEESAW_SET_SLIDING_FRICTION_DIRECTIONAL_CONTROL

```cpp
SEESAW_SET_SLIDING_FRICTION_DIRECTIONAL_CONTROL(body_name, drag_amount);
```

**Profile C: Top-Down Friction**

Applies linear drag to the body each frame. Pair with `SEESAW_SETDYNAMIC_DIRECTIONAL_CONTROL` so the body doesn't slide forever.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
SEESAW_SETDYNAMIC_DIRECTIONAL_CONTROL(ship, 0.8f);
SEESAW_SET_SLIDING_FRICTION_DIRECTIONAL_CONTROL(ship, 0.92f);
```

---

### Analog Stick Input

```cpp
float x = SEESAW_DIRECTIONAL_X_INPUT();  // -1.0 to +1.0
float y = SEESAW_DIRECTIONAL_Y_INPUT();  // -1.0 to +1.0
```

Raw analog stick axes, normalized from −1.0 to +1.0.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
auto& ship = PULSE2D_GET_BODY(ship);
ship.set_velocity({
    SEESAW_DIRECTIONAL_X_INPUT() * 5.0f,
    SEESAW_DIRECTIONAL_Y_INPUT() * 5.0f
});
```

---

### Direction Helpers

```cpp
SEESAW_DIRECTION_IS_LEFT()
SEESAW_DIRECTION_IS_RIGHT()
SEESAW_DIRECTION_IS_UP()
SEESAW_DIRECTION_IS_DOWN()
```

Boolean helpers that return true when the stick is pushed more than halfway (> 0.5 magnitude) in the given direction.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
if (SEESAW_DIRECTION_IS_UP() && on_ground) {
    jump();
}

if (SEESAW_DIRECTION_IS_LEFT()) {
    facing_left = true;
}
```

---

## Engine Lifecycle

### PULSE2D_ON_GAMESTART

```cpp
PULSE2D_ON_GAMESTART() {
    // Initialization code
}
```

Maps to Arduino `setup()`. Runs once on power-on. This is where you initialize serial, the engine, the gamepad, and set the initial scene.

**Scope:** `global`

**Example:**
```cpp
PULSE2D_ON_GAMESTART() {
    Serial.begin(115200);
    PULSE2D_POLL_SERIAL_CONNECTION();
    PULSE2D_REGISTER_ETL_ERROR_HANDLER();

    PULSE2D_INIT(0.0f, 0.0f, 10);
    PULSE2D_START_SEESAW_GAMEPAD();
    PULSE2D_SET_SCENE(Main_Menu);
}
```

---

### PULSE2D_ON_GAMELOOP

```cpp
PULSE2D_ON_GAMELOOP() {
    // Per-frame code
}
```

Maps to Arduino `loop()`. Runs every frame (~60 Hz). For most games, this only needs `PULSE2D_TICK_GAMESCENE()`.

**Scope:** `global`

**Example:**
```cpp
PULSE2D_ON_GAMELOOP() {
    PULSE2D_TICK_GAMESCENE();
}
```

---

### PULSE2D_TICK_PULSE

```cpp
PULSE2D_TICK_PULSE();
```

Manually ticks the game engine. You typically don't need this - `PULSE2D_RENDER` calls it automatically.

**Scope:** `PULSE2D_ON_GAMELOOP`

---

## Debug, Utilities

### PULSE2D_PRINT_STACKSIZE

```cpp
PULSE2D_PRINT_STACKSIZE();
```

Prints stack usage to serial every 300 frames (~5 seconds at 60 fps). Compiled away in non-debug builds.

**Scope:** `PULSE2D_ON_GAMESCENE`

**Example:**
```cpp
PULSE2D_ON_GAMESCENE(Game_Level) {
    PULSE2D_TICK_WORLD(Game_Level);
    PULSE2D_PRINT_STACKSIZE();

    PULSE2D_RENDER(active_scene);
}
```

**Output:**
```
stack used: 8192 bytes
stack used: 8256 bytes
```

---

### PULSE2D_REGISTER_ETL_ERROR_HANDLER

```cpp
PULSE2D_REGISTER_ETL_ERROR_HANDLER();
```

Registers a Serial callback for ETL assertion failures. With `-fno-exceptions` (required by Teensyduino), ETL bounds violations are silent by default. This makes them print to serial:

```
[ETL] Error in scene.h:42 with 'map full'
```

Call once in `PULSE2D_ON_GAMESTART()` after `Serial.begin()`. Compiled away in non-debug builds.

**Scope:** `PULSE2D_ON_GAMESTART`

**Example:**
```cpp
PULSE2D_ON_GAMESTART() {
    Serial.begin(115200);
    PULSE2D_POLL_SERIAL_CONNECTION();
    PULSE2D_REGISTER_ETL_ERROR_HANDLER();

    PULSE2D_INIT(0.0f, 0.0f, 10);
}
```

---

## Example

Here's a complete game demonstrating most DSL features:

```cpp
#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS
#include "../include/explosion-anim.h"
#include "../include/stars-bg.h"

PULSE2D_START_PULSE();
PULSE2D_ENABLE_SEESAW_GAMEPAD();

// Single scene with up to 20 bodies and 6 sprites
PULSE2D_DEFINE_SCENE(Space_Shooter, 20, 6);
PULSE2D_GAME_SCENES(Space_Shooter);

PULSE2D_DEFINE int score = 0;
PULSE2D_DEFINE bool enemy_hit = false;

PULSE2D_ON_GAMESCENE_START(Space_Shooter) {
    // Background
    PULSE2D_SPRITE_FLASH(bg_stars, stars_bg, 320, 240);

    // Sprites
    PULSE2D_SPRITE(ship_sprite, "ship.bin", 48, 48);
    PULSE2D_SPRITE(enemy_sprite, "enemy.bin", 48, 48);
    PULSE2D_SPRITE(bullet_sprite, "bullet.bin", 12, 8);

    PULSE2D_ADD_PARALLAX_LAYER(bg_stars, 320.0f, 15.0f);

    // Setup animation
    PULSE2D_DEFINE_VFX(explosion, explosion_frames, 64, 64, 8, 12);

    // Setup ammo pool
    PULSE2D_INIT_POOL(bullet_pool, {
        .width = { 0.15f, 0.08f }
    });

    // Player
    PULSE2D_CONTROLLED_BODY(ship_object, {
        .position = { -4.0f, 0.0f },
        .width = { 0.5f, 0.5f }
    });

    // Enemy
    PULSE2D_DYNAMIC_BODY(enemy_object, {
        .position = { 3.0f, 0.0f },
        .mass = 1.0f,
        .width = { 0.6f, 0.6f }
    });
}

PULSE2D_DEFINE uint32_t cooldown = 0;

PULSE2D_ON_GAMESCENE(Space_Shooter) {
    PULSE2D_TICK_WORLD(Space_Shooter);

    PULSE2D_POLL_SEESAW_GAMEPAD();

    PULSE2D_RENDER_BACKGROUNDS();

    SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL(ship_object, 3.5f);

    auto& ship = PULSE2D_GET_BODY(ship_object);

    // Fire bullets
    if (cooldown > 0)
      cooldown--;
    if (SEESAW_BUTTON_INPUT(SEESAW_A) && cooldown == 0) {
        PULSE2D_SPAWN(bullet_pool,
            100,
            ship.position.x + 0.6f, ship.position.y,
            8.0f, 0.0f
        );
        cooldown = 10;
    }

    // Update live bullets
    PULSE2D_RENDER_POOL(bullet_pool, [&](auto* bullet) {
        auto& enemy = PULSE2D_GET_BODY(enemy_object);

        if (bullet->position.x > 6.0f) {
            PULSE2D_DESPAWN(bullet_pool, bullet);
        } else {
            PULSE2D_DRAW_BODY(bullet, bullet_sprite);
        }

        PULSE2D_ON_COLLISION(bullet, &enemy, [&] {
            if (!enemy_hit) {
                enemy_hit = true;
                auto [sx, sy] = PULSE2D_BODY_COORDINATES((&enemy));
                PULSE2D_PLAY_VFX(explosion, sx, sy);
                score += 100;
            }
            PULSE2D_DESPAWN(bullet_pool, bullet);
        });
    });

    // Reset
    if (SEESAW_BUTTON_INPUT(SEESAW_START)) {
        PULSE2D_SET_SCENE(Space_Shooter);
    }

    // Draw code
    PULSE2D_DRAW(ship_object, ship_sprite);

    if (!enemy_hit) {
        PULSE2D_DRAW(enemy_object, enemy_sprite);
    }

    PULSE2D_TICK_VFX();
    PULSE2D_RENDER(active_scene);
}

PULSE2D_ON_GAMESTART() {
    Serial.begin(115200);

    PULSE2D_REGISTER_ETL_ERROR_HANDLER();
    PULSE2D_INIT(0.0f, 0.0f, 10);
    PULSE2D_START_SEESAW_GAMEPAD();
    PULSE2D_SET_SCENE(Space_Shooter);
}

PULSE2D_ON_GAMELOOP() {
    PULSE2D_TICK_GAMESCENE();
}
```

---

## See Also

- [Physics README](pulse2d/graphics/readme.md) - Physics engine details
- [Blog series](https://soliloq.uy/tag/pulse2d/) - Embedded development blog
