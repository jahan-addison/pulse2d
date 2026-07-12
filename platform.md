# Platform - Teensy 4.1

---

## Memory layout

The i.MX RT1062 has five usable memory regions. The Teensyduino linker script assigns each output section to one of them.

- **ITCM** (512 KB, zero-wait) - code (`.text`)
- **DTCM** (512 KB, zero-wait) - stack, `.data`, `.bss`; this is the region to watch
- **OCRAM** (512 KB, single-cycle) - DMA buffers (`PULSE2D_EXTMEM`)
- **FLASH** (8 MB, cached) - read-only data (`PULSE2D_FLASHMEM`) and overflow code
- **PSRAM** (8 MB, slower) - extended memory

The DTCM budget for `.data` + `.bss` is about 380 KB. Below that, SdFat's FIFO_SDIO DMA needs at least 128 KB of call stack. The linker only errors when a region is fully exhausted - use the `sections` tool to know when you're close.

---

## Static allocation

Pulse2D uses fixed-size containers with `etlcpp`. Every sizing decision is a compile-time constant that sits directly in `.bss`.

### ETL containers

`etl::array<T, N>`, `etl::vector<T, N>`, and `etl::map<K, V, N>` pre-allocate their full capacity at `sizeof(T) * N` bytes, regardless of how many elements are stored. The `N` template parameter directly controls static size - oversized constants silently inflate `.bss`.

```cpp
etl::array<Enemy, 64> enemies{};
```

### `Static_Inplace_T<T>`

Occupies `sizeof(T)` bytes in `.bss` even before `emplace()` is called. The storage is a raw `alignas(T) std::byte[sizeof(T)]` buffer inside the wrapper.

```cpp
static pulse2d::Static_Inplace_T<Pulse2d> engine;
```

### Scene pools

`PULSE_DEFINE_SCENE(name, bodies, sprites)` allocates fixed arrays for physics bodies and sprite slots. Start conservative and expand only when needed.

```cpp
PULSE_DEFINE_SCENE(Level_One, 7, 12);
```

---

## Physics bodies

### Zero-width bodies

Never create a body with `width = {0, 0}`. `set_dynamic_body` calls `Body::Set(width, mass)`, which computes the moment of inertia as `I = mass * (w² + h²) / 12`. With zero width, `I = 0` and `inv_i = 1 / 0` - infinity or NaN depending on the FPU. The physics solver propagates that into every subsequent integration step for that body. Later calling `set_width` on the body does not patch `I` or `inv_i`, so the corruption is permanent for the lifetime of that body.

If you need a body to start inactive, spawn it off-screen with its real dimensions and zero velocity instead:

```cpp
asterisk.set_dynamic_body("enemy",
    {
        .position = { 20.0f, 0.0f }, // off-screen
        .velocity = { 0.0f,  0.0f },
        .width    = pixels_to_units(32.0f, 32.0f), // real width from the start
        .mass = 1.0f,
        .is_sensor = true
});
```

When ready, move it into play and restore its velocity when the game logic activates it.

---

## Keeping assets in flash

Large read-only data (backgrounds, sprite sheets, animation frames) must not be in DTCM. The Teensyduino linker copies `.rodata` into DTCM at boot by default - a single 320×240 RGB565 background is 150 KB, which exhausts the budget immediately.

Use `PULSE2D_FLASHMEM` on any large constant array:

```cpp
PULSE2D_FLASHMEM static constexpr uint16_t nebula_bg[320 * 240] = { ... };
```

`png2header` generates headers with `PULSE2D_FLASHMEM` already applied. Do not remove it.

For DMA-facing buffers that must live in OCRAM:

```cpp
// Placed in .dmabuffers -> OCRAM, not DTCM.
PULSE2D_EXTMEM static uint16_t framebuffer[320 * 240];
```

---

## The `sections` tool

Parses the compiled ELF and prints every allocatable section grouped by memory region, with VMA, LMA, size in bytes, and whether it loads from flash or is zero-initialised. Each region header shows total capacity and bytes used. Regions that exceed their recommended limit are printed in red.

```bash
tools/sections build-teensy/my_game.elf
tools/sections build-teensy/my_game.elf --all          # include sections < 64 bytes
tools/sections build-teensy/my_game.elf --min-bytes 1024
# or via make:
make sections
make sections SECTIONS_ALL=1
```

**Run this after any build that changes pool sizes, adds global state, or introduces large asset headers.** The output makes it immediately visible which ETL containers are unexpectedly large and whether any region is approaching its limit.

<img width="1266" height="854" alt="sections tool output" src="https://github.com/user-attachments/assets/458971e1-0fbf-47d1-9afe-db388e86afc5" />

---

## Stack usage

`pulse2d::stack_used()` returns a runtime byte count of stack consumed since boot. The most useful call site is inside a debug scene tick or on a button press:

```cpp
PULSE_ON_GAMESCENE(Level_One) {
    // ...
    pulse_print_stacksize(); // prints every 300 frames in DEBUG builds
}
```

`pulse_print_stacksize()` compiles away in non-debug builds. Call `stack_used()` directly if you need it unconditionally.

---

## Serial debug

```cpp
PULSE_ON_GAMESTART() {
    Serial.begin(115200);
    // ...
}
```

To block until a host connects before continuing:

```cpp
PULSE_POLL_SERIAL_CONNECTION();
```

`PULSE2D_DEBUG_SERIAL(fmt, ...)` wraps `Serial.printf` and is gated on the `DEBUG` build flag - compiles to nothing in release. `PULSE2D_ERROR_SERIAL(fmt, ...)` is always-on; use it for conditions that are always bugs (null body, missing sprite), not for gameplay events.

---

## Boot failures

### Serial output never appears

Almost always a DTCM overflow: the stack has collided with `.bss` before `Serial.begin()` returns. Run `make sections` and look at the DTCM row. If `.data` + `.bss` is close to 380 KB or in red, reduce pool sizes or move large constants to `PULSE2D_FLASHMEM`.

### Crash or hang after a few frames

SdFat's FIFO_SDIO DMA needs uninterrupted stack depth during SD reads. If DTCM is over budget the DMA call stack has nowhere to go. Symptom: the game starts, renders one or two frames, then freezes. Run `sections` and check DTCM headroom.

### `etl::error_handler` fires

Register the ETL error handler in `PULSE_ON_GAMESTART`:

```cpp
PULSE_ON_GAMESTART() {
    Serial.begin(115200);
    pulse_register_etl_error_handler();
    // ...
}
```

ETL fires on out-of-bounds access, full-container insertions, and similar violations. The handler prints the file name, line number, and error string to serial. Without it, a violation silently corrupts state.

Below are the most common triggers:

- Insufficient scene sizing: If you spawn 3 bodies or 3 loaded sprites, but your scene is sized as `PULSE_DEFINE_SCENE(Level_One, 2, 2)`, the game will crash. Enable `PULSE_POLL_SERIAL_CONNECTION()` that provides helpful body and sprite spawn count details to ensure the number is correct.
- Cached pointer across a scene transition: `PULSE_SET_SCENE` calls `emplace<scene>()`, which destroys the old scene and default-constructs a fresh one. Any `pulse2d_body*` held from the previous scene is immediately a dangling pointer. Use `PULSE_DEFER_SCENE` from tick callbacks so the transition fires after the full tick completes, and never store a body pointer that outlives the scene that owns it.
- Access before registration: Bodies and sprites are not registered until `PULSE_ON_GAMESCENE_START` runs. A name lookup that fires before registration (for example, in a constructor or a tick that somehow executes before start) is an OOB access against an empty map.

---

## The `ldscript` tool

Parses `imxrt1062_t41.ld` from the active Teensyduino install and prints the MEMORY region table and the output section -> region map. The fastest way to answer "which attribute keeps this array in flash?" without reading the raw linker script.

```bash
tools/ldscript
tools/ldscript --ld /path/to/imxrt1062_t41.ld
# or via make:
make ldscript
```

![ldscript tool output](/images/tool-ldscript.png)

---

## Build flags

- `DEBUG` - enables `PULSE2D_DEBUG_SERIAL`, `pulse_print_stacksize`, and ETL error handler registration
- `USE_SANITIZER` - host-only; passes `-fsanitize=...` to the CMake build
- `SECTIONS_ALL=1` - include sections under 64 bytes in `make sections` output
- `GAME_INC` - extra `-I` paths appended to the compiler include search
