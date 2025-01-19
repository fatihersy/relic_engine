#include "user_interface.h"
#include <defines.h>
#include <settings.h>

#include "core/fmath.h"
#include "core/event.h"
#include "core/fmemory.h"

#include "fshader.h"

#define SPACE_BTW_V(OFFSET_X, OFFSET_Y, POS, DIM, f) ((Vector2){ \
    .x = (POS).x - ((DIM).x / 2.0f) + (((DIM).x / (f)) * (OFFSET_X)), \
    .y = (POS).y - ((DIM).y / 2.0f) + (((DIM).y / (f)) * (OFFSET_Y)) \
})

typedef struct user_interface_system_state {
  spritesheet_play_system spritesheet_system;
  Vector2 offset;
  Vector2 mouse_pos;
  button      buttons[BTN_ID_MAX];
  button_type button_types[BTN_TYPE_MAX];
  slider      sliders[SDR_ID_MAX];
  slider_type slider_types[SDR_TYPE_MAX];
  progress_bar prg_bars[PRG_BAR_ID_MAX];
  progress_bar_type prg_bar_types[PRG_BAR_TYPE_ID_MAX];
  Font mood_font;
  Font mood_outline_font;
  Font mini_mood_font;
  Font mini_mood_outline_font;
  spritesheet ss_to_draw_bg;
  bool b_show_pause_menu;
  bool b_show_settings_menu;
  panel default_panel;
} user_interface_system_state;

static user_interface_system_state *state;

#define DEFAULT_MENU_BUTTON_SCALE 3

#define PSPRITESHEET_SYSTEM state // Don't forget to undef at very bottom of the file
#include "game/spritesheet.h"

bool user_interface_on_event(u16 code, void *sender, void *listener_inst, event_context context);

void update_buttons();
void update_sliders();

void draw_slider_body(slider* sdr);
void draw_texture_stretch(texture_id body, Vector2 pos, Vector2 scale, Rectangle stretch_part, u16 stretch_part_mltp, bool should_center);
void draw_texture_regular(texture_id _id, Rectangle dest, Color tint, bool should_center);
void draw_texture_npatch(texture_id _id, Rectangle dest, Vector4 offsets, bool should_center);
void gui_draw_settings_screen();
bool gui_button(const char* text, button_id _id, Font font, f32 font_size_scale);

void _register_button(Vector2 _attached_position, button_id _btn_id, button_type_id _btn_type_id);
void register_button_type(button_type_id _btn_type_id, spritesheet_type _ss_type, Vector2 frame_dim, Vector2 on_click_text_offset, f32 _scale, bool _play_reflection, bool _play_crt, bool _should_center);
void register_progress_bar(progress_bar_id _id, progress_bar_type_id _type_id, f32 width_multiply, Vector2 scale);
void register_progress_bar_type(progress_bar_type_id _type_id, texture_id _body_inside, texture_id _body_outside, shader_id _mask_shader_id);
void _register_slider(Vector2 _pos, slider_id _sdr_id, slider_type_id _sdr_type_id, button_id _left_btn_id, button_id _right_btn_id, bool _is_clickable);
void register_slider_type(slider_type_id _sdr_type_id, spritesheet_type _ss_sdr_body_type, f32 _scale, u16 _width_multiply, button_type_id _left_btn_type_id, button_type_id _right_btn_type_id, u16 _char_limit);

#define MENU_BUTTON_FONT state->mini_mood_font
#define MENU_BUTTON_FONT_SIZE state->mini_mood_font.baseSize / 30.f
#define MINI_BUTTON_FONT state->mini_mood_font
#define MINI_BUTTON_FONT_SIZE state->mini_mood_font.baseSize / 60.f

#define register_button(BTN_ID, BTN_TYPE_ID, POS, OFFSET_X, OFFSET_Y, F) \
  _register_button( SPACE_BTW_V( OFFSET_X, OFFSET_Y, POS, state->button_types[BTN_TYPE_ID].dest_frame_dim, F), BTN_ID, BTN_TYPE_ID) 

#define register_slider(SDR_ID, SDR_TYPE_ID, POS, OFFSET, F, LEFT_BTN_ID, RIGHT_BTN_ID, IS_CLICKABLE) \
  _register_slider(SPACE_BTW_V( OFFSET.x, OFFSET.y, POS, state->slider_types[SDR_TYPE_ID].whole_body_width, F), SDR_ID, SDR_TYPE_ID, LEFT_BTN_ID, RIGHT_BTN_ID, IS_CLICKABLE) 

#define draw_text(TEXT, TEXT_POS, FONT, FONT_SIZE, COLOR)                                                               \
  DrawTextEx(FONT, TEXT,                                                                                \
  (Vector2) { .x = TEXT_POS.x + BUTTON_TEXT_SHADOW_OFFSET.x, .y = TEXT_POS.y + BUTTON_TEXT_SHADOW_OFFSET.y, },    \
  FONT_SIZE, UI_FONT_SPACING, BUTTON_TEXT_SHADOW_COLOR);                                                          \
  DrawTextEx(FONT, TEXT, TEXT_POS, FONT_SIZE, UI_FONT_SPACING, COLOR);                    

Rectangle get_texture_source_rect(texture_id _id);

void user_interface_system_initialize() {
  if (state) return;

  state = (user_interface_system_state *)allocate_memory_linear(sizeof(user_interface_system_state), true);
  
  // Loading fonts
  { 
    state->mood_font = LoadFont(rs_path("mood.ttf"));
    if (state->mood_font.baseSize == 0) { // If custom font load failed
      state->mood_font = GetFontDefault();
    }
    state->mood_outline_font = LoadFont(rs_path("mood_outline.ttf"));
    if (state->mood_outline_font.baseSize == 0) { // If custom font load failed
      state->mood_outline_font = GetFontDefault();
    }
    state->mini_mood_font = LoadFont(rs_path("mini_mood.ttf"));
    if (state->mini_mood_font.baseSize == 0) { // If custom font load failed
      state->mini_mood_font = GetFontDefault();
    }
    state->mini_mood_outline_font = LoadFont(rs_path("mini_mood_outline.ttf"));
    if (state->mini_mood_outline_font.baseSize == 0) { // If custom font load failed
      state->mini_mood_outline_font = GetFontDefault();
    }
  }
  // Loading fonts
  
  state->default_panel = (panel) {
    .signal_state = BTN_STATE_UNDEFINED,
    .bg_tex_id    = TEX_ID_CRIMSON_FANTASY_PANEL_BG,
    .frame_tex_id = TEX_ID_CRIMSON_FANTASY_PANEL,
    .bg_tint = (Color) {255, 255, 255, 200},
    .offsets = (Vector4) {6, 6, 6, 6},
  };

  initialize_shader_system();

  // BUTTON TYPES
  {  
  register_button_type(
    BTN_TYPE_MENU_BUTTON, MENU_BUTTON, 
    (Vector2){80, 16}, (Vector2){0, 2}, 
    DEFAULT_MENU_BUTTON_SCALE, true, true, false);
  register_button_type(
    BTN_TYPE_MENU_BUTTON_NO_CRT, MENU_BUTTON, 
    (Vector2){80, 16}, (Vector2){0, 2}, 
    DEFAULT_MENU_BUTTON_SCALE, true, false, false);
  register_button_type(
    BTN_TYPE_SLIDER_LEFT_BUTTON, SLIDER_LEFT_BUTTON, 
    (Vector2){10, 10}, (Vector2){0, 0}, 
    DEFAULT_MENU_BUTTON_SCALE, false, false, false);
  register_button_type(
    BTN_TYPE_SLIDER_RIGHT_BUTTON, SLIDER_RIGHT_BUTTON, 
    (Vector2){10, 10}, (Vector2){0, 0}, 
    DEFAULT_MENU_BUTTON_SCALE, false, false, false);
  register_button_type(
    BTN_TYPE_FLAT_BUTTON, FLAT_BUTTON, 
    (Vector2){44, 14}, (Vector2){0, 0}, 
    2, false, false, false);
  }
  // BUTTON TYPES

  // SLIDER TYPES
  {
  register_slider_type(
    SDR_TYPE_PERCENT, SLIDER_PERCENT, 
    DEFAULT_MENU_BUTTON_SCALE, DEFAULT_MENU_BUTTON_SCALE,
    BTN_TYPE_SLIDER_LEFT_BUTTON, BTN_TYPE_SLIDER_RIGHT_BUTTON, 6
  );
  register_slider_type(
    SDR_TYPE_OPTION, SLIDER_OPTION, 
    DEFAULT_MENU_BUTTON_SCALE, 2,
    BTN_TYPE_SLIDER_LEFT_BUTTON, BTN_TYPE_SLIDER_RIGHT_BUTTON, 6
  );
  }
  // SLIDER TYPES

  // PROGRES BAR TYPES
  {
    register_progress_bar_type(
      PRG_BAR_TYPE_ID_CRIMSON_FANT_BAR,
      TEX_ID_PROGRESS_BAR_INSIDE_FULL, TEX_ID_PROGRESS_BAR_OUTSIDE_FULL,
      SHADER_ID_PROGRESS_BAR_MASK
    );
  }
  // PROGRES BAR TYPES

  // PROGRES BARS
  {
    register_progress_bar(
      PRG_BAR_ID_PLAYER_EXPERIANCE,
      PRG_BAR_TYPE_ID_CRIMSON_FANT_BAR,
      5,
      (Vector2) {3,3}
    );
    register_progress_bar(
      PRG_BAR_ID_PLAYER_HEALTH,
      PRG_BAR_TYPE_ID_CRIMSON_FANT_BAR,
      3,
      (Vector2) {3,3}
    );
  }
  // PROGRES BARS

  // IN GAME
  {
  }
  // IN GAME

  // MAIN MENU
  {  
  register_button(BTN_ID_MAINMENU_BUTTON_PLAY,     BTN_TYPE_MENU_BUTTON_NO_CRT, *get_resolution_div2(), 0,  0, 3.f);
  register_button(BTN_ID_MAINMENU_BUTTON_EDITOR,   BTN_TYPE_MENU_BUTTON_NO_CRT, *get_resolution_div2(), 0,  4, 3.f);
  register_button(BTN_ID_MAINMENU_BUTTON_SETTINGS, BTN_TYPE_MENU_BUTTON_NO_CRT, *get_resolution_div2(), 0,  8, 3.f);
  register_button(BTN_ID_MAINMENU_BUTTON_EXTRAS,   BTN_TYPE_MENU_BUTTON_NO_CRT, *get_resolution_div2(), 0, 12, 3.f);
  register_button(BTN_ID_MAINMENU_BUTTON_EXIT,     BTN_TYPE_MENU_BUTTON_NO_CRT, *get_resolution_div2(), 0, 16, 3.f);
  }
  // MAIN MENU

  // EDITOR
  {
  register_button(BTN_ID_EDITOR_BUTTON_LOAD_MAP, BTN_TYPE_MENU_BUTTON, *get_resolution_div2(), 0, 1, 3.f);
  register_button(BTN_ID_EDITOR_BUTTON_SAVE_MAP, BTN_TYPE_MENU_BUTTON, *get_resolution_div2(), 0, 3, 3.f);
  }
  // EDITOR

  // USER INTERFACE
  {
  register_button(BTN_ID_PAUSEMENU_BUTTON_INVENTORY,   BTN_TYPE_FLAT_BUTTON, *get_resolution_div2(), -3.00f, -7.5f, 1.f);
  register_button(BTN_ID_PAUSEMENU_BUTTON_TECHNOLOGIES,BTN_TYPE_FLAT_BUTTON, *get_resolution_div2(), -1.50f, -7.5f, 1.f);
  register_button(BTN_ID_PAUSEMENU_BUTTON_SETTINGS,    BTN_TYPE_FLAT_BUTTON, *get_resolution_div2(),  0.00f, -7.5f, 1.f);
  register_button(BTN_ID_PAUSEMENU_BUTTON_CREDITS,     BTN_TYPE_FLAT_BUTTON, *get_resolution_div2(),  1.50f, -7.5f, 1.f);
  register_button(BTN_ID_PAUSEMENU_BUTTON_EXIT,        BTN_TYPE_FLAT_BUTTON, *get_resolution_div2(),  3.00f, -7.5f, 1.f);
  }
  // USER INTERFACE

  // SETTINGS
  {
  register_slider(
    SDR_ID_SETTINGS_SOUND_SLIDER,  SDR_TYPE_PERCENT, 
    *get_resolution_div2(), ((Vector2) {0, 0}), 3.f, 
    BTN_ID_SETTINGS_SLIDER_SOUND_LEFT_BUTTON, BTN_ID_SETTINGS_SLIDER_SOUND_RIGHT_BUTTON, true);
  register_slider(
    SDR_ID_SETTINGS_RES_SLIDER,  SDR_TYPE_OPTION, 
    *get_resolution_div2(), ((Vector2) {0, 5}), 3.f, 
    BTN_ID_SETTINGS_SLIDER_RES_LEFT_BUTTON, BTN_ID_SETTINGS_SLIDER_RES_RIGHT_BUTTON, false);
  register_slider(
    SDR_ID_SETTINGS_WIN_MODE_SLIDER,  SDR_TYPE_OPTION, 
    *get_resolution_div2(), ((Vector2) {0, 10}), 3.f, 
    BTN_ID_SETTINGS_SLIDER_WIN_MODE_LEFT_BUTTON, BTN_ID_SETTINGS_SLIDER_WIN_MODE_RIGHT_BUTTON, false);
  register_button(BTN_ID_SETTINGS_APPLY_SETTINGS_BUTTON, BTN_TYPE_MENU_BUTTON,            *get_resolution_div2(), -2, 15, 3.f);
  register_button(BTN_ID_MAIN_MENU_SETTINGS_CANCEL_SETTINGS_BUTTON, BTN_TYPE_MENU_BUTTON, *get_resolution_div2(),  2, 15, 3.f);
  }
  // SETTINGS

  // SLIDER OPTIONS
  {
  gui_slider_add_option(SDR_ID_SETTINGS_RES_SLIDER, "1920x1080", (data_pack) {
    .data.u16[0] = 1920,
    .data.u16[1] = 1080,
    .array_lenght = 2,
    .type_flag = DATA_TYPE_U16
  });
  gui_slider_add_option(SDR_ID_SETTINGS_RES_SLIDER, "2560x1440", (data_pack) {
    .data.u16[0] = 2560,
    .data.u16[1] = 1440,
    .array_lenght = 2,
    .type_flag = DATA_TYPE_U16
  });
  gui_slider_add_option(SDR_ID_SETTINGS_RES_SLIDER, "3840x2160", (data_pack) {
    .data.u16[0] = 3840,
    .data.u16[1] = 2160,
    .array_lenght = 2,
    .type_flag = DATA_TYPE_U16
  });
  gui_slider_add_option(SDR_ID_SETTINGS_WIN_MODE_SLIDER, "WINDOWED", (data_pack) {
    .data.i32[0] = 0,
    .array_lenght = 0,
    .type_flag = DATA_TYPE_C
  });
  gui_slider_add_option(SDR_ID_SETTINGS_WIN_MODE_SLIDER, "BORDERLESS", (data_pack) {
    .data.i32[0] = FLAG_BORDERLESS_WINDOWED_MODE,
    .array_lenght = 0,
    .type_flag = DATA_TYPE_C
  });
  gui_slider_add_option(SDR_ID_SETTINGS_WIN_MODE_SLIDER, "FULL SCREEN", (data_pack) {
    .data.i32[0] = FLAG_FULLSCREEN_MODE,
    .array_lenght = 0,
    .type_flag = DATA_TYPE_C
  });
  }
  // SLIDER OPTIONS

  event_register(EVENT_CODE_UI_SHOW_PAUSE_MENU, 0, user_interface_on_event);
  event_register(EVENT_CODE_UI_SHOW_SETTINGS_MENU, 0, user_interface_on_event);
  event_register(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, 0, user_interface_on_event);
}

void update_user_interface() {
  state->mouse_pos = GetMousePosition();
  state->offset = (Vector2) { get_screen_offset(), get_screen_offset()};
  update_sprite_renderqueue();

  update_buttons();
  update_sliders();
}

void update_buttons() {
  for (int i = 0; i < BTN_ID_MAX; ++i) {
    if (state->buttons[i].id == BTN_ID_UNDEFINED) {
      continue;
    }
    if (!state->buttons[i].on_screen) { continue; }

    button* btn = &state->buttons[i];
    if (CheckCollisionPointRec(state->mouse_pos, btn->dest)) {
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        btn->state = BTN_STATE_PRESSED;
        if(state->buttons[btn->id].btn_type.play_reflection)  {
          stop_sprite(state->buttons[btn->id].reflection_render_index, true);
        }
      } else {
        if (btn->state == BTN_STATE_PRESSED) { 
          btn->state = BTN_STATE_RELEASED;
        }
        else if (btn->state != BTN_STATE_HOVER) {
          btn->state = BTN_STATE_HOVER;
        }
      }
    } else {
      if (btn->state != BTN_STATE_UP) { 
        if(state->buttons[btn->id].btn_type.play_reflection)  {
          reset_sprite(btn->reflection_render_index, true);
        }
        btn->state = BTN_STATE_UP;
      }
    }
    btn->on_screen = false;
  }
}
void update_sliders() {
  for (int i = 0; i < SDR_ID_MAX; ++i) {
    if (state->sliders[i].id == SDR_ID_UNDEFINED || !state->sliders[i].on_screen) continue;
      slider* sdr = &state->sliders[i];
      if (!sdr->is_registered) {
        TraceLog(LOG_WARNING, "WARNING::user_interface::update_sliders()::Using slider didn't registered");
        sdr->on_screen = false;
        continue;
      }
      if (sdr->sdr_type.id != SDR_TYPE_PERCENT || !sdr->is_clickable) { continue; } // Only percent type sliders needs update every frame

      Rectangle sdr_rect = (Rectangle) {
        sdr->position.x, sdr->position.y, 
        sdr->sdr_type.body_width * sdr->sdr_type.width_multiply, sdr->sdr_type.body_height
      };
    
      if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(state->mouse_pos, sdr_rect)) {
          f32 relative = state->mouse_pos.x - sdr_rect.x;
          f32 ratio = relative / sdr_rect.width;

          sdr->current_value = ratio * sdr->max_value + 1;
          FCLAMP(sdr->current_value, sdr->min_value, sdr->max_value);
      }
    }
    
    sdr->on_screen = false;
  }  
}

void render_user_interface() {
  
  if (state->b_show_pause_menu) {
    gui_draw_pause_screen();
  }

  if (state->b_show_settings_menu) {
    gui_draw_settings_screen();
  }
}
bool gui_menu_button(const char* text, button_id _id) {
  return gui_button(text, _id, MENU_BUTTON_FONT, MENU_BUTTON_FONT_SIZE);
}
bool gui_mini_button(const char* text, button_id _id) {
  return gui_button(text, _id, MINI_BUTTON_FONT, MINI_BUTTON_FONT_SIZE);
}
bool gui_slider_button(button_id _id) {
  return gui_button("", _id, (Font) {0}, 0);
}

bool gui_button(const char* text, button_id _id, Font font, f32 font_size_scale) {
  if (_id >= BTN_ID_MAX || _id <= BTN_ID_UNDEFINED) {
    TraceLog(LOG_WARNING, "WARNING::user_interface::gui_button()::Recieved button type out of bound");
    return false;
  }

  button* _btn = &state->buttons[_id];
  if (!_btn->is_registered) {
    TraceLog(LOG_WARNING, "WARNING::user_interface::gui_button()::The button is not registered");
    return false;
  }
  button_type* _btn_type = &_btn->btn_type;
  _btn->on_screen = true;

  Vector2 text_measure;
  Vector2 text_pos;

  if (!TextIsEqual(text, "")) {
    text_measure = MeasureTextEx(font, text, font.baseSize * font_size_scale, UI_FONT_SPACING);
    text_pos = (Vector2) {
      .x = _btn->dest.x + (_btn->dest.width / 2.f) - (text_measure.x / 2.f),
      .y = _btn->dest.y + (_btn->dest.height / 2.f) - (text_measure.y / 2.f)
    };
  }

  _btn_type->play_crt ? play_sprite_on_site(_btn->crt_render_index, WHITE, _btn->dest) : 0;

  Vector2 pos = (Vector2) {_btn->dest.x, _btn->dest.y};
  Vector2 draw_sprite_scale = (Vector2) {_btn->btn_type.scale,_btn->btn_type.scale};

  if (_btn->state == BTN_STATE_PRESSED) {
    draw_sprite_on_site(_btn->btn_type.ss_type, WHITE, pos, draw_sprite_scale, 1, false);
    if (!TextIsEqual(text, "")) {
      Vector2 pressed_text_pos = vec2_add(text_pos, _btn_type->text_offset_on_click);
      draw_text(text, pressed_text_pos, font, font.baseSize * font_size_scale, BUTTON_TEXT_PRESSED_COLOR);
    }
  } else {
    draw_sprite_on_site(_btn->btn_type.ss_type, WHITE, pos, draw_sprite_scale, 0, false);
    if (_btn->state == BTN_STATE_HOVER) {
      _btn_type->play_reflection ? play_sprite_on_site(_btn->reflection_render_index, WHITE, _btn->dest) : 0;
      if (!TextIsEqual(text, "")) {
        draw_text(text, text_pos, font, font.baseSize * font_size_scale, BUTTON_TEXT_HOVER_COLOR);
      }
    }
    if (_btn->state != BTN_STATE_HOVER) {
      draw_text(text, text_pos, font, font.baseSize * font_size_scale, BUTTON_TEXT_UP_COLOR);
    }
  }
  return _btn->state == BTN_STATE_RELEASED;
}

void gui_progress_bar(progress_bar_id bar_id, Vector2 pos, bool _should_center) {
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::gui_player_experiance_process()::ui system didn't initialized");
    return;
  }
  progress_bar prg_bar = state->prg_bars[bar_id];
  if (!prg_bar.is_initialized) {
    TraceLog(LOG_ERROR, "user_interface::gui_player_experiance_process()::Player experiance process bar didn't initialized");
    return;
  }

  draw_texture_stretch(
    prg_bar.type.body_outside, 
    pos, 
    prg_bar.scale, 
    (Rectangle) {.x = 27, .y = 0, .width = 10, .height = 9},
    prg_bar.width_multiply,
    _should_center
  );

  BeginShaderMode(get_shader_by_enum(prg_bar.type.mask_shader_id)->handle);
  set_shader_uniform(prg_bar.type.mask_shader_id, 0, (data_pack) {.data.f32[0] = prg_bar.progress});
  draw_texture_stretch(
    prg_bar.type.body_inside, 
    pos, 
    prg_bar.scale, 
    (Rectangle) {.x = 27, .y = 0, .width = 10, .height = 9},
    prg_bar.width_multiply,
    _should_center
  );
  EndShaderMode();
}

void draw_texture_stretch(texture_id body, Vector2 pos, Vector2 scale, Rectangle stretch_part, u16 stretch_part_mltp, bool should_center) {
  if (body>= TEX_ID_MAX || body<= TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "user_interface::draw_repetitive_body_tex()::Recieved texture out of bound");
    return;
  }
  Texture2D* body_tex    = get_texture_by_enum(body);
  if (!body_tex) {
    TraceLog(LOG_ERROR, "user_interface::draw_repetitive_body_tex()::Recieved texture returned NULL");
    return;
  }

  Rectangle first_source = (Rectangle){
    .x = 0, .y = 0,
    .width = stretch_part.x, .height = stretch_part.height,
  };
  Rectangle first_dest = (Rectangle){
    .x = pos.x, .y = pos.y,
    .width = first_source.width * scale.x, .height = first_source.height * scale.y,
  };
  Rectangle second_dest = (Rectangle){
    .x = pos.x + first_dest.width, .y = pos.y,
    .width = stretch_part.width * scale.x * stretch_part_mltp, .height = stretch_part.height * scale.y,
  };
  Rectangle third_source = (Rectangle){
    .x = stretch_part.x + stretch_part.width, .y = 0,
    .width = body_tex->width - (stretch_part.x + stretch_part.width), .height = stretch_part.height,
  };
  Rectangle third_dest = (Rectangle){
    .x = pos.x + first_dest.width + second_dest.width, .y = pos.y,
    .width = third_source.width * scale.x, .height = third_source.height * scale.y,
  };
  if (should_center) {
    first_dest.x  -= first_dest.width + (second_dest.width / 2.f);
    second_dest.x -= first_dest.width + (second_dest.width / 2.f);
    third_dest.x  -= first_dest.width + (second_dest.width / 2.f);
  }
  DrawTexturePro(*body_tex, first_source, first_dest, (Vector2) {0}, 0, WHITE);

  DrawTexturePro(*body_tex, stretch_part, second_dest, (Vector2) {0}, 0, WHITE);
  
  DrawTexturePro(*body_tex, third_source, third_dest, (Vector2) {0}, 0, WHITE);
}

void gui_slider(slider_id _id) {
  if (_id >= SDR_ID_MAX || _id <= SDR_ID_UNDEFINED || !state) {
    TraceLog(LOG_WARNING, "user_interface::gui_slider()::One of recieved ids was out of bound");
    return;
  }
  slider* sdr = &state->sliders[_id];
  if (!sdr) {
    TraceLog(LOG_WARNING, "user_interface::gui_slider()::Slider %d returned NULL", _id);
    return;
  }
  slider_type sdr_type = sdr->sdr_type;
  if (!sdr->is_registered || (sdr_type.id == SDR_TYPE_OPTION && sdr->max_value <= 0)) return;
 
  sdr->on_screen = true;

  draw_slider_body(sdr);

  if(sdr_type.left_btn_id != 0) if (gui_slider_button(sdr_type.left_btn_id)) {
    if (sdr->current_value > sdr->min_value) {
      sdr->current_value--;
    }
  }

  if(sdr_type.right_btn_id != 0) if (gui_slider_button(sdr_type.right_btn_id)) {
    if (sdr->current_value < sdr->max_value-1) {
      sdr->current_value++;
    }
  }
}

void draw_slider_body(slider* sdr) {
  slider_type sdr_type = sdr->sdr_type;

  switch (sdr->sdr_type.id) {
    case SDR_TYPE_PERCENT: {
      u16 scaled_value = sdr->current_value * sdr_type.scale;
      u16 scaled_max   = sdr->max_value     * sdr_type.scale;
      Vector2 draw_sprite_scale = (Vector2) {sdr_type.scale, sdr_type.scale};

      for (int i = 0; i < sdr_type.width_multiply; ++i) {
        Vector2 _pos = sdr->position;
        _pos.x += i * sdr_type.body_width; 

        draw_sprite_on_site(
        sdr_type.ss_sdr_body, WHITE, _pos, draw_sprite_scale, 
        FMIN(scaled_value, sdr->max_value), false
        );

        scaled_value -= scaled_value < sdr->max_value ? scaled_value : sdr->max_value;
      }
      break;
    }
    case SDR_TYPE_OPTION: {
      u16 total_body_width = sdr_type.body_width * sdr_type.width_multiply;
      u16 each_body_width = (total_body_width - ((sdr->max_value) * get_screen_offset())) / (sdr->max_value-1);
      f32 each_body_scale = (float)each_body_width / sdr_type.origin_body_width;
      Vector2 draw_sprite_scale = (Vector2) {each_body_scale, sdr_type.scale};
      Vector2 _pos_temp = (Vector2) {sdr->position.x + get_screen_offset(), sdr->position.y};
      const char* text = TextFormat("%s", sdr->options[sdr->current_value].display_text);
      Vector2 text_measure = MeasureTextEx(state->mood_font, text, state->mood_font.baseSize, UI_FONT_SPACING);

      for (int i = 1; i < sdr->max_value; ++i) {
        Vector2 _pos = _pos_temp;
        _pos.x += (each_body_width + get_screen_offset()) * (i-1); 

        draw_sprite_on_site(
          sdr_type.ss_sdr_body, WHITE, 
          _pos, draw_sprite_scale, 
          (i == sdr->current_value) ? 1 : 0, false
        );
      }

      Vector2 text_pos = (Vector2) {
        sdr->position.x + total_body_width/2.f - text_measure.x / 2.f,
        sdr->position.y + sdr_type.body_height/2.f - text_measure.y / 2.f
      };

      draw_text(text, text_pos, state->mood_font, state->mood_font.baseSize, BUTTON_TEXT_UP_COLOR);
      break;
    }

    default: TraceLog(LOG_WARNING, "WARNING::user_interface::render_slider_body()::Unsupported slider type");
    break;
  }
}

void gui_panel(panel pan, Rectangle dest, bool _should_center) {
  
  draw_texture_regular(pan.bg_tex_id, dest, pan.bg_tint, _should_center);
  draw_texture_npatch(pan.frame_tex_id, dest, pan.offsets, _should_center);
}
bool gui_panel_clickable(panel* pan, Rectangle dest, bool _should_center) {

  if (_should_center) {
    dest.x -= dest.width / 2.f;
    dest.y -= dest.height / 2.f;
  }

  if (CheckCollisionPointRec(state->mouse_pos, dest)) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      pan->current_state = BTN_STATE_PRESSED;
    } else {
      if (pan->current_state == BTN_STATE_PRESSED) { 
        pan->current_state = BTN_STATE_RELEASED;
      }
      else if (pan->current_state != BTN_STATE_HOVER) {
        pan->current_state = BTN_STATE_HOVER;
      }
    }
  } else {
    if (pan->current_state != BTN_STATE_UP) { 
      pan->current_state = BTN_STATE_UP;
    }
  }

  (pan->current_state == BTN_STATE_HOVER) 
    ? draw_texture_regular(pan->bg_tex_id, dest, pan->bg_hover_tint, false)
    : draw_texture_regular(pan->bg_tex_id, dest, pan->bg_tint, false);

  draw_texture_npatch(pan->frame_tex_id, dest, pan->offsets, false);

  return pan->current_state == pan->signal_state;
}

void gui_draw_settings_screen() { // TODO: Return to settings later

  gui_slider(SDR_ID_SETTINGS_SOUND_SLIDER);

  gui_slider(SDR_ID_SETTINGS_RES_SLIDER);

  gui_slider(SDR_ID_SETTINGS_WIN_MODE_SLIDER);

  if(gui_menu_button("Apply", BTN_ID_MAIN_MENU_SETTINGS_CANCEL_SETTINGS_BUTTON)) {
    slider sdr_win_mode = state->sliders[SDR_ID_SETTINGS_WIN_MODE_SLIDER];
    i32 window_mod = sdr_win_mode.options[sdr_win_mode.current_value].content.data.i32[0];

    if (window_mod == FLAG_BORDERLESS_WINDOWED_MODE) {
      event_fire(EVENT_CODE_TOGGLE_BORDERLESS, 0, (event_context) {0});
    }
    else if (window_mod == FLAG_FULLSCREEN_MODE) {
      event_fire(EVENT_CODE_TOGGLE_FULLSCREEN, 0, (event_context) {0});
    }
    else if (window_mod == 0) {
      event_fire(EVENT_CODE_TOGGLE_WINDOWED, 0, (event_context) {0});
    }
  }
}

void gui_draw_pause_screen() {
  Rectangle dest = (Rectangle) {
    get_resolution_div2()->x,
    get_resolution_div2()->y,
    854,480 //TODO: Make it responsive based on resolution
  };
  gui_panel(state->default_panel, dest, true);

  if (gui_mini_button("Inventory", BTN_ID_PAUSEMENU_BUTTON_INVENTORY)) {
    state->b_show_pause_menu = !state->b_show_pause_menu;
  }
  if (gui_mini_button("Update", BTN_ID_PAUSEMENU_BUTTON_TECHNOLOGIES)) {

  }
  if (gui_mini_button("Settings", BTN_ID_PAUSEMENU_BUTTON_SETTINGS)) {
    
  }
  if (gui_mini_button("Credits", BTN_ID_PAUSEMENU_BUTTON_CREDITS)) {
    
  }
  if (gui_mini_button("Exit", BTN_ID_PAUSEMENU_BUTTON_EXIT)) {
    event_fire(EVENT_CODE_APPLICATION_QUIT, 0, (event_context) {0});
  }
}
bool gui_slider_add_option(slider_id _id, const char* _display_text, data_pack content) {
  if (_id >= SDR_ID_MAX || _id <= SDR_ID_UNDEFINED || !state) {
    TraceLog(LOG_WARNING, "WARNING::user_interface::gui_slider_add_option()::Slider ids was out of bound");
    return false;
  }
  slider* sdr = &state->sliders[_id];
  if (!sdr->is_registered) {
    TraceLog(LOG_WARNING, "WARNING::user_interface::gui_slider_add_option()::Given slider didn't registered");
    return false;
  }
  sdr->options[sdr->max_value] = (slider_option) {
    .display_text = 0,
    .content = content
  };
  TextCopy(sdr->options[sdr->max_value].display_text, _display_text);

  sdr->max_value++;

  return true;
}
void register_button_type(button_type_id _btn_type_id, spritesheet_type _ss_type, Vector2 frame_dim, Vector2 on_click_text_offset, f32 _scale, bool _play_reflection, bool _play_crt, bool _should_center) {
  if (_ss_type     >= SPRITESHEET_TYPE_MAX || _ss_type <= SPRITESHEET_UNSPECIFIED || 
      _btn_type_id >= BTN_TYPE_MAX         || _btn_type_id <= BTN_TYPE_UNDEFINED  ||
      !state) {
    TraceLog(LOG_WARNING, "WARNING::user_interface::register_button_type()::Recieved id was out of bound");
    return;
  }
  button_type btn_type = {
    .id = _btn_type_id,
    .scale = _scale,
    .ss_type = _ss_type,
    .source_frame_dim = frame_dim,
    .text_offset_on_click = (Vector2) { .x = on_click_text_offset.x * _scale, .y = on_click_text_offset.y * _scale},
    .play_crt = _play_crt,
    .play_reflection = _play_reflection,
    .should_center = _should_center,
    .dest_frame_dim = (Vector2) {
      .x = frame_dim.x * _scale,
      .y = frame_dim.y * _scale,
    }
  };
  state->button_types[_btn_type_id] = btn_type;
}
void _register_button(Vector2 _attached_position, button_id _btn_id, button_type_id _btn_type_id) {
  if (_btn_id      >= BTN_ID_MAX   || _btn_id      <= BTN_ID_UNDEFINED   || 
      _btn_type_id >= BTN_TYPE_MAX || _btn_type_id <= BTN_TYPE_UNDEFINED || !state) 
  {
    TraceLog(LOG_WARNING, "WARNING::user_interface::register_button()::One of recieved ids was out of bound");
    return;
  }

  button_type* _btn_type = &state->button_types[_btn_type_id];

  if (_btn_type->should_center) {
    _attached_position.x -= _btn_type->dest_frame_dim.x/2.f;
    _attached_position.y -= _btn_type->dest_frame_dim.y/2.f;
  }

  button btn = {
    .id = _btn_id,
    .btn_type = state->button_types[_btn_type_id],
    .dest = (Rectangle) {
      .x = _attached_position.x, .y = _attached_position.y,
      .width = _btn_type->dest_frame_dim.x, .height = _btn_type->dest_frame_dim.y
    },
    .state = BTN_STATE_UP,
    .crt_render_index = _btn_type->play_crt 
      ? register_sprite(BUTTON_CRT_SHEET, true, false, false)
      : 0,
    .reflection_render_index = _btn_type->play_reflection 
      ? register_sprite(BUTTON_REFLECTION_SHEET, false, true, false)
      : 0,
    .on_screen = false,
    .is_registered = true,
  };

  state->buttons[_btn_id] = btn;
}
void register_progress_bar(progress_bar_id _id, progress_bar_type_id _type_id, f32 width_multiply, Vector2 scale) {
  if (_id     >= PRG_BAR_ID_MAX      || _id      <= PRG_BAR_ID_UNDEFINED      ||
      _type_id>= PRG_BAR_TYPE_ID_MAX || _type_id <= PRG_BAR_TYPE_ID_UNDEFINED ||
      !state) {
    TraceLog(LOG_WARNING, "user_interface::register_progress_bar()::Recieved id was out of bound");
    return;
  }
  progress_bar prg_bar = {0};

  prg_bar.type = state->prg_bar_types[_type_id];
  prg_bar.id = _id;
  prg_bar.scale = scale;
  prg_bar.width_multiply = width_multiply;
  prg_bar.is_initialized = true;

  state->prg_bars[_id] = prg_bar;
}
void register_progress_bar_type(progress_bar_type_id _type_id, texture_id _body_inside, texture_id _body_outside, shader_id _mask_shader_id) {
  if (_type_id       >= PRG_BAR_TYPE_ID_MAX || _type_id       <= PRG_BAR_TYPE_ID_UNDEFINED ||
      _body_inside   >= TEX_ID_MAX          || _body_inside   <= TEX_ID_UNSPECIFIED        ||
      _body_outside  >= TEX_ID_MAX          || _body_outside  <= TEX_ID_UNSPECIFIED        ||
      _mask_shader_id>= SHADER_ID_MAX       || _mask_shader_id<= SHADER_ID_UNSPECIFIED     ||
      !state) {
    TraceLog(LOG_WARNING, "user_interface::register_progress_bar_type()::Recieved id was out of bound");
    return;
  }

  progress_bar_type prg_type = {0};

  prg_type.body_inside     = _body_inside;
  prg_type.body_outside    = _body_outside;
  prg_type.mask_shader_id  = _mask_shader_id;

  state->prg_bar_types[_type_id] = prg_type;
}
void register_slider_type(
  slider_type_id _sdr_type_id, spritesheet_type _ss_sdr_body_type, f32 _scale, u16 _width_multiply,
  button_type_id _left_btn_type_id, button_type_id _right_btn_type_id, u16 _char_limit) {
    if (_ss_sdr_body_type >= SPRITESHEET_TYPE_MAX || _ss_sdr_body_type <= SPRITESHEET_UNSPECIFIED || 
        _sdr_type_id      >= SDR_TYPE_MAX         || _sdr_type_id      <= SDR_TYPE_UNDEFINED      ||
        _left_btn_type_id >= BTN_TYPE_MAX         || _left_btn_type_id <= BTN_TYPE_UNDEFINED      ||
        _right_btn_type_id>= BTN_TYPE_MAX         || _right_btn_type_id<= BTN_TYPE_UNDEFINED      ||
        !state) {
      TraceLog(LOG_WARNING, "WARNING::user_interface::register_slider_type()::One of recieved ids was out of bound");
      return;
    }

  spritesheet ss_body = get_spritesheet_by_enum(_ss_sdr_body_type);
  button_type* left_btn_type = &state->button_types[_left_btn_type_id]; 
  button_type* right_btn_type = &state->button_types[_right_btn_type_id]; 

  slider_type sdr_type = {
    .id = _sdr_type_id,
    .scale = _scale,
    .ss_sdr_body = _ss_sdr_body_type,
    .source_frame_dim = (Vector2) {
      .x = ss_body.current_frame_rect.width, .y = ss_body.current_frame_rect.height
    },
    .left_btn_type_id = _left_btn_type_id,
    .right_btn_type_id = _right_btn_type_id,
    .left_btn_width = left_btn_type->source_frame_dim.x * left_btn_type->scale,
    .right_btn_width = right_btn_type->source_frame_dim.x * right_btn_type->scale,
    .origin_body_width = ss_body.current_frame_rect.width,
    .body_width = ss_body.current_frame_rect.width * _scale,
    .body_height = ss_body.current_frame_rect.height * _scale,
    .width_multiply = _width_multiply,
    .char_limit = _scale * _char_limit,
  };
  sdr_type.whole_body_width.x = (sdr_type.body_width * sdr_type.width_multiply) + sdr_type.left_btn_width + sdr_type.right_btn_width;
  sdr_type.whole_body_width.y = sdr_type.body_height;

  state->slider_types[_sdr_type_id] = sdr_type;
}
/**
 * @param _is_clickable for SDR_TYPE_PERCENT type sliders. Does not affect others
 */
void _register_slider(
  Vector2 _pos, slider_id _sdr_id, slider_type_id _sdr_type_id, 
  button_id _left_btn_id, button_id _right_btn_id, bool _is_clickable) {
  if (_sdr_id       >= SDR_ID_MAX   || _sdr_id      <= SDR_ID_UNDEFINED  || 
      _left_btn_id  >= BTN_ID_MAX   || _left_btn_id  < BTN_ID_UNDEFINED  || 
      _right_btn_id >= BTN_ID_MAX   || _right_btn_id < BTN_ID_UNDEFINED  || 
      _sdr_type_id  >= SDR_TYPE_MAX || _sdr_type_id <= SDR_TYPE_UNDEFINED||
      !state) 
  {
    TraceLog(LOG_WARNING, "user_interface::register_slider()::One of recieved ids was out of bound");
    return;
  }

  slider_type* _sdr_type = &state->slider_types[_sdr_type_id];
  
  slider sdr = {
    .id = _sdr_id,
    .sdr_type = *_sdr_type,
    .position = (Vector2) {
      .x = _pos.x, .y = _pos.y
    },
    .current_value = _sdr_type_id == SDR_TYPE_PERCENT ? 7 : 1,
    .max_value = _sdr_type_id == SDR_TYPE_PERCENT ? 10 : 1,
    .min_value = 1,
    .is_clickable = _is_clickable,
    .on_screen = false,
    .is_registered = true,
  };
  if(_left_btn_id != BTN_ID_UNDEFINED){
    sdr.sdr_type.left_btn_id = _left_btn_id;
    _register_button((Vector2) { 
        .x = sdr.position.x - state->button_types[_sdr_type->left_btn_type_id].dest_frame_dim.x, 
        .y = sdr.position.y }, 
      sdr.sdr_type.left_btn_id, sdr.sdr_type.left_btn_type_id
    );
  }
  if(_right_btn_id != BTN_ID_UNDEFINED){
    sdr.sdr_type.right_btn_id = _right_btn_id;
    _register_button((Vector2) { 
        .x = sdr.position.x + _sdr_type->body_width * _sdr_type->width_multiply, 
        .y = sdr.position.y },
      sdr.sdr_type.right_btn_id, sdr.sdr_type.right_btn_type_id
    );
  }

  state->sliders[_sdr_id] = sdr;
}
void gui_draw_texture_to_background(texture_id _id) {
  draw_texture_regular(_id, (Rectangle) {0, 0, GetScreenWidth(), GetScreenHeight()}, WHITE, false);
}
void gui_draw_spritesheet_to_background(spritesheet_type _type, Color _tint) {
  if (_type >= SPRITESHEET_TYPE_MAX || _type <= SPRITESHEET_UNSPECIFIED || !state) {
    TraceLog(LOG_WARNING, "WARNING::user_interface::gui_draw_spritesheet_to_background()::Sprite type out of bound");
    return;
  }
  if (state->ss_to_draw_bg.type != _type) {
    state->ss_to_draw_bg = get_spritesheet_by_enum(_type);
    state->ss_to_draw_bg.render_queue_index = register_sprite(_type, true, false, false);
  }
  Rectangle dest = (Rectangle) {0, 0, GetScreenWidth(), GetScreenHeight()};
  play_sprite_on_site(state->ss_to_draw_bg.render_queue_index, _tint, dest);
}
/**
 * @note inline function, returns "(Rectangle) {0}" if texture type returns null pointer
 * @return (Rectangle) { .x = 0, .y = 0, .width = tex->width, .height = tex->height}; 
 */
inline Rectangle get_texture_source_rect(texture_id _id) {
  Texture2D* tex = get_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "WARNING::user_interface::get_texture_source_rect()::Requested type was null");
    return (Rectangle) {0}; 
  }
  
  return (Rectangle) { .x = 0, .y = 0, .width = tex->width, .height = tex->height};
}
/**
 * @brief Centers over -> dest.x -= dest.width / 2.f; 
 */
inline void draw_texture_regular(texture_id _id, Rectangle dest, Color tint, bool should_center) {
  if (_id >= TEX_ID_MAX || _id <= TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, 
    "user_interface::draw_texture_regular()::ID was out of bound"); 
    return; 
  }
  Texture2D* tex = get_texture_by_enum(_id);
  if (!tex) { TraceLog(
  LOG_WARNING, "WARNING::user_interface::draw_texture_regular()::Tex was null");
    return; 
  }
  if (should_center) {
    dest.x -= dest.width / 2.f; 
    dest.y -= dest.height / 2.f; 
  }
  DrawTexturePro(*tex, 
  (Rectangle) { 0, 0, tex->width, tex->height}, 
  dest, 
  (Vector2) {0}, 0, tint);
}
/**
 * @brief Centers over -> dest.x -= dest.width / 2.f; 
 */
inline void draw_texture_npatch(texture_id _id, Rectangle dest, Vector4 offsets, bool should_center) {
  if (_id >= TEX_ID_MAX || _id <= TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, 
    "user_interface::draw_texture_npatch()::ID was out of bound"); 
    return; 
  }
  Texture2D* tex = get_texture_by_enum(_id);
  if (!tex) { TraceLog(LOG_WARNING, 
    "user_interface::draw_texture_npatch()::Tex was null"); 
    return; 
  }

  if (should_center) {
    dest.x -= dest.width / 2.f; 
    dest.y -= dest.height / 2.f; 
  }

  NPatchInfo npatch = (NPatchInfo){
    (Rectangle) {0, 0, tex->width, tex->height},
    offsets.x,
    offsets.y,
    offsets.z,
    offsets.w,
    NPATCH_NINE_PATCH
  };

  DrawTextureNPatch(*tex, npatch, dest, (Vector2) {0}, 0, WHITE);
}
Font* ui_get_font(font_type font) {
  if (!state) {
    TraceLog(LOG_WARNING, "user_interface::user_interface_state_get_font()::user interface didn't initialized. Returning default font");
    return (Font*) {0};
  }
  switch (font) {
  case FONT_TYPE_MOOD:              return &state->mood_font;
  case FONT_TYPE_MOOD_OUTLINE:      return &state->mood_outline_font;
  case FONT_TYPE_MINI_MOOD:         return &state->mini_mood_font;
  case FONT_TYPE_MINI_MOOD_OUTLINE: return &state->mini_mood_outline_font;
  default: TraceLog(LOG_WARNING, "user_interface::ui_get_font()::Unknown font type");
  }

  return (Font*) {0};
}

void user_interface_system_destroy() {

}

bool user_interface_on_event(u16 code, void *sender, void *listener_inst, event_context context) {
  switch (code) {
    case EVENT_CODE_UI_SHOW_PAUSE_MENU: {
      state->b_show_pause_menu = !state->b_show_pause_menu;
      return true;
    }
    case EVENT_CODE_UI_SHOW_SETTINGS_MENU: {
      state->b_show_settings_menu = !state->b_show_settings_menu;
      return true;
    }
    case EVENT_CODE_UI_UPDATE_PROGRESS_BAR: {
      state->prg_bars[(i32)context.data.f32[0]].progress = context.data.f32[1];
      return true;
    }
  };

  return false;
}

#undef PSPRITESHEET_SYSTEM
