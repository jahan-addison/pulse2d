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
