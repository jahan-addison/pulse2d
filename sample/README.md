# Game Development on Luya

Below are notes on building games with Luya on the Teensy 4.1 — covering the memory layout,
what to watch out for at each stage of development, and practices that keep things
working as the game grows.

The Teensy 4.1 has 512 KB of DTCM (the main RAM where all your data and stack live),
512 KB of OCRAM (a second bank, separate from DTCM, good for large buffers), and
7,936 KB of flash. That sounds like a lot until you realize a single full-screen
RGB565 framebuffer is 150 KB. Memory pressure is the central constraint.

---

## Memory regions at a glance

| Region | Address     | Size   | What goes there |
|--------|-------------|--------|-----------------|
| Flash  | `0x60000000`| 7936 KB| `.text`, read-only data, asset blobs |
| DTCM   | `0x20000000`| 512 KB | `.data`, `.bss`, stack (grows down) |
| OCRAM  | `0x20200000`| 512 KB | DMA buffers, large static arrays (`DMAMEM`) |
| PSRAM  | `0x70000000`| 32 MB  | External SPI PSRAM if soldered (`EXTMEM`) |

The stack lives in DTCM and grows downward. Teensyduino reserves 8 KB for it.
Everything you declare `static` or globally goes into DTCM `.bss` by default,
which is the same pool the stack is cutting into from the other end.

The build script prints all three after every build:

```
  Flash  (text+data):    200 KB /   7936 KB  [........................................]   2%
  DTCM   (data+bss):     198 KB /    512 KB  [###############.........................]  38%
  OCRAM  (dmabuffers):   164 KB /    512 KB  [############............................]  32%
```

**Watch DTCM**. If it climbs _past 80%_ the stack is in serious danger.

---

## Stage 1 — Prototype (host, SDL2)

Develop the whole game on your desktop first. The SDL2 driver opens a window at
the ILI9341 native resolution (320×240) scaled up, so you see exactly what the
Teensy will show.

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug \
      -DUSE_SANITIZER="Address;Undefined" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DSDL2_DIR=$(brew --prefix sdl2)/lib/cmake/SDL2
cmake --build build
./build/sample_game
```

The `Address;Undefined` sanitizers catch out-of-bounds and undefined behaviour
that would silently corrupt state on hardware. Fix everything the sanitizers flag
before moving to Stage 2 — the Teensy has no way to tell you why it crashed.

**Things to keep in mind at this stage:**

- Keep all game objects as `static` globals, not locals or heap. There is no heap
  on the Teensy build and stack frames for large objects will overflow.
- Avoid `std::string`, `std::vector`, or any container that calls `new`. Use
  `etl::vector`, `etl::map`, and `etl::string` with fixed capacity.
- Never use exceptions or RTTI (`-fno-exceptions -fno-rtti` are already set).
  Propagate errors by return value.
- Keep `loop()` / `tick()` allocation-free. No temporaries that call constructors
  or allocate. Every frame should touch the same memory it touched last frame.

---

## Enforcing Teensy constraints during development

The host has a much larger address space and a full heap, so mistakes that would
hard-fault on the Teensy can go completely unnoticed during desktop development.
The sample build has several of these guards enabled by default. They are documented
here so you know what you are getting and can apply the same patterns to your own
game targets.

### Abort on ETL capacity overflow (enabled by default)

ETL containers call a global error handler when they are full instead of silently
doing nothing. `sample_game.cc` registers an aborting handler at the top of
`main()`. Do the same in your game:

```cpp
static void on_etl_error(const etl::exception& e)
{
    std::fprintf(stderr, "ETL error: %s (%s:%d)\n",
        e.what(), e.file_name(), e.line_number());
    std::abort();
}

// first line of main():
etl::error_handler::set_callback<&on_etl_error>();
```

Without this, pushing past capacity on an `etl::vector` is silently ignored —
a mismatch between host behaviour and the hard limit you will hit on the Teensy.

### Warn on large stack frames (enabled by default)

The `sample` CMake target is built with `-Wstack-usage=256 -fstack-usage`. Any
function whose frame exceeds 256 bytes gets a compiler warning. The threshold is
conservative — the Teensy stack is 8 KB total shared across all active call
frames, so individual functions should stay well under that.

`-fstack-usage` also writes a `.su` file next to each object file. Each line reads:

```
sample_game.cc:42:5  main  128  static
```

Look for anything marked `static` (worst-case unbounded) or over a few hundred
bytes. Add the same options to your own target:

```cmake
target_compile_options(my_game PRIVATE -Wstack-usage=256 -fstack-usage)
```

### Budget your data sections with `static_assert` (you add these)

`static_assert` on struct sizes is game-specific — the sample cannot know your
budgets. Write them into your code as you go:

```cpp
static_assert(sizeof(Game_State) < 32 * 1024,
    "Game_State exceeds 32 KB DTCM budget");
static_assert(sizeof(s_framebuffer) + sizeof(Game_State) < 200 * 1024,
    "combined static data is too large");
```

These are free at runtime and make the constraints visible to anyone reading the
code later.

### Summary

| Practice | Status | What it catches |
|----------|--------|------------------|
| ETL error handler aborts | default | Pushing past ETL container capacity |
| `-Wstack-usage=256` | default | Functions with frames unsafe for the 8 KB Teensy stack |
| `-fstack-usage` + `.su` files | default | Per-function frame sizes for pre-flash review |
| `static_assert` on struct sizes | you add | Data section budgets enforced at compile time |

**Deferred construction for hardware objects** — Any object with a non-trivial
constructor that touches hardware (SPI, SD, audio) must not be constructed at
static-init time, before `setup()` runs. Use `luya::HARDWARE_Deferred_Init<T>`:

```cpp
static luya::HARDWARE_Deferred_Init<luya::Engine> engine;

void setup() {
    engine.emplace();   // constructs Engine here, hardware is ready
    engine->init();
}
```

Declare the `HARDWARE_Deferred_Init` as a global — its own constructor is trivial and
does not touch hardware. T's constructor only runs when you call `emplace()`.

---

## Stage 2 — First Teensy build

Once the game runs on the host, build for the Teensy:

```bash
cmake --build build --target teensy
# or directly:
bash cmake/build-teensy.sh
```

Check the memory report. The big things that eat DTCM are:

1. **Large static arrays** — sprite pixel data, tile maps, audio buffers. Move them
   to OCRAM with `LUYA_EXTMEM` (defined in `luya/display.h`):
   ```cpp
   static LUYA_EXTMEM uint16_t my_tilemap[64 * 64];
   ```
   On the host `LUYA_EXTMEM` is a no-op, so the same declaration compiles both ways.

2. **The framebuffer** — already in OCRAM. Do not copy it or make a second one.

3. **ETL containers** — their capacity is baked into their type and size. Declare
   them static so they live in `.bss`, not on the stack.

The `.dmabuffers` section (OCRAM) is initialized to zero by the Teensyduino startup
code, just like `.bss`. You do not need to zero `LUYA_EXTMEM` variables yourself.

---

## Stage 3 — Sprites and assets

Sprites on the Teensy are loaded from the SD card at startup in a raw binary format:
`uint16_t` width, `uint16_t` height, then `width × height` RGB565 pixels. Convert
them on your desktop:

```python
# rough conversion — see sample/images/ for reference
from PIL import Image
import struct, sys

img = Image.open(sys.argv[1]).convert("RGB")
w, h = img.size
with open(sys.argv[2], "wb") as f:
    f.write(struct.pack("<HH", w, h))
    for r, g, b in img.getdata():
        px = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
        f.write(struct.pack("<H", px))
```

**Memory impact of sprites:**

Each `luya::Sprite` struct holds a pointer to pixel data plus width and height.
The pixel data itself lives in the `Storage` internal buffer, which is a fixed-size
array. Size that array to the sum of all your sprite pixel counts:

```cpp
// In Storage — adjust k_sprite_buffer to fit your sprites
static constexpr size_t k_sprite_buffer = 64 * 64 * 4;  // four 64x64 sprites
```

Do not load all sprite frames into memory at once for animations. Load the current
frame, draw it, then load the next. SD card reads are fast enough at 60 Hz for a
few sprites if you keep frame sizes small (under 32×32 works comfortably).

For anything larger — background tiles, large characters — store the raw data in
flash as a `const` array with `PROGMEM` or just as a `static const uint16_t[]`.
Flash reads on the i.MX RT1062 are cached and nearly as fast as DTCM for sequential
access.

---

## Stage 4 — Physics and game objects

The physics engine uses fixed-size ETL containers internally. The world capacity
(max bodies, max arbiters) is set at construction:

```cpp
static luya::physics::World world({ 0.0f, -10.0f }, 16);  // max 16 bodies
```

Pick a capacity and do not exceed it. Bodies themselves are small (about 80 bytes
each), so even 32 bodies only costs ~2.5 KB — that is fine in DTCM.

**Tricks for complex scenes:**

- **Pool your bodies.** Allocate a fixed array of `Body` upfront. "Spawn" an object
  by pulling from the pool and setting its fields; "destroy" it by zeroing the entry
  and removing it from the world. No allocation, no fragmentation.
- **Use world coordinates, not pixel coordinates.** Let the renderer project to
  screen space each frame with `Renderer::project_coordinates()`. Keeping physics
  in metres avoids floating-point precision loss at large pixel counts.
- **Step at a fixed rate.** The sample uses `k_dt = 1.0f / 60.0f` and steps once
  per frame. Do not skip steps or step multiple times per frame unless you have
  spare cycles — the solver is tuned for one step at 60 Hz.

---

## Stage 5 — Audio

Audio runs in its own interrupt on the Teensy via the I2S DMA path. The buffers
live outside DTCM already (Teensyduino places them in OCRAM). Your only job is to
not block inside `loop()` for more than about 2 ms, or the audio buffer will underrun
and you will hear clicks.

Long SD card reads inside `loop()` are the usual culprit. Load all audio clips
at `setup()` time if they fit, or stream from SD using double-buffering in a
background task.

---

## Debugging memory on hardware

**Stack depth** — The sample's `stack_used()` function walks `.bss` looking for
the canary value `0xA5A5A5A5` that Teensyduino writes at startup. Add it and call
it periodically via `Serial.printf`:

```cpp
extern "C" uint32_t _ebss;
extern "C" uint32_t _estack;

uint32_t stack_used()
{
    const uint32_t* p = (const uint32_t*)&_ebss;
    uint32_t count = 0;
    while (*p++ == 0xA5A5A5A5)
        count += 4;
    return (uint32_t)&_estack - (uint32_t)&_ebss - count;
}
```

Wrap it in `#ifdef DEBUG` so it compiles out of release builds.

**Section sizes** — After building, check where large symbols actually landed:

```bash
arm-none-eabi-nm build-teensy/sample_game.elf | sort | grep -v " [tT] "
```

Symbols at `0x20000000–0x2007ffff` are in DTCM. Symbols at `0x20200000+` are in
OCRAM. If something large is in DTCM that should not be, add `LUYA_EXTMEM`.

**Section breakdown:**

```bash
arm-none-eabi-objdump -h build-teensy/sample_game.elf | \
  awk 'NR>4 {printf "%-20s %s bytes\n", $2, $3}'
```

---

## Quick reference

| Situation | Solution |
|-----------|----------|
| Large static buffer eating DTCM | `static LUYA_EXTMEM type name[N]` |
| Sprite pixel data too big | Load one frame at a time from SD |
| `std::vector` / `std::string` | Replace with `etl::vector` / `etl::string` |
| Large const lookup table | `static const type name[] = {...}` — lives in flash |
| Stack overflow (hard fault) | Reduce local array sizes; move statics to global scope |
| Audio clicks | No blocking calls in `loop()`; load clips at `setup()` |
| Game object count growing | Pre-allocate a body pool; recycle entries |
| Host build works, Teensy crashes | Run sanitizers; check for stack locals over ~1 KB |
