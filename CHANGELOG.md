## [1.1.4] - 2026-07-12

- fix(renderer): k_glcd_cell_h  -> 8 for full descender rows; fix draw_text_centered per-char advance truncation

- chore: v1.1.4
- chore: update CHANGELOG.md for v1.1.3

- Merge pull request #5 from jahan-addison/chore/changelog-v1.1.3
## [1.1.3] - 2026-07-12

- feat: draw_text_centered, bss_symbols tool, EXTMEM game runtime object, 32KB block trap docs

- chore: v1.1.3
- chore: update CHANGELOG.md for v1.1.2

- Merge pull request #4 from jahan-addison/chore/changelog-v1.1.2
## [1.1.2] - 2026-07-12

- fix(audio): move AudioStream objects to file scope, add enable_audio runtime init; px_to_units -> pixels_to_units

- chore: v1.1.2
- chore: update CHANGELOG.md for v1.1.1

- Merge pull request #3 from jahan-addison/chore/changelog-v1.1.1
## [1.1.1] - 2026-07-11

- feat(audio): looping music and dual SFX channels via SGTL5000 and AudioMixer4
- feat(renderer): text writing with glcdfont, Color enum, float size scale
- feat(api): add draw_sprite, standardize PULSE_SCENE_FN concept to pulse2d::Scene

- fix(dsl): PULSE_DEFER_SCENE, rename units_to_pixels, update scene transition docs

- chore: v1.1.1
- chore: documentation clean up
- chore: update CHANGELOG.md for v1.1.0

- refactor: HARDWARE_Deferred_Init -> Static_Inplace_T
- Merge pull request #2 from jahan-addison/chore/changelog-v1.1.0
## [1.1.0] - 2026-07-03

- feat: embedded sprite pool, scene architecture, state machines and state types, additional API primitives, documentation
- feat: add Entropy/TRNG support, body field setters, trng_random docs, and GAME_INC to Makefile.teensy
- feat: add px_to_units, overload pattern, body debug serial, and sections limit warnings
- feat: add set_sprite(name, path) overload for .bin files, since dimensions come from the file header

- chore: v1.1.0 🚀
- chore: update CHANGELOG.md for v1.0.1

- refactor: perfect forwarding on action callbacks, trng improvements
- ci: improvements to release workflow
- Merge pull request #1 from jahan-addison/chore/changelog-v1.0.1
## [1.0.1] - 2026-06-28

- feat: changelog generation, changelog.md
- feat: AABB debug outlines, set_controlled_body auto-mass and gamepad changes for true static wall collision, stronger Runtime type constraints

- fix: api reference updates

- chore: bump version
- chore: fix physics engine memory bloat, stale arbiters, sensor division-by-zero, static body over-integration, kinematic body misclassification, animation dangling sprite pointer, sprite bounds check typo, null sprite renderer guard, and silent SD load failures; replace the O(n^2) no-prefilter broad phase with a two-stage AABB prefilter and static/active partition; introduce MAX_ARBITER_PAIRS and is_kinematic as first-class engine concepts; add always-on PULSE2D_ERROR_SERIAL and the imghelper bin validation tool
- chore: sections tool ITCM region, accurate regional totals, capacity vs in-use header
- chore: improve runtime type safety and animation documentation
- chore: update readme.md
- chore: seesaw gamepad macro + gamepad_hal HAL refactor, tick_vfx, PULSE_TICK_GAMESCENE renames, p_ui* alias shorteninwqg, directional control @brief docs, PULSE_POLL_SERIAL_CONNECTION

- ci: fix release workflow
- docs: update documentation
## [1.0.0] - 2026-06-21

- chore: update readme
- chore: update readme
- chore: tick vfx API, collision improvements, docs

- docs: api reference updates
- docs: api reference updates
## [1.0.0-beta.2] - 2026-06-20

- feat: Core API Runtime + Internal DSL two-layer split, api.md reference, other improvements
## [1.0.0-beta.1] - 2026-06-20

- feat: type aliases and math namespace + helpers, DSL and documentation updates
- feat: sensor non-solid trigger zone physics bodies
- feat: animation2header python script, improvements and documentation
- feat(dsl): PULSE2D_BODY_COORDINATES, documentation updates
- feat: PULSE2D_DEFER_SCENE, DSL documentation updates

- chore: 1.0.0-beta.1
- chore: collision macro renames and documentation, physics sensor updates
- chore: improved seesaw gamepad control macros
- chore: distinguish VFX animation from persistant; DSL documentation
## [0.2.1] - 2026-06-14

- feat: Kinematic pool instance and timer management, PULSE2D_RENDER_POOL

- chore: kinematic pool improvements and macro updates
## [0.2.0] - 2026-06-13

- feat: scene management rewrite, DSL improvements and reference, readme reorganization
- feat: single-direction gamepad movement, scene refactor, animation management

- fix: shebang and license in python scripts

- chore: update readme

- refactor: ETL_VERBOSE_ERRORS; DSL, movement physics improvements
- refactor(dsl): PULSE2D_ADD_SCROLLING_LAYER -> PULSE2D_ADD_PARALLAX_LAYER, PULSE2D_ADD_BACKGROUND_LAYER with docs
## [0.1.2] - 2026-05-30

- feat: background image and parallax features, DSL, memory section tools

- chore: bump version
- chore: documentation updates

- refactor: header includes clean up for both targets
## [0.1.0] - 2026-05-24

- feat: I2C seesaw gamepad and teensy shift game, documentation
- feat: I2C seesaw gamepad and teensy shift game, documentation
- feat: game development DSL and documentation
- feat(graphics): body, world designated initializers and descriptor pattern
- feat: game development DSL initial code
- feat: pulse2d, physics -> graphics, teensy makefile cross-compilation with documentation
- feat: libluya, Teensyduino build system, sample game Teensy entry point
- feat: add control module, renderer test suite
- feat(renderer): sprite rotation, chroma-key transparency, debug rect flag, sample image sync
- feat(physics): documentation, intuitiveness, modernization of box2d-lite, and test suite
- feat: documentation, formatting, renderer and sprite loader, test sample game
- feat: add minimal, ETL-compatible physics engine
- feat: ILI9341, SDL2 cross-compilation targets; display, audio, storage component definitions

- chore: bump version
- chore: update readme
- chore: namespace, license, and code formatting clean up
- chore: ILI9341 touch CS bus collision, gamedev utilities, compiletime checks
- chore: display::make -> display::factory; leave controller interface to game developer
- chore: readme updates, renderer documentation
- chore: readme updates
- chore: add the ETLCPP embedded template library, SDL2 cross-compile improvements

- refactor: common.h -> util.h, Deferred_Init<T> -> HARDWARE_Deferred_Init<T>
- refactor: tighten display driver, hardware Deferred_Init<T>, sample_game-teensy, common.h
- refactor: aggregate physics Body type, fireball sample game improvements
- refactor: root include/ header separation for readability
- Initial commit
