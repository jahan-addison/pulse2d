/*****************************************************************************
 * Copyright (c) Jahan Addison
 *
 * This software is dual-licensed under the Apache License, Version 2.0 or
 * the GNU General Public License, Version 3.0 or later.
 *
 * You may use this work, in part or in whole, under the terms of either
 * license.
 *
 * See the LICENSE.Apache-v2 and LICENSE.GPL-v3 files in the project root
 * for the full text of these licenses.
 ****************************************************************************/

#include <doctest/doctest.h>

#include <luya/display/display.h>
#include <luya/physics/body.h>
#include <luya/physics/world.h>
#include <luya/renderer.h>
#include <luya/sprite.h>

#include <algorithm>
#include <cstdint>

using namespace luya;

struct Mock_Display : public display::Display
{
    display::frame_buffer_t captured{};
    int blit_call_count{ 0 };

    void init() override {}
    void clear(uint16_t) override {}
    void blit(display::frame_buffer_t const* fb, int) override
    {
        captured = *fb;
        ++blit_call_count;
    }
};

struct Renderer_Fixture
{
    Mock_Display display;
    Renderer renderer;

    Renderer_Fixture()
        : renderer(display)
    {
    }
};

TEST_CASE("renderer.cc: Renderer::world_to_screen origin")
{
    Renderer_Fixture f;
    auto s = f.renderer.world_to_screen(0.0f, 0.0f);
    CHECK(s.x == display::config::width / 2);
    CHECK(s.y == display::config::height / 2);
}

TEST_CASE("renderer.cc: Renderer::world_to_screen positive x")
{
    Renderer_Fixture f;
    auto origin = f.renderer.world_to_screen(0.0f, 0.0f);
    auto right = f.renderer.world_to_screen(1.0f, 0.0f);
    CHECK(right.x > origin.x);
    CHECK(right.y == origin.y);
}

TEST_CASE("renderer.cc: Renderer::world_to_screen positive y")
{
    Renderer_Fixture f;
    auto origin = f.renderer.world_to_screen(0.0f, 0.0f);
    auto up = f.renderer.world_to_screen(0.0f, 1.0f);
    CHECK(up.y < origin.y);
    CHECK(up.x == origin.x);
}

TEST_CASE("renderer.cc: Renderer::world_to_screen pixels per unit")
{
    Renderer_Fixture f;
    // k_pixels_per_unit == 30
    auto origin = f.renderer.world_to_screen(0.0f, 0.0f);
    auto s = f.renderer.world_to_screen(1.0f, 0.0f);
    CHECK(s.x - origin.x == 30);
}

TEST_CASE("renderer.cc: Renderer::clear default color")
{
    Renderer_Fixture f;
    f.renderer.clear();
    f.renderer.render();

    bool all_zero = std::all_of(f.display.captured.begin(),
        f.display.captured.end(),
        [](uint16_t px) { return px == 0x0000; });
    CHECK(all_zero);
}

TEST_CASE("renderer.cc: Renderer::clear explicit zero")
{
    Renderer_Fixture f;
    f.renderer.clear(0x0000);
    f.renderer.render();

    bool all_zero = std::all_of(f.display.captured.begin(),
        f.display.captured.end(),
        [](uint16_t px) { return px == 0x0000; });
    CHECK(all_zero);
}

TEST_CASE("renderer.cc: Renderer::clear non-zero color")
{
    Renderer_Fixture f;
    constexpr uint16_t sky_blue = 0x041F;
    f.renderer.clear(sky_blue);
    f.renderer.render();

    bool all_color = std::all_of(f.display.captured.begin(),
        f.display.captured.end(),
        [](uint16_t px) { return px == 0x041F; });
    CHECK(all_color);
}

TEST_CASE("renderer.cc: Renderer::render calls blit")
{
    Renderer_Fixture f;
    f.renderer.render();
    CHECK(f.display.blit_call_count == 1);
}

TEST_CASE("renderer.cc: Renderer::add_sprite null pointer")
{
    Renderer_Fixture f;
    physics::World world({ 0.0f, 0.0f }, 0);

    f.renderer.clear();
    f.renderer.add_sprite(nullptr, 0, 0);
    f.renderer.draw(world); // must not crash

    f.renderer.render();
    CHECK(f.display.blit_call_count == 1);
}

TEST_CASE("renderer.cc: Renderer::add_sprite axis-aligned visible pixel")
{
    Renderer_Fixture f;
    physics::World world({ 0.0f, 0.0f }, 0);

    // 2x2 sprite, top-left pixel is a visible red, rest are transparent
    constexpr uint16_t red = 0xF800;
    constexpr uint16_t transparent = 0xF81F;
    const uint16_t pixels[4] = { red, transparent, transparent, transparent };
    Sprite sprite{ pixels, 2, 2 };

    f.renderer.clear();
    f.renderer.add_sprite(&sprite, 5, 7); // place at screen (5, 7)
    f.renderer.draw(world);
    f.renderer.render();

    // pixel at (5, 7) must equal red
    CHECK(f.display.captured[7 * display::config::width + 5] == red);
}

TEST_CASE("renderer.cc: Renderer::add_sprite axis-aligned transparent pixels")
{
    Renderer_Fixture f;
    physics::World world({ 0.0f, 0.0f }, 0);

    constexpr uint16_t red = 0xF800;
    constexpr uint16_t transparent = 0xF81F;
    const uint16_t pixels[4] = { red, transparent, transparent, transparent };
    Sprite sprite{ pixels, 2, 2 };

    f.renderer.clear(0x0000);
    f.renderer.add_sprite(&sprite, 5, 7);
    f.renderer.draw(world);
    f.renderer.render();

    // pixel at (6, 7) is the transparent texel — framebuffer must stay 0
    CHECK(f.display.captured[7 * display::config::width + 6] == 0x0000);
}

TEST_CASE("renderer.cc: Renderer::draw drains sprite queue")
{
    Renderer_Fixture f;
    physics::World world({ 0.0f, 0.0f }, 0);

    constexpr uint16_t red = 0xF800;
    constexpr uint16_t transparent = 0xF81F;
    const uint16_t pixels[4] = { red, transparent, transparent, transparent };
    Sprite sprite{ pixels, 2, 2 };

    f.renderer.clear();
    f.renderer.add_sprite(&sprite, 5, 7);
    f.renderer.draw(world);

    // second draw with no new sprites – clear to black and re-render
    f.renderer.clear(0x0000);
    f.renderer.draw(world);
    f.renderer.render();

    // the sprite must not appear in this second frame
    CHECK(f.display.captured[7 * display::config::width + 5] == 0x0000);
}

TEST_CASE("renderer.cc: Renderer::show_debug_rects default")
{
    Renderer_Fixture f;
    CHECK(f.renderer.show_debug_rects == false);
}

TEST_CASE("renderer.cc: Renderer::show_debug_rects false")
{
    Renderer_Fixture f;
    physics::World world({ 0.0f, -10.0f }, 10);

    physics::Body box;
    box.set({ 1.0f, 1.0f }, 1.0f); // centered at origin → screen center
    world.add(&box);

    f.renderer.clear(0x0000);
    f.renderer.show_debug_rects = false;
    f.renderer.draw(world);
    f.renderer.render();

    // with debug rects off the framebuffer must remain all black
    bool all_zero = std::all_of(f.display.captured.begin(),
        f.display.captured.end(),
        [](uint16_t px) { return px == 0x0000; });
    CHECK(all_zero);
}

TEST_CASE("renderer.cc: Renderer::show_debug_rects true")
{
    Renderer_Fixture f;
    physics::World world({ 0.0f, -10.0f }, 10);

    physics::Body box;
    box.set({ 2.0f, 2.0f }, 1.0f); // large body at origin → fills screen center
    world.add(&box);

    f.renderer.clear(0x0000);
    f.renderer.show_debug_rects = true;
    f.renderer.draw(world);
    f.renderer.render();

    // the center pixel must have been painted white (0xFFFF)
    const int cx = display::config::width / 2;
    const int cy = display::config::height / 2;
    CHECK(f.display.captured[cy * display::config::width + cx] == 0xFFFF);
}
