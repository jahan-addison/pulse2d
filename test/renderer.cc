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

#include <luya/display.h>
#include <luya/physics/body.h>
#include <luya/physics/world.h>
#include <luya/renderer.h>
#include <luya/sprite.h>

#include <algorithm>
#include <cstdint>

using namespace luya;

// Helper: read the framebuffer via LUYA_PRIVATE access
static frame_buffer_t const& framebuffer(Renderer const& r)
{
    return *r.framebuffer_;
}

struct Renderer_Fixture
{
    Display display;
    Renderer renderer;

    Renderer_Fixture()
        : renderer(display)
    {
        renderer.init();
    }
};

TEST_CASE("renderer.cc: Renderer::project_coordinates origin")
{
    Renderer_Fixture f;
    auto s = f.renderer.project_coordinates(0.0f, 0.0f);
    CHECK(s.x == config::width / 2);
    CHECK(s.y == config::height / 2);
}

TEST_CASE("renderer.cc: Renderer::project_coordinates positive x")
{
    Renderer_Fixture f;
    auto origin = f.renderer.project_coordinates(0.0f, 0.0f);
    auto right = f.renderer.project_coordinates(1.0f, 0.0f);
    CHECK(right.x > origin.x);
    CHECK(right.y == origin.y);
}

TEST_CASE("renderer.cc: Renderer::project_coordinates positive y")
{
    Renderer_Fixture f;
    auto origin = f.renderer.project_coordinates(0.0f, 0.0f);
    auto up = f.renderer.project_coordinates(0.0f, 1.0f);
    CHECK(up.y < origin.y);
    CHECK(up.x == origin.x);
}

TEST_CASE("renderer.cc: Renderer::project_coordinates pixels per unit")
{
    Renderer_Fixture f;
    // k_pixels_per_unit == 30
    auto origin = f.renderer.project_coordinates(0.0f, 0.0f);
    auto s = f.renderer.project_coordinates(1.0f, 0.0f);
    CHECK(s.x - origin.x == 30);
}

TEST_CASE("renderer.cc: Renderer::clear default color")
{
    Renderer_Fixture f;
    f.renderer.clear();
    f.renderer.render();

    bool all_zero = std::all_of(framebuffer(f.renderer).begin(),
        framebuffer(f.renderer).end(),
        [](uint16_t px) { return px == 0x0000; });
    CHECK(all_zero);
}

TEST_CASE("renderer.cc: Renderer::clear explicit zero")
{
    Renderer_Fixture f;
    f.renderer.clear(0x0000);
    f.renderer.render();

    bool all_zero = std::all_of(framebuffer(f.renderer).begin(),
        framebuffer(f.renderer).end(),
        [](uint16_t px) { return px == 0x0000; });
    CHECK(all_zero);
}

TEST_CASE("renderer.cc: Renderer::clear non-zero color")
{
    Renderer_Fixture f;
    constexpr uint16_t sky_blue = 0x041F;
    f.renderer.clear(sky_blue);
    f.renderer.render();

    bool all_color = std::all_of(framebuffer(f.renderer).begin(),
        framebuffer(f.renderer).end(),
        [](uint16_t px) { return px == 0x041F; });
    CHECK(all_color);
}

TEST_CASE("renderer.cc: Renderer::render calls blit")
{
    Renderer_Fixture f;
    // render() must not crash and framebuffer must be reachable afterwards
    f.renderer.render();
    CHECK(framebuffer(f.renderer).size() ==
          static_cast<std::size_t>(config::width * config::height));
}

TEST_CASE("renderer.cc: Renderer::add_sprite null pointer")
{
    Renderer_Fixture f;
    physics::World world({ 0.0f, 0.0f }, 0);

    f.renderer.clear();
    f.renderer.add_sprite(nullptr, 0, 0);
    f.renderer.draw(world); // must not crash

    f.renderer.render();
    CHECK(framebuffer(f.renderer).size() ==
          static_cast<std::size_t>(config::width * config::height));
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
    CHECK(framebuffer(f.renderer)[7 * config::width + 5] == red);
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
    CHECK(framebuffer(f.renderer)[7 * config::width + 6] == 0x0000);
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
    CHECK(framebuffer(f.renderer)[7 * config::width + 5] == 0x0000);
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
    box.set_mass({ 1.0f, 1.0f }, 1.0f); // centered at origin → screen center
    world.add(&box);

    f.renderer.clear(0x0000);
    f.renderer.show_debug_rects = false;
    f.renderer.draw(world);
    f.renderer.render();

    // with debug rects off the framebuffer must remain all black
    bool all_zero = std::all_of(framebuffer(f.renderer).begin(),
        framebuffer(f.renderer).end(),
        [](uint16_t px) { return px == 0x0000; });
    CHECK(all_zero);
}

TEST_CASE("renderer.cc: Renderer::show_debug_rects true")
{
    Renderer_Fixture f;
    physics::World world({ 0.0f, -10.0f }, 10);

    physics::Body box;
    box.set_mass(
        { 2.0f, 2.0f }, 1.0f); // large body at origin → fills screen center
    world.add(&box);

    f.renderer.clear(0x0000);
    f.renderer.show_debug_rects = true;
    f.renderer.draw(world);
    f.renderer.render();

    // the center pixel must have been painted white (0xFFFF)
    const int cx = config::width / 2;
    const int cy = config::height / 2;
    CHECK(framebuffer(f.renderer)[cy * config::width + cx] == 0xFFFF);
}
