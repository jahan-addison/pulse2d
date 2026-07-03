/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

/****************************************************************************
 * State machine primitives (boost/sml)
 *
 * Building blocks for game entities driven by boost/sml:
 *
 *   Draw_Fn          - non-capturing function pointer for body draw calls.
 *                      Use when a state machine action needs to render a body
 *                      without capturing the game runtime by reference.
 *
 *   Entity_Controller<SM_Def, Data, Config>
 *                    - owns a Data + Config + sml::sm<SM_Def> triple.
 *                      sml injects Data and Config into every action/guard
 *                      that lists them as parameters (dependency injection).
 *                      placement-new reset() lets the SM re-enter its initial
 *                      state without heap allocation; the caller dispatches
 *                      whatever init event their SM requires.
 *
 * Minimal enemy example
 *
 *   // Events
 *   struct Activate_Event {};
 *   struct Hit_Event       {};
 *   struct Render_Event    {};
 *
 *   // States
 *   struct Idle   {};
 *   struct Active {};
 *   struct Dead   {};
 *
 *   // Per-instance mutable state
 *   struct Enemy_Data { p_ui8 hp = 3; };
 *
 *   // Per-instance fixed config
 *   struct Enemy_Config {
 *       pulse2d::graphics::Body* body   = nullptr;
 *       const char*              sprite = nullptr;
 *       pulse2d_state::Draw_Fn   draw   = nullptr;
 *   };
 *
 *   // Transition table
 *   struct enemy_sm {
 *       auto operator()() const {
 *           using namespace sml;
 *
 *           auto will_die  = [](Enemy_Data const& d) { return d.hp <= 1; };
 *           auto on_hit    = [](Enemy_Data& d) { d.hp--; };
 *           auto on_render = [](Enemy_Config const& cfg) {
 *               cfg.draw(cfg.body, cfg.sprite);
 *           };
 *           auto on_death  = [](Enemy_Config const& cfg) {
 *               cfg.body->set_width({ 0.0f, 0.0f });
 *           };
 *
 *           return make_transition_table(
 *               *state<Idle>   + event<Activate_Event>     = state<Active>,
 *                state<Active> + event<Render_Event>                  /
 * on_render, state<Active> + event<Hit_Event>    [ will_die]  / on_hit  =
 * state<Dead>, state<Active> + event<Hit_Event>    [!will_die]  / on_hit,
 *                state<Dead>   + on_entry<_>                      / on_death
 *           );
 *       }
 *   };
 *
 *   // Controller type alias
 *   using Enemy = pulse2d_state::Entity_Controller<enemy_sm, Enemy_Data,
 * Enemy_Config>;
 *
 *   // Usage
 *   PULSE_DEFINE Enemy enemy{};
 *
 *   // scene start:
 *   enemy.configure({ .body = &my_game.get_body("enemy_1"),
 *                     .sprite = "enemy_sprite",
 *                     .draw = [](pulse2d::graphics::Body* b, const char* s) {
 *                         my_game.draw_body(b, s); } });
 *   enemy.dispatch(Activate_Event{});
 *
 *   // per frame:
 *   enemy.dispatch(Render_Event{});
 *   if (enemy.is<Dead>()) { ... }
 *
 *   // respawn:
 *   enemy.reset();            // placement new, returns to Idle
 *   enemy.dispatch(Activate_Event{});
 *
 ****************************************************************************/

#include <new>

#include <boost/sml.hpp>
#include <pulse2d/graphics/body.h>

namespace pulse2d::state {

///////////////////////////////////////////////////////////////////////////////
// Draw_Fn
//
// Non-capturing function pointer for body draw calls inside SM actions.
// Non-capturing lambdas that reference only translation unit globals are
// convertible to this type and can be stored without heap allocation or
// std::function.
//
//   .draw = [](pulse2d::graphics::Body* b, const char* s) {
//       my_game.draw_body(b, s);
//   }
///////////////////////////////////////////////////////////////////////////////

using Draw_Fn = void (*)(pulse2d::graphics::Body*, const char*);

///////////////////////////////////////////////////////////////////////////////
// Entity_Controller
//
// Template parameters:
//   SM_Def  - the struct with operator()() returning the transition table
//   Data    - mutable per-instance runtime state (injected by sml)
//   Config  - immutable per-instance configuration  (injected by sml)
//
// sml injects Data and Config into action/guard lambdas by parameter type,
// so any lambda in the transition table can list them by reference:
//
//   auto my_action = [](Data& d, Config const& cfg) { ... };
//
// reset() tears down the SM via explicit destructor and reconstructs it
// in-place via placement new. The caller dispatches whatever init event
// their SM needs after reset - Entity_Controller does not assume one.
///////////////////////////////////////////////////////////////////////////////

template<typename SM_Def, typename Data, typename Config>
class Entity_Controller
{
    using SM = boost::sml::sm<SM_Def>;

  public:
    Entity_Controller()
        : sm_(data_, cfg_)
    {
    }

    Entity_Controller(Entity_Controller const&) = delete;
    Entity_Controller& operator=(Entity_Controller const&) = delete;

  public:
    void configure(Config const& c) { cfg_ = c; }

    template<typename Event>
    void dispatch(Event const& e)
    {
        sm_.process_event(e);
    }

    template<typename State>
    bool is()
    {
        return sm_.is(boost::sml::state<State>);
    }

    Data const& data() const { return data_; }
    Config const& config() const { return cfg_; }

    // returns the SM to its initial state, caller dispatches the init event
    void reset(Data const& d = {})
    {
        data_ = d;
        sm_.~SM();
        ::new (&sm_) SM(data_, cfg_);
    }

  private:
    Data data_{};
    Config cfg_{};
    SM sm_;
};

} // namespace pulse2d::state
