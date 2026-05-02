## Physics

### Box2D-lite modified for embedded devices

Box2D Lite is a minimal, rigid-body physics engine originally written by Erin Catto as part of his GDC 2006 presentation. It implements sequential impulse-based constraint solving over a fixed timestep with contact caching, making it small enough and well-suited to heavy modification for embedded devices.

Dynamic memory allocation has been removed in favor of fixed-size ETL storage, floating-point types are narrowed to `float` to match the FPv5-D16 hard-float pipeline on the i.MX RT1062.

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

A `Body` that is constructed with no call to `set()` has infinite mass and will never move. Use this for walls, floors, and any fixed platform:

```cpp
physics::Body floor;
floor.position = { 0.0f, -4.0f };  // place it, then add to the world
floor.width    = { 5.0f, 0.5f };   // 5 × 0.5 unit platform
```

To make a body dynamic (i.e. affected by gravity and collisions), call `set()` with full dimensions and a mass in kg. `set()` also zeroes all motion state, so calling it again is safe and reinitializes the body:

```cpp
physics::Body box;
box.set({ 1.0f, 1.0f }, 2.0f);  // 1 × 1 unit box, 2 kg
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

`friction` controls how much a body resists sliding against another. `0.0` is frictionless ice, `1.0` is a rough grip. It defaults to `0.2` (light wood):

```cpp
box.friction = 0.8f;  // sticky surface
```

---

### World

`World` is the simulation context. It holds every body and joint, runs the solver each frame, and records every active contact in `arbiters`.

Construct it with a gravity vector and an iteration count. Gravity is in world units per second squared - use a negative y value to pull things downward. The iteration count is how many passes the solver makes per step: more passes make stacked objects more stable, at higher CPU cost. **10 is a practical default** on both the Teensy and the host.

```cpp
physics::World world({ 0.0f, -10.0f }, 10);
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
1. broad_phase()        find which body pairs overlap this frame
2. integrate forces     apply gravity + add_force() to velocity
3. pre_step()           compute effective masses; warm-start solver
4. apply_impulse() × N  push bodies apart (N = iterations)
5. integrate positions  move bodies by their (corrected) velocity
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
physics::Joint hinge;
hinge.set(&body_a, &body_b, { 0.0f, 1.0f });  // pin at world position (0, 1)
world.add(&hinge);
```

Two parameters tune the feel of the constraint:

| Field | Default | Effect |
|-------|---------|--------|
| `bias_factor` | `0.2` | How aggressively drift is corrected each step (0–1). Higher = stiffer. |
| `softness` | `0.0` | Adds spring-like give. `0.0` = rigid. Increase slightly if the joint feels too stiff. |

```cpp
hinge.bias_factor = 0.3f;  // correct 30% of drift per step (slightly stiffer)
hinge.softness    = 0.01f; // small spring give
```
