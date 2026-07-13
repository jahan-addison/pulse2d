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

### The 32 KB block trap

ITCM and DTCM are not independently sized. They share a single 512 KB RAM1 pool, and the hardware allocates it in **32 KB blocks**. The Teensyduino linker adds their byte counts and checks whether the sum fits within 512 KB - it does not round. The result is a class of silent overflow the linker cannot see.

Say the build produces 230 KB of ITCM code and 276 KB of DTCM data. The sum is 506 KB - well under the 512 KB limit, so the linker reports success and the binary flashes cleanly. The hardware then allocates memory in 32 KB blocks:

- ITCM: ⌈230 / 32⌉ = **8 blocks = 256 KB**
- DTCM: ⌈276 / 32⌉ = **9 blocks = 288 KB**
- Total: **17 blocks = 544 KB > 512 KB**

The hardware silently truncates the overflow. At boot, the startup code tries to zero-initialise `.bss` and copy `.data`, writing into addresses that are no longer mapped. The Teensy hard faults before `Serial.begin()` ever runs. The symptom is indistinguishable from a static initialisation crash: the USB device never enumerates and the serial port never appears.

**The fix**: move large global objects from DTCM to OCRAM. `PULSE_INIT_GAME` places the game Runtime in `PULSE2D_EXTMEM` (OCRAM) for exactly this reason. If you add large scene state or pools and the Teensy stops booting, this is the first thing to check.

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

## The `bss_symbols` tool

Runs `arm-none-eabi-nm` against the compiled ELF and prints every uninitialized variable sorted by size, with color-coded warnings for variables and totals that approach the DTCM limit. It is the fastest way to answer "what is actually eating my `.bss`?"

```bash
make bss_symbols
# or directly:
tools/bss_symbols build-teensy/my_game.elf
tools/bss_symbols build-teensy/my_game.elf --min-bytes 1024   # hide small variables
tools/bss_symbols build-teensy/my_game.elf --top 5            # show only the 5 largest
```

Warning thresholds:

| Scope | Yellow | Red |
|---|---|---|
| Single variable | > 20 KB | > 100 KB |
| Total BSS | > 200 KB | > 240 KB |

**Reading the output**: the address column tells you which region a variable lives in. Variables at `0x200xxxxx` are in DTCM - those are the ones that count toward the 32 KB block budget. Variables at `0x202xxxxx` are in OCRAM (`PULSE2D_EXTMEM`) and do not affect the ITCM/DTCM balance. The tool counts all BSS symbols together, so the total shown in the header will be inflated by the OCRAM objects (`asterisk`, `s_framebuffer`) even when the DTCM footprint is healthy. Focus on the per-symbol list and the addresses, not the total alone.

**Run this whenever the Teensy stops booting after adding new global state.** A variable appearing red in DTCM is the likely culprit. Move it to `PULSE2D_EXTMEM` or reduce its static size.

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

Almost always a DTCM overflow - but `.bss` overflow on Teensy is a **silent killer**. The linker sums ITCM and DTCM in bytes and reports success as long as the total is under 512 KB. The hardware then rounds each region up to the nearest 32 KB block boundary. If the rounded total exceeds 512 KB, the startup code writes into unmapped RAM during `.bss` zero-initialisation, the Teensy hard faults, and the USB device never enumerates. There is no linker error. There is no serial output. The board appears completely dead.

Run `make bss_symbols` and look for large variables at DTCM addresses (`0x200xxxxx`). If any are unexpectedly large, move them to `PULSE2D_EXTMEM` or reduce their static size. Then run `make sections` to confirm the DTCM row is well under the limit. See the 32 KB block trap section above for the exact arithmetic.

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

---

# Hardware Support

## Display

Targets the [PJRC ILI9341 TFT](https://www.pjrc.com/store/display_ili9341_touch.html) on hardware. On host, the same logical resolution is used for the test suite scaled by `pulse2d::config::scale` - no code changes needed between environments.

## Storage

Sprites are loaded via `Storage::load_sprite()`. On host, any format supported by stb_image works. On Teensy, `set_sprite` reads `.bin` files (raw RGB565 pixels with a two-byte width/height header) from the SD card. The `png2bin` tool converts PNGs to this format.

## Audio

Targets the SGTL5000 codec over I2S. Three independent `AudioPlayMemory` channels (looping background music, SFX slot 1, and SFX slot 2) are mixed through a 4-channel `AudioMixer4` before output. Channel 3 is reserved. Both SFX channels play simultaneously without interrupting each other. Audio data must be in the `AudioPlayMemory` format produced by Teensy's `wav2sketch` tool. Call `enable_audio()` once in `PULSE_ON_GAMESTART` after `init()` to start the subsystem (only when an audio shield is attached). The runtime API then exposes `play_music`, `stop_music`, `play_sfx`, `play_sfx2`, `tick_audio`, and `set_volume`. Call `tick_audio()` once per frame to keep looping music going.

## Physics

A port of [box2d-lite](https://github.com/erincatto/box2d-lite) adapted for fixed-size allocation, single-precision float, and a two-stage AABB broad phase. See the [physics readme](pulse2d/graphics/readme.md) for details.

## Renderer

The rendering pipeline on the full-screen RGB565 framebuffer

## Gamepad

Targets the [Adafruit Seesaw Gamepad QT](https://www.adafruit.com/product/5743) over I2C. The DSL (`PULSE_POLL_SEESAW_GAMEPAD`, `SEESAW_BUTTON_INPUT`, etc.) wraps polling and input into single-line calls.
