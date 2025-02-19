#include "scene_in_game.h"
#include <settings.h>

#include <core/ftime.h>
#include <core/event.h>
#include <core/fmath.h>
#include <core/fmemory.h>

#include "game/resource.h"
#include "game/game_manager.h"
#include "game/user_interface.h"

typedef struct scene_in_game_state {
  player_state* player;
  panel skill_up_panels[MAX_UPDATE_ABILITY_PANEL_COUNT];
  
  bool has_game_started;
} scene_in_game_state;

static scene_in_game_state *state;


#define STATE_ASSERT(FUNCTION) if (!state) {                                                          \
    TraceLog(LOG_ERROR, "scene_in_game::" FUNCTION "::In game state was not initialized");            \
    event_fire(EVENT_CODE_SCENE_MAIN_MENU, (event_context){});                                        \
    return;                                                                                           \
  }

void in_game_update_bindings();
void in_game_update_mouse_bindings();
void in_game_update_keyboard_bindings();
void start_game();
u16 get_random(u16 min, u16 max);

bool initialize_scene_in_game(camera_metrics* _camera_metrics) {

  state = (scene_in_game_state *)allocate_memory_linear(sizeof(scene_in_game_state), true);

  // Game
  if (!game_manager_initialize(_camera_metrics)) { // Inits player & spawns
    TraceLog(LOG_ERROR, "game_manager_initialize() failed");
    return false;
  }

  state->player = get_player_state_if_available();

  _set_player_position(*get_resolution_div2());

  user_interface_system_initialize();

  for (u32 i = 0; i < 360; i += 20) {
    Vector2 position = get_a_point_of_a_circle(_get_player_position(false), 500, i);
    Texture2D *tex = get_texture_by_enum(TEX_ID_ENEMY_TEXTURE);
    rectangle_collision rect_col = (rectangle_collision) {
      .rect = (Rectangle) {
        .x = position.x,
        .y = position.y,
        .width = tex->width,
        .height = tex->height
      },
      .owner_type = ENEMY
    };
    rect_col.owner_id = _spawn_character((Character2D){
        .character_id = 0,
        .tex = tex,
        .initialized = false,
        .collision = rect_col.rect,
        .position = position,
        .w_direction = WORLD_DIRECTION_LEFT,
        .type = ENEMY,
        .rotation = 0,
        .health = 100,
        .damage = 10,
        .speed = 1,
    });
    add_collision(rect_col);
  }

  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, (event_context){
    .data.f32[0] = PRG_BAR_ID_PLAYER_EXPERIANCE,
    .data.f32[1] = get_player_state_if_available()->exp_perc,
  });
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, (event_context){
    .data.f32[0] = PRG_BAR_ID_PLAYER_HEALTH,
    .data.f32[1] = get_player_state_if_available()->health_perc,
  });  
  event_fire(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, (event_context){
    .data.f32[0] = get_player_state_if_available()->position.x,
    .data.f32[1] = get_player_state_if_available()->position.y,
  });
  
  panel default_panel = (panel) {
    .signal_state  = BTN_STATE_RELEASED,
    .bg_tex_id     = TEX_ID_CRIMSON_FANTASY_PANEL_BG,
    .frame_tex_id  = TEX_ID_CRIMSON_FANTASY_PANEL,
    .bg_tint       = (Color) {255, 255, 255, 200},
    .bg_hover_tint = (Color) {255, 55, 55, 184},
    .offsets = (Vector4) {6, 6, 6, 6},
  };
  for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
    state->skill_up_panels[i] = default_panel;
  }

  set_is_game_paused(true);

  return true;
}

void update_scene_in_game() {
  STATE_ASSERT("update_scene_in_game")

  in_game_update_bindings();
  update_user_interface();

  if (get_is_game_paused() || !state->has_game_started) {
    return;
  }
  update_game_manager();
  update_time();

  event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, (event_context){
    .data.f32[0] = _get_player_position(false).x,
    .data.f32[1] = _get_player_position(false).y,
  });
}

void render_scene_in_game() {
  STATE_ASSERT("render_scene_in_game")

  render_game();
}

void render_interface_in_game() {
  STATE_ASSERT("render_interface_in_game")
  DrawFPS(get_screen_offset().x, get_resolution_div2()->y);

  if (!state->has_game_started) {
    gui_label("Press Space to Start!", (Vector2) {get_resolution_div2()->x, get_resolution_3div2()->y}, WHITE);
    return;
  }

  if (state->player->is_player_have_skill_points) {
    set_is_game_paused(true);
    Rectangle dest = (Rectangle) { // TODO: Make it responsive
      get_resolution_div4()->x, get_resolution_div2()->y, 
      get_resolution_div4()->x, get_resolution_3div2()->y 
    };
    f32 dest_x_buffer = dest.x;
    for (int i=0; i<MAX_UPDATE_ABILITY_PANEL_COUNT; ++i) {
      panel* pnl = &state->skill_up_panels[i];
      if(pnl->buffer[0].data.u16[0] <= 0 || pnl->buffer[0].data.u16[0] >= ABILITY_TYPE_MAX) {
        pnl->buffer[0].data.u16[0] = get_random(1,ABILITY_TYPE_MAX-1);
      }
      dest.x = dest_x_buffer + ((dest.width + get_screen_offset().x) * i);
      if(gui_panel_active(pnl, dest, true)) {
        set_is_game_paused(false);
        state->player->is_player_have_skill_points = false;
      }
      switch (pnl->buffer[0].data.u16[0]) {
        case ABILITY_TYPE_RADIATION: {
          //gui_draw_texture_id(0, (Rectangle) {0});
          gui_label("Radiation", VECTOR2(dest.x, dest.y), WHITE);
          break;
        }
        case ABILITY_TYPE_FIREBALL: {
          gui_label("Fireball", VECTOR2(dest.x, dest.y), WHITE);
          
          break;
        }
        case ABILITY_TYPE_COMET: {
          gui_label("Comet", VECTOR2(dest.x, dest.y), WHITE);
          
          break;
        }
        case ABILITY_TYPE_BULLET: {
          gui_label("Bullet", VECTOR2(dest.x, dest.y), WHITE);
          
          break;
        }
        default: {
          TraceLog(LOG_WARNING, "scene_in_game::render_interface_in_game()::Update panel ability is trying to render an unknown ability: %d", pnl->buffer[0].data.u16[0]);
          break;
        }
      }
    }
  }
  else {
    gui_progress_bar(PRG_BAR_ID_PLAYER_EXPERIANCE, (Vector2){.x = get_resolution_div2()->x, .y = get_screen_offset().x}, true);
    gui_progress_bar(PRG_BAR_ID_PLAYER_HEALTH, get_screen_offset(), false);
  }

  render_user_interface();
}

void start_game() {
  _upgrade_ability(&state->player->ability_system.abilities[state->player->starter_ability]);
}


void in_game_update_bindings() {
  in_game_update_mouse_bindings();
  in_game_update_keyboard_bindings();
}

void in_game_update_mouse_bindings() { 

}
void in_game_update_keyboard_bindings() {

  if (!state->has_game_started && IsKeyPressed(KEY_SPACE)) {
    start_game();
    state->has_game_started = true;
    set_is_game_paused(false);
    return;
  }

  if (IsKeyReleased(KEY_ESCAPE)) {
    if(!state->player->is_player_have_skill_points) toggle_is_game_paused();
    event_fire(EVENT_CODE_UI_SHOW_PAUSE_MENU, (event_context){0});
  }
}





