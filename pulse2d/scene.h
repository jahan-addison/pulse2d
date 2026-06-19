/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <algorithm>
#include <cmath>
#include <etl/array.h>
#include <etl/error_handler.h>
#include <etl/map.h>
#include <etl/vector.h>
#include <pulse2d/config.h>
#include <pulse2d/graphics/body.h>
#include <pulse2d/graphics/world.h>
#include <pulse2d/renderer.h>
#include <pulse2d/sprite.h>
#include <pulse2d/storage.h>
#include <pulse2d/util.h>
#include <string>

namespace pulse2d {

namespace assets {

struct Background_Layer
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

#if defined(PULSE2D_TEENSY)
struct CStrLess
{
    bool operator()(const char* a, const char* b) const noexcept
    {
        return __builtin_strcmp(a, b) < 0;
    }
};
#endif

struct Sprite_Animator
{
    Sprite_Animator() = default;

    // change states (e.g., from idle to walk)
    void set_state(const pulse2d::assets::Animation_Def* new_def)
    {
        if (def !=
            new_def) { // only reset if the animation is actually changing
            def = new_def;
            accumulator = 0.0f;
            current_frame = 0;
        }
    }

    // advance the loop and update the Sprite every frame
    void tick(pulse2d::Sprite& target_sprite, float dt)
    {
        if (def == nullptr)
            return;

        accumulator += dt;
        if (accumulator >= def->time_per_frame) {
            accumulator -= def->time_per_frame;
            current_frame = (uint16_t)(current_frame + 1) % def->total_frames;
        }

        // Overwrite the target sprite's data pointer with the new frame
        uint32_t pixels_per_frame = def->frame_w * def->frame_h;
        target_sprite.data =
            def->flash_data + (current_frame * pixels_per_frame);
    }

    const assets::Animation_Def* def = nullptr;
    float accumulator = 0.0f;
    uint16_t current_frame = 0;
};

struct Pulse2d_Scene_Animation
{
    void tick_and_draw_animations(pulse2d::Renderer& renderer, float dt)
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

#if defined(PULSE2D_TEENSY)
    etl::map<const char*,
        pulse2d::assets::Animation_Def,
        MAX_ANIMATION_DEFINITION,
        CStrLess>
        anim_defs;
#else
    etl::map<std::string,
        pulse2d::assets::Animation_Def,
        MAX_ANIMATION_DEFINITION>
        anim_defs;
#endif
    etl::vector<pulse2d::assets::Animation_Instance, MAX_ACTIVE_ANIMATION>
        active_animations;
};

class Pulse2d_Scene_Kinematic_Object
{
  public:
    Pulse2d_Scene_Kinematic_Object() = default;

    Pulse2d_Scene_Kinematic_Object(
        const Pulse2d_Scene_Kinematic_Object&) = delete;
    Pulse2d_Scene_Kinematic_Object& operator=(
        const Pulse2d_Scene_Kinematic_Object&) = delete;

    explicit Pulse2d_Scene_Kinematic_Object(
        graphics::detail::Body_Descriptor const& desc)
        : descriptor_(desc)
    {
        descriptor_.mass = 0.0f;
    }

  public:
    void set_descriptor(graphics::detail::Body_Descriptor const& desc)
    {
        descriptor_ = desc;
        descriptor_.mass = 0.0f;
    }

    void deploy(
#if defined(PULSE2D_TEENSY)
        HARDWARE_Deferred_Init<pulse2d::graphics::World>* world,
#else
        pulse2d::graphics::World* world,
#endif
        float x,
        float y,
        float vx,
        float vy)
    {
        auto* obj = memory_.allocate();
        if (obj != nullptr) {
            obj->set(descriptor_);
            obj->position = { x, y };
            obj->velocity = { vx, vy };

#if defined(PULSE2D_TEENSY)
            world->get()->add(obj);
#else
            world->add(obj);
#endif
            active_list_.emplace_back(obj);
        }
    }

    void retract(
#if defined(PULSE2D_TEENSY)
        HARDWARE_Deferred_Init<pulse2d::graphics::World>* world,
#else
        pulse2d::graphics::World* world,
#endif
        pulse2d::graphics::Body* obj)
    {
        auto it = std::ranges::find(active_list_, obj);
        if (it != active_list_.end()) {
            // O(1) swap-and-pop
#if defined(PULSE2D_TEENSY)
            world->get()->remove(obj);
#else
            world->remove(obj);
#endif

            std::iter_swap(it, active_list_.end() - 1);
            active_list_.pop_back();

            memory_.release(obj);
        }
    }

    etl::vector<pulse2d::graphics::Body*, config::max_pooled_objects>&
    active_objects()
    {
        return active_list_;
    }

  private:
    etl::pool<pulse2d::graphics::Body, config::max_pooled_objects> memory_;
    etl::vector<pulse2d::graphics::Body*, config::max_pooled_objects>
        active_list_;
    pulse2d::graphics::detail::Body_Descriptor descriptor_{};
};

struct Pulse2d_Scene_Kinematic_Pool
{
    Pulse2d_Scene_Kinematic_Pool() = default;

#if defined(PULSE2D_TEENSY)
    etl::
        map<const char*, elapsedMillis, MAX_ACTIVE_KINEMATIC_INSTANCE, CStrLess>
            instance_timer;
    etl::map<const char*,
        Pulse2d_Scene_Kinematic_Object,
        MAX_ACTIVE_KINEMATIC_INSTANCE,
        CStrLess>
        instances;
#else
    etl::map<std::string,
        Pulse2d_Scene_Kinematic_Object,
        MAX_ACTIVE_KINEMATIC_INSTANCE>
        instances;
#endif
};

template<std::size_t T_Sprite>
struct Pulse2d_Scene_Background
{

#if defined(PULSE2D_TEENSY)
    using Sprite_Pool = etl::map<const char*, std::size_t, T_Sprite, CStrLess>;
#else
    using Sprite_Pool = etl::map<std::string, std::size_t, T_Sprite>;
#endif

    Pulse2d_Scene_Background() = delete;

    explicit Pulse2d_Scene_Background(Sprite_Pool& sprite_pool,
        etl::array<pulse2d::Sprite, T_Sprite>& sprites)
        : sprite_pool_(sprite_pool)
        , sprites_(sprites)
    {
    }

    void update_and_draw_layers(pulse2d::Renderer& renderer, float delta_time)
    {
        for (auto& layer : background_layers) {
            // Skip any layer whose sprite was not registered (e.g. because
            // T_Sprite in PULSE2D_DEFINE_SCENE is fewer than the number of
            // PULSE2D_ADD_PARALLAX_LAYER calls, or set_from_flash was never
            // called for it). Without this guard, sprite_pool.at() would hit
            // an ETL assertion and hard-fault the board on the first loop tick.
            auto it = sprite_pool_.find(layer.sprite_name);
            if (it == sprite_pool_.end()) {
                PULSE2D_DEBUG_SERIAL(
                    "[WARN] parallax: sprite '%s' not registered, skipping\n",
                    layer.sprite_name);
                continue;
            }

            layer.current_offset += layer.scroll_speed * delta_time;
            layer.current_offset = std::fmod(layer.current_offset, layer.width);

            float draw_x = -layer.current_offset;

            const pulse2d::Sprite& _spr = sprites_.at(it->second);

            renderer.add_sprite(&_spr, static_cast<int16_t>(draw_x), 0);
            renderer.add_sprite(
                &_spr, static_cast<int16_t>(draw_x + layer.width), 0);
        }
    }
    etl::vector<assets::Background_Layer, MAX_BACKGROUND_LAYERS>
        background_layers;

  private:
    Sprite_Pool& sprite_pool_;
    etl::array<pulse2d::Sprite, T_Sprite>& sprites_;
};

template<std::size_t T_Body, std::size_t T_Sprite, std::size_t T_Joint = 0>
struct Pulse2d_Scene_Base
{
    Pulse2d_Scene_Base() = default;

    etl::array<pulse2d::graphics::Body, T_Body> bodies;
    etl::array<pulse2d::graphics::Joint, T_Joint> joints;
    etl::array<pulse2d::Sprite, T_Sprite> sprites;

    std::size_t active_bodies = 0;
    std::size_t active_sprites = 0;

  public:
#if defined(PULSE2D_TEENSY)
    etl::map<const char*, std::size_t, T_Body, CStrLess> body_pool;
    etl::map<const char*, std::size_t, T_Sprite, CStrLess> sprite_pool;
#else
    etl::map<std::string, std::size_t, T_Body> body_pool;
    etl::map<std::string, std::size_t, T_Sprite> sprite_pool;
#endif
    Pulse2d_Scene_Kinematic_Pool pool_manager; // default constructed
    Pulse2d_Scene_Animation animation_manager; // default constructed
    Pulse2d_Scene_Background<T_Sprite> background_manager{ sprite_pool,
        sprites };
};

template<std::size_t T_Body, std::size_t T_Sprite, std::size_t T_Joint = 0>
class Pulse2d_Scene : public Pulse2d_Scene_Base<T_Body, T_Sprite, T_Joint>
{
  public:
    Pulse2d_Scene() = default;

  public:
    static_assert(T_Body <= MAX_PHYSICS_BODIES,
        "T_Body exceeds MAX_PHYSICS_BODIES");
    static_assert(T_Body <= MAX_LOADED_SPRITES,
        "T_Sprite exceeds MAX_LOADED_SPRITES");
    static_assert(T_Joint <= MAX_PHYSICS_JOINTS,
        "T_Joint exceeds MAX_PHYSICS_JOINTS");

  public:
    inline graphics::Body& get_body(const char* name)
    {
        return this->bodies.at(this->body_pool.at(name));
    }

    inline Sprite& get_sprite(const char* name)
    {
        return this->sprites.at(this->sprite_pool.at(name));
    }

  public:
    void set(const char* name, pulse2d::graphics::detail::Body_Descriptor& body)
    {
        if (this->active_bodies >= MAX_PHYSICS_BODIES) {
            PULSE2D_DEBUG_SERIAL(
                "[WARN] body pool full, cannot spawn '%s'\n", name);
            return;
        }
        this->body_pool[name] = this->active_bodies;
        this->bodies.at(this->active_bodies++).set(body);
    }

    void set(const char* name,
        pulse2d::Storage& storage,
        const char* path,
        uint16_t x,
        uint16_t y)
    {
        if (this->active_sprites >= config::max_loaded_sprites) {
            PULSE2D_DEBUG_SERIAL(
                "[WARN] sprite pool full, cannot load '%s'\n", path);
            return;
        }
        this->sprite_pool[name] = this->active_sprites;
        this->sprites.at(this->active_sprites++) =
            storage.load_sprite(path, x, y);
        if (this->sprites[this->active_sprites - 1].data == nullptr) {
            PULSE2D_DEBUG_SERIAL(
                "[WARN] sprite load failed: '%s' -> '%s'\n", name, path);
        } else {
            PULSE2D_DEBUG_SERIAL("sprite '%s' ready: %ux%u\n",
                name,
                this->sprites[this->active_sprites - 1].width,
                this->sprites[this->active_sprites - 1].height);
        }
    }

    void set_from_flash(const char* name,
        uint16_t const* flash_data,
        uint16_t w,
        uint16_t h)
    {
        if (this->active_sprites >= config::max_loaded_sprites) {
            PULSE2D_DEBUG_SERIAL(
                "[WARN] sprite pool full, cannot load flash sprite '%s'\n",
                name);
            return;
        }
        this->sprite_pool[name] = this->active_sprites;

        this->sprites.at(this->active_sprites).data = flash_data;
        this->sprites.at(this->active_sprites).width = w;
        this->sprites.at(this->active_sprites).height = h;

        this->active_sprites++;

        PULSE2D_DEBUG_SERIAL("flash sprite '%s' ready: %ux%u\n", name, w, h);
    }
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
