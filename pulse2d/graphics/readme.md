# Physics Driver

## Box2D-lite: modified for embedded devices

Box2D-lite is a minimal rigid-body physics engine originally written by Erin Catto as part of his GDC 2006 presentation. It implements sequential impulse-based constraint solving over a fixed timestep with contact caching, well-suited for modification on embedded devices.

### Modifications

- **No heap allocation** - `std::vector`, `std::map`, and native arrays replaced with `etl::vector`, `etl::map`, and `etl::array`. Pool sizes are set at compile time via constants in `pulse2d/config.h`. The key ones for the physics system are:
  - `MAX_PHYSICS_BODIES` (64) - capacity of the bodies vector
  - `MAX_PHYSICS_JOINTS` (32) - capacity of the joints vector
  - `MAX_ARBITER_PAIRS` (64) - capacity of the arbiter map; separate from body count. Worst-case is n*(n-1)/2 pairs; in practice a small fraction are touching at once
  - `MAX_CONTACT_POINTS` (2) - contacts per arbiter. `collide()` returns at most 2: the clip pipeline writes into `ClipVertex[2]` and iterates `for i < 2`
- **`float`** - all types narrowed from `double` to `float` for the FPv5-D16 hard-float unit.
- **Full-dimension `width`** - original `Body` stored half-extents in `h`. This fork stores full width and height in `width`. `collide.cc` computes half-extents internally as `h = 0.5 * width`.
- **`Body::set_motion()`** - original `Body::Set(Vec2, float)` renamed to `set_motion()`. A zero-argument overload computes `inv_mass`, `I`, and `inv_i` from the current `mass` and `width` without resetting position or velocity.
- **Body descriptor pattern** - `Body_Descriptor` struct with designated initializers; `Body::set(Body_Descriptor)` applies it field by field. Used by the Core API (`my_game.set_static_body()`, `my_game.set_dynamic_body()`, etc.).
- **`World::remove(Body*)`** - removes a single body and erases all arbiter entries referencing it. Without the cleanup, stale contacts accumulate until the map fills. Used by kinematic pools to despawn projectiles.
- **`Body::is_sensor`** - non-solid trigger zone. Overlaps are recorded in `world.arbiters` but no impulse is applied. Sensor pairs are skipped in `pre_step()` to avoid a divide-by-zero when both bodies have `inv_mass == 0`. See [Sensors](#sensors).
- **Two-stage broad phase** - the original `broad_phase()` ran the full SAT on every pair. The replacement partitions into pure-static and active sets, pre-computes AABB half-extents in O(n), and rejects non-adjacent pairs before SAT. See [Broad phase](#broad-phase).

---

### Bodies

A `Body` is a solid rectangle, the only shape the engine supports. You describe its size with `width`, which is the full width and height of the box.

```
width = { 1.0, 1.0 }      width = { 4.0, 0.5 }
+----------+               +------------------+
|          |               |                  |
|  1 × 1   |               |    4 × 0.5       |
|          |               +------------------+
+----------+
```

A `Body` that is constructed with no call to `set_motion` has infinite mass and will never move. Use this for walls, floors, and any fixed platform:

```cpp
graphics::Body floor;
floor.position = { 0.0f, -4.0f };  // place it, then add to the world
floor.width    = { 5.0f, 0.5f };   // 5 × 0.5 unit platform
```

To make a body respond to gravity and collisions, call `set_motion` with full dimensions and a mass in kg. `set_motion` also zeroes all motion state, so calling it again is safe and reinitializes the body:

```cpp
graphics::Body box;
box.set_motion({ 1.0f, 1.0f }, 2.0f);  // 1 × 1 unit box, 2 kg
box.position = { 0.0f, 3.0f };  // place it above the floor
```

To push a body, use `add_force()`. Forces are cleared at the end of each `step()`, so you only need to call it in frames where you want the push to apply:

```cpp
// push the box right for one frame
box.add_force({ 5.0f, 0.0f });
```

If you want a continuous effect (like a thruster), call `add_force()` every frame inside your game loop. To set velocity directly instead of pushing:

```cpp
box.velocity = { 3.0f, 0.0f };  // moves 3 units/second to the right
```

`friction` controls how much a body resists sliding against another. `0.0` is frictionless ice, `1.0` is a rough grip. It defaults to `0.2`:

```cpp
box.friction = 0.8f;  // sticky surface
```

---

### World

`World` is the simulation context: it holds every body and joint, runs the solver each frame, and records every active contact in `arbiters`.

Construct it with a gravity vector and an iteration count. Gravity is in world units per second squared. Use a negative y value to pull things downward. The iteration count is how many passes the solver makes per step - more passes make stacked objects more stable, at higher CPU cost. 10 is a good default on both the Teensy and the host.

```cpp
graphics::World world({ 0.0f, -10.0f }, 10);
//                         ^gravity       ^iterations
```

Register every body and joint with `add()`, then call `step()` once per frame with a fixed timestep. A fixed timestep (e.g. `1/60`) is important - a variable `dt` can cause objects to tunnel through thin walls at low frame rates:

```cpp
world.add(&floor);
world.add(&box);

// inside your game loop:
world.step(1.0f / 60.0f);
```

`step()` runs five stages in order:

```
1. broad_phase()        AABB prefilter then SAT; builds the arbiter map
2. integrate forces     apply gravity + add_force() to velocity (dynamic bodies only)
3. pre_step()           compute effective masses; warm-start solver
4. apply_impulse() × N  push bodies apart (N = iterations)
5. integrate positions  move bodies by their (corrected) velocity (dynamic bodies only)
```

After `step()` returns, forces and torques are cleared and positions are up to date. Checking `world.arbiters` tells you which pairs are currently in contact:

```cpp
if (!world.arbiters.empty()) {
    // at least one pair of bodies is touching this frame
}

// iterate over every active contact pair:
for (auto& [key, arb] : world.arbiters) {
    // key.body1 and key.body2 are the two touching bodies
    // arb.num_contacts is 1 or 2
    // arb.contacts[i].normal is the direction of the contact
}
```

Call `clear()` when changing scenes. It removes all bodies, joints, and contact records without deallocating any storage:

```cpp
world.clear();
```

---

### Joints

A `Joint` pins two bodies together at a point in the world so they cannot drift apart. The engine enforces the constraint by applying a small corrective push each iteration.

Call `set()` with both bodies and the anchor point in world space, then add the joint to the world. The joint does not need any per-frame calls - `step()` handles it automatically:

```cpp
graphics::Joint hinge;
hinge.set(&body_a, &body_b, { 0.0f, 1.0f });  // pin at world position (0, 1)
world.add(&hinge);
```

Two parameters control the feel of the constraint:

- `bias_factor`: How aggressively drift is corrected each step (0–1) - higher = stiffer
- `softness`: Adds spring-like give, `0.0` = rigid - increase slightly if the joint feels too stiff

```cpp
hinge.bias_factor = 0.3f;  // correct 30% of drift per step (slightly stiffer)
hinge.softness    = 0.01f; // small spring give
```

---

### Broad phase

`broad_phase()` is the first stage of `step()`. It decides which body pairs are close enough to warrant the full SAT, then updates the arbiter map.

The original box2d-lite broad phase ran `collide()` on every pair - ~150 float ops each. The replacement uses two stages:

**Stage 1 - partition + AABB prefilter**

Bodies are split into two sets:

- *Pure-static*: `inv_mass == 0` and not a sensor - walls and floors
- *Active*: everything else - dynamic bodies and sensors

Static×static pairs are never iterated. The remaining pairs are:

```
Active × Active   (dynamic/sensor vs dynamic/sensor)
Active × Static   (dynamic/sensor vs pure-static walls and floors)
```

Per-body AABB half-extents are pre-computed once - N scalar ops. Static bodies have `rotation == 0` and hit the fast path. Rotated bodies expand their OBB into a conservative AABB:

```
half_extent_x = hx|cosθ| + hy|sinθ|
half_extent_y = hx|sinθ| + hy|cosθ|
```

No false negatives; false positives fall through to the SAT.

**Stage 2 - narrow phase (SAT)**

Only pairs that pass the AABB test reach `collide()`. For a typical scene (4 walls, ~5 dynamic bodies, ~8 projectiles) most pairs are rejected in stage 1.

If `collide()` finds contacts, the pair's `Arbiter` is inserted or updated with warm-start data from the previous frame. If no contacts, the `Arbiter` is erased.

---

### Sensors

A sensor is a non-solid body that detects overlaps without applying any pushback. Other bodies pass through it freely, but the overlap is still recorded in `world.arbiters` for the duration of the contact - so you can react to it with game logic.

Set `is_sensor = true` on a `Body_Descriptor` when creating the body:

```cpp
my_game.set_static_body("pickup_zone", {
    .position  = { 2.0f, 0.0f },
    .width     = { 1.0f, 1.0f },
    .is_sensor = true
});
```

Or assign it directly on an existing body before adding it to the world:

```cpp
graphics::Body zone;
zone.position  = { 2.0f, 0.0f };
zone.width     = { 1.0f, 1.0f };
zone.is_sensor = true;
world.add(&zone);
```

While a body overlaps the sensor, `world.arbiters` contains an entry for that pair - the same map used for physical collisions. Use `my_game.on_collision_with` to respond:

```cpp
my_game.on_collision_with("pickup_zone", [&]() {
    if (!item_collected) {
        item_collected = true;
        score += 50;
    }
});
```

Sensors correctly detect overlaps with static bodies - they sit in the active partition and are tested against every pure-static body, so a sensor on a wall or floor still fires.

Sensors are useful anywhere you need a spatial trigger with no physics effect:

- **Pickup zones** - coins, health packs, powerups the player walks through
- **Damage zones** - lava floors, spike pits, enemy auras that hurt on contact
- **Level triggers** - invisible boundary that fires when the player crosses it
- **Detection radii** - enemy aggro range, line-of-sight entry point

The key distinction from a regular static body: a static body pushes other bodies away when they overlap. A sensor lets them pass through, and only tells you that the overlap happened.