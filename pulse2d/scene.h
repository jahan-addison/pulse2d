/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <cmath>
#include <etl/array.h>
#include <etl/error_handler.h>
#include <etl/map.h>
#include <etl/vector.h>
#include <pulse2d/graphics/body.h>
#include <pulse2d/graphics/world.h>
#include <pulse2d/renderer.h>
#include <pulse2d/sprite.h>
#include <pulse2d/storage.h>
#include <pulse2d/util.h>
#include <string>

#define MAX_PARALLAX_LAYERS 8

#ifndef MAX_ANIMATION_DEFINITION
#define MAX_ANIMATION_DEFINITION 16
#endif

#ifndef MAX_ACTIVE_ANIMATION
#define MAX_ACTIVE_ANIMATION 32
#endif

namespace pulse2d {

namespace assets {

struct Parallax_Layer
{
    char const* sprite_name;
    float width;
    float scroll_speed;
    float current_offset = 0.0f;
};

struct Animation_Def
{
    uint16_t const* flash_data;
    uint16_t frame_w;
    uint16_t frame_h;
    uint16_t total_frames;
    float time_per_frame; // e.g., 0.066f for 15 FPS
};

struct Animation_Instance
{
    Animation_Def const* def;
    float x;
    float y;
    float accumulator = 0.0f;
    uint16_t current_frame = 0;
};

} // namespace assets

struct Pulse2d_Scene_Base
{
    inline static std::size_t total_bodies = 0;
    inline static std::size_t total_sprites = 0;
};

#if defined(PULSE2D_TEENSY)
struct CStrLess
{
    bool operator()(const char* a, const char* b) const noexcept
    {
        return __builtin_strcmp(a, b) < 0;
    }
};
#endif

template<std::size_t T_Body, std::size_t T_Sprite, std::size_t T_Joint = 0>
struct Pulse2d_Scene : Pulse2d_Scene_Base
{
    static_assert(T_Body <= MAX_PHYSICS_BODIES,
        "T_Body exceeds MAX_PHYSICS_BODIES");
    static_assert(T_Joint <= MAX_PHYSICS_JOINTS,
        "T_Joint exceeds MAX_PHYSICS_JOINTS");

    inline graphics::Body& get_body(const char* name)
    {
        return bodies.at(body_pool.at(name));
    }

    inline Sprite& get_sprite(const char* name)
    {
        return sprites.at(sprite_pool.at(name));
    }

    inline void set(const char* name,
        pulse2d::graphics::detail::Body_Descriptor& body)
    {
        if (total_bodies >= MAX_PHYSICS_BODIES) {
            PULSE2D_DEBUG_SERIAL(
                "[WARN] body pool full, cannot spawn '%s'\n", name);
            return;
        }
        body_pool[name] = active_bodies;
        bodies.at(active_bodies++).set(body);
        ++total_bodies;
    }

    inline void set(const char* name,
        pulse2d::Storage& storage,
        const char* path,
        uint16_t x,
        uint16_t y)
    {
        if (total_sprites >= pulse2d::Storage::k_max_loaded_sprites) {
            PULSE2D_DEBUG_SERIAL(
                "[WARN] sprite pool full, cannot load '%s'\n", path);
            return;
        }
        sprite_pool[name] = active_sprites;
        sprites.at(active_sprites++) = storage.load_sprite(path, x, y);
        ++total_sprites;
        if (sprites[active_sprites - 1].data == nullptr) {
            PULSE2D_DEBUG_SERIAL(
                "[WARN] sprite load failed: '%s' -> '%s'\n", name, path);
        } else {
            PULSE2D_DEBUG_SERIAL("sprite '%s' ready: %ux%u\n",
                name,
                sprites[active_sprites - 1].width,
                sprites[active_sprites - 1].height);
        }
    }

    inline void set_from_flash(const char* name,
        uint16_t const* flash_data,
        uint16_t w,
        uint16_t h)
    {
        if (total_sprites >= pulse2d::Storage::k_max_loaded_sprites ||
            active_sprites >= T_Sprite) {
            PULSE2D_DEBUG_SERIAL(
                "[WARN] sprite pool full, cannot load flash sprite '%s'\n",
                name);
            return;
        }
        sprite_pool[name] = active_sprites;

        sprites.at(active_sprites).data = flash_data;
        sprites.at(active_sprites).width = w;
        sprites.at(active_sprites).height = h;

        ++active_sprites;
        ++total_sprites;

        PULSE2D_DEBUG_SERIAL("flash sprite '%s' ready: %ux%u\n", name, w, h);
    }

    inline void tick_and_draw_animations(pulse2d::Renderer& renderer, float dt)
    {
        auto it = active_animations.begin();

        while (it != active_animations.end()) {
            it->accumulator += dt;
            // advance frame if enough time has passed
            if (it->accumulator >= it->def->time_per_frame) {
                it->accumulator -= it->def->time_per_frame;
                it->current_frame++;
            }

            // check if the animation is finished (Queue is empty)
            if (it->current_frame >= it->def->total_frames) {
                it = active_animations.erase(it);
            } else {
                // draw the current frame
                uint32_t pixels_per_frame = it->def->frame_w * it->def->frame_h;
                const uint16_t* frame_ptr =
                    it->def->flash_data +
                    (it->current_frame * pixels_per_frame);

                // create a temporary Sprite to pass to renderer
                pulse2d::Sprite temp_spr{
                    frame_ptr, it->def->frame_w, it->def->frame_h
                };

                renderer.add_sprite(&temp_spr,
                    static_cast<int16_t>(it->x),
                    static_cast<int16_t>(it->y));

                ++it;
            }
        }
    }

    inline void update_and_draw_parallax(pulse2d::Renderer& renderer,
        float delta_time)
    {
        for (auto& layer : background_layers) {
            // Skip any layer whose sprite was not registered (e.g. because
            // T_Sprite in PULSE2D_DEFINE_SCENE is fewer than the number of
            // PULSE2D_ADD_PARALLAX_LAYER calls, or set_from_flash was never
            // called for it). Without this guard, sprite_pool.at() would hit
            // an ETL assertion and hard-fault the board on the first loop tick.
            auto it = sprite_pool.find(layer.sprite_name);
            if (it == sprite_pool.end()) {
                PULSE2D_DEBUG_SERIAL(
                    "[WARN] parallax: sprite '%s' not registered, skipping\n",
                    layer.sprite_name);
                continue;
            }

            layer.current_offset += layer.scroll_speed * delta_time;
            layer.current_offset = std::fmod(layer.current_offset, layer.width);

            float draw_x = -layer.current_offset;

            const pulse2d::Sprite& _spr = sprites.at(it->second);

            renderer.add_sprite(&_spr, static_cast<int16_t>(draw_x), 0);
            renderer.add_sprite(
                &_spr, static_cast<int16_t>(draw_x + layer.width), 0);
        }
    }

    etl::array<pulse2d::graphics::Body, T_Body> bodies;
    etl::array<pulse2d::graphics::Joint, T_Joint> joints;
    etl::vector<assets::Parallax_Layer, MAX_PARALLAX_LAYERS> background_layers;
    etl::array<pulse2d::Sprite, T_Sprite> sprites;

#if defined(PULSE2D_TEENSY)
    etl::map<const char*, std::size_t, T_Body, CStrLess> body_pool;
    etl::map<const char*, std::size_t, T_Sprite, CStrLess> sprite_pool;
    etl::map<const char*,
        pulse2d::assets::Animation_Def,
        MAX_ANIMATION_DEFINITION,
        CStrLess>
        anim_defs;
#else
    etl::map<std::string, std::size_t, T_Body> body_pool;
    etl::map<std::string, std::size_t, T_Sprite> sprite_pool;
    etl::map<std::string,
        pulse2d::assets::Animation_Def,
        MAX_ANIMATION_DEFINITION>
        anim_defs;
#endif
    etl::vector<pulse2d::assets::Animation_Instance, MAX_ACTIVE_ANIMATION>
        active_animations;

    std::size_t active_bodies{ 0 };
    std::size_t active_sprites{ 0 };
};

template<std::size_t T_Body, std::size_t T_Sprite, std::size_t T_Joint = 0>
using Pulse2d_Level = Pulse2d_Scene<T_Body, T_Sprite, T_Joint>;

#if defined(PULSE2D_TEENSY)
extern "C" uint32_t _ebss;
extern "C" uint32_t _estack;

uint32_t inline stack_used()
{
    const uint32_t* p = (const uint32_t*)&_ebss;
    uint32_t count = 0;
    while (*p++ == 0xA5A5A5A5)
        count += 4;
    return (uint32_t)&_estack - (uint32_t)&_ebss - count; // bytes consumed
}
#endif

} // namespace teensy
