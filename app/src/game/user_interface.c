#include "user_interface.h"
#include <reasings.h>
#include <settings.h>

#include "core/fmath.h"
#include "core/event.h"
#include "core/fmemory.h"

#include "fshader.h"

typedef struct user_interface_system_state {
  button      buttons[BTN_ID_MAX];
  button_type button_types[BTN_TYPE_MAX];
  slider      sliders[SDR_ID_MAX];
  slider_type slider_types[SDR_TYPE_MAX];
  progress_bar prg_bars[PRG_BAR_ID_MAX];
  progress_bar_type prg_bar_types[PRG_BAR_TYPE_ID_MAX];
  
  panel default_panel;
  spritesheet ss_to_draw_bg;
  
  Vector2 offset;
  Vector2 mouse_pos;

  Font mood_font;
  Font mood_outline_font;
  Font mini_mood_font;
  Font mini_mood_outline_font;

  u16 fade_animation_duration;
  f32 fade_animation_timer;
  bool fadein;
  bool fade_animation_playing;
  bool b_show_pause_menu;
  bool b_show_settings_menu;
} user_interface_system_state;

static user_interface_system_state *restrict state;

#define MAX_UI_WORDWRAP_WORD_LENGTH 20
#define MAX_UI_WORDWRAP_SENTENCE_LENGTH 300
#define MENU_BUTTON_FONT state->mini_mood_font
#define MENU_BUTTON_FONT_SIZE state->mini_mood_font.baseSize / 30.f
#define MINI_BUTTON_FONT state->mini_mood_font
#define MINI_BUTTON_FONT_SIZE state->mini_mood_font.baseSize / 60.f
#define LABEL_MOOD_FONT_SIZE state->mood_font.baseSize * .1f
#define LABEL_MOOD_OUTLINE_FONT_SIZE state->mood_outline_font.baseSize * .1f
#define LABEL_MINI_FONT_SIZE state->mini_mood_font.baseSize * .1f
#define LABEL_MINI_OUTLINE_FONT_SIZE state->mini_mood_outline_font.baseSize * .1f
#define DEFAULT_MENU_BUTTON_SCALE 3
#define SPACE_BTW_V(OFFSET_X, OFFSET_Y, POS, DIM, f) ((Vector2){  \
  .x = (POS).x - ((DIM).x / 2.0f) + (((DIM).x / (f)) * (OFFSET_X)), \
  .y = (POS).y - ((DIM).y / 2.0f) + (((DIM).y / (f)) * (OFFSET_Y)) \
})
#define draw_text(TEXT, TEXT_POS, FONT, FONT_SIZE, COLOR, CENTER)                                                 \
  if (CENTER) {                                                                                                   \
    Vector2 text_measure = MeasureTextEx(FONT, TEXT, FONT_SIZE, UI_FONT_SPACING);                                 \
    TEXT_POS.x -= (text_measure.x / 2.f);                                                                         \
    TEXT_POS.y -= (text_measure.y / 2.f);                                                                         \
  }                                                                                                               \
  DrawTextEx(FONT, TEXT,                                                                                          \
  (Vector2) { .x = TEXT_POS.x + TEXT_SHADOW_OFFSET.x, .y = TEXT_POS.y + TEXT_SHADOW_OFFSET.y, },                  \
  FONT_SIZE, UI_FONT_SPACING, TEXT_SHADOW_COLOR);                                                                 \
  DrawTextEx(FONT, TEXT, TEXT_POS, FONT_SIZE, UI_FONT_SPACING, COLOR);                    

#define SDR_CURR_VAL(ID) state->sliders[ID].options[state->sliders[ID].current_value]

#define PSPRITESHEET_SYSTEM state->spritesheet_system // Don't forget to undef at very bottom of the file
#include "game/spritesheet.h"

bool user_interface_on_event(u16 code, event_context context);

void update_buttons(void);
void update_sliders(void);

void draw_fade_effect();
void draw_slider_body(slider* sdr);
void draw_atlas_texture_stretch(atlas_texture_id body, Vector2 pos, Vector2 scale, Rectangle stretch_part, u16 stretch_part_mltp, bool should_center);
void draw_atlas_texture_regular(atlas_texture_id _id, Rectangle dest, Color tint, bool should_center);
void draw_atlas_texture_npatch(atlas_texture_id _id, Rectangle dest, Vector4 offsets, bool should_center);
void gui_draw_settings_screen(void);
bool gui_button(const char* text, button_id _id, Font font, f32 font_size_scale, Vector2 pos);

void register_button(button_id _btn_id, button_type_id _btn_type_id);
void register_button_type(button_type_id _btn_type_id, spritesheet_id _ss_type, Vector2 frame_dim, Vector2 on_click_text_offset, f32 _scale, bool _play_reflection, bool _should_center);
void register_progress_bar(progress_bar_id _id, progress_bar_type_id _type_id, f32 width_multiply, Vector2 scale);
void register_progress_bar_type(progress_bar_type_id _type_id, atlas_texture_id _body_inside, atlas_texture_id _body_outside, shader_id _mask_shader_id);
void register_slider(slider_id _sdr_id, slider_type_id _sdr_type_id, button_id _left_btn_id, button_id _right_btn_id, bool _is_clickable);
void register_slider_type(slider_type_id _sdr_type_id, spritesheet_id _ss_sdr_body_type, f32 _scale, u16 _width_multiply, button_type_id _left_btn_type_id, button_type_id _right_btn_type_id, u16 _char_limit);

Rectangle get_texture_source_rect(texture_id _id);
void DrawTextBoxed(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint);
const char* wrap_text(const char* text, Font font, i32 font_size, Rectangle bounds, bool center_x);

Vector2 make_vector(f32 x, f32 y);

void user_interface_system_initialize(void) {
  if (state) {
    TraceLog(LOG_WARNING, "user_interface::user_interface_system_initialize()::Initialize called twice");
    return;
  }

  state = (user_interface_system_state *)allocate_memory_linear(sizeof(user_interface_system_state), true);
  
  // Loading fonts
  { 
    state->mood_font = LoadFont(_rs_path("mood.ttf"));
    if (state->mood_font.baseSize == 0) { // If custom font load failed
      state->mood_font = GetFontDefault();
    }
    state->mood_outline_font = LoadFont(_rs_path("mood_outline.ttf"));
    if (state->mood_outline_font.baseSize == 0) { // If custom font load failed
      state->mood_outline_font = GetFontDefault();
    }
    state->mini_mood_font = LoadFont(_rs_path("mini_mood.ttf"));
    if (state->mini_mood_font.baseSize == 0) { // If custom font load failed
      state->mini_mood_font = GetFontDefault();
    }
    state->mini_mood_outline_font = LoadFont(_rs_path("mini_mood_outline.ttf"));
    if (state->mini_mood_outline_font.baseSize == 0) { // If custom font load failed
      state->mini_mood_outline_font = GetFontDefault();
    }
  }
  // Loading fonts
  
  state->default_panel = (panel) {
    .signal_state = BTN_STATE_UNDEFINED,
    .bg_tex_id    = ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG,
    .frame_tex_id = ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL,
    .bg_tint = (Color) { 30, 39, 46, 245},
    .offsets = (Vector4) {6, 6, 6, 6},
  };

  initialize_shader_system();

  // BUTTON TYPES
  {  
  register_button_type(
    BTN_TYPE_MENU_BUTTON, SHEET_ID_MENU_BUTTON, 
    (Vector2){80, 16}, (Vector2){0, 2}, 
    DEFAULT_MENU_BUTTON_SCALE, true, false);
  register_button_type(
    BTN_TYPE_MENU_BUTTON_NO_CRT, SHEET_ID_MENU_BUTTON, 
    (Vector2){80, 16}, (Vector2){0, 2}, 
    DEFAULT_MENU_BUTTON_SCALE, true, false);
  register_button_type(
    BTN_TYPE_SLIDER_LEFT_BUTTON, SHEET_ID_SLIDER_LEFT_BUTTON, 
    (Vector2){10, 10}, (Vector2){0, 0}, 
    DEFAULT_MENU_BUTTON_SCALE, false, false);
  register_button_type(
    BTN_TYPE_SLIDER_RIGHT_BUTTON, SHEET_ID_SLIDER_RIGHT_BUTTON, 
    (Vector2){10, 10}, (Vector2){0, 0}, 
    DEFAULT_MENU_BUTTON_SCALE, false, false);
  register_button_type(
    BTN_TYPE_FLAT_BUTTON, SHEET_ID_FLAT_BUTTON, 
    (Vector2){44, 14}, (Vector2){0, 0}, 
    2, false, false);
  }
  // BUTTON TYPES

  // SLIDER TYPES
  {
  register_slider_type(
    SDR_TYPE_PERCENT, SHEET_ID_SLIDER_PERCENT, 
    DEFAULT_MENU_BUTTON_SCALE, DEFAULT_MENU_BUTTON_SCALE,
    BTN_TYPE_SLIDER_LEFT_BUTTON, BTN_TYPE_SLIDER_RIGHT_BUTTON, 6
  );
  register_slider_type(
    SDR_TYPE_OPTION, SHEET_ID_SLIDER_OPTION, 
    DEFAULT_MENU_BUTTON_SCALE, 2,
    BTN_TYPE_SLIDER_LEFT_BUTTON, BTN_TYPE_SLIDER_RIGHT_BUTTON, 6
  );
  }
  // SLIDER TYPES

  // PROGRES BAR TYPES
  {
    register_progress_bar_type(
      PRG_BAR_TYPE_ID_CRIMSON_FANT_BAR,
      ATLAS_TEX_ID_PROGRESS_BAR_INSIDE_FULL, ATLAS_TEX_ID_PROGRESS_BAR_OUTSIDE_FULL,
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
  register_button(BTN_ID_MAINMENU_BUTTON_PLAY,     BTN_TYPE_MENU_BUTTON_NO_CRT);
  register_button(BTN_ID_MAINMENU_BUTTON_EDITOR,   BTN_TYPE_MENU_BUTTON_NO_CRT);
  register_button(BTN_ID_MAINMENU_BUTTON_SETTINGS, BTN_TYPE_MENU_BUTTON_NO_CRT);
  register_button(BTN_ID_MAINMENU_BUTTON_EXTRAS,   BTN_TYPE_MENU_BUTTON_NO_CRT);
  register_button(BTN_ID_MAINMENU_BUTTON_EXIT,     BTN_TYPE_MENU_BUTTON_NO_CRT);
  register_button(BTN_ID_MAINMENU_SETTINGS_CANCEL, BTN_TYPE_MENU_BUTTON_NO_CRT);
  register_button(BTN_ID_MAINMENU_BACK_BUTTON,     BTN_TYPE_MENU_BUTTON_NO_CRT);
  }
  // MAIN MENU

  // EDITOR
  {
    register_slider(
      SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER, SDR_TYPE_OPTION, 
      BTN_ID_EDITOR_ACTIVE_TILEMAP_EDIT_LAYER_DEC,BTN_ID_EDITOR_ACTIVE_TILEMAP_EDIT_LAYER_INC, false);
    register_button(BTN_ID_EDITOR_BTN_STAGE_MAP_CHANGE_LEFT, BTN_TYPE_SLIDER_LEFT_BUTTON);
    register_button(BTN_ID_EDITOR_BTN_STAGE_MAP_CHANGE_RIGHT, BTN_TYPE_SLIDER_LEFT_BUTTON);
  }
  // EDITOR

  // USER INTERFACE
  {
  register_button(BTN_ID_PAUSEMENU_BUTTON_INVENTORY,   BTN_TYPE_FLAT_BUTTON);
  register_button(BTN_ID_PAUSEMENU_BUTTON_TECHNOLOGIES,BTN_TYPE_FLAT_BUTTON);
  register_button(BTN_ID_PAUSEMENU_BUTTON_SETTINGS,    BTN_TYPE_FLAT_BUTTON);
  register_button(BTN_ID_PAUSEMENU_BUTTON_CREDITS,     BTN_TYPE_FLAT_BUTTON);
  register_button(BTN_ID_PAUSEMENU_BUTTON_EXIT,        BTN_TYPE_FLAT_BUTTON);
  }
  // USER INTERFACE

  // SETTINGS
  {
  register_slider(
    SDR_ID_SETTINGS_SOUND_SLIDER,  SDR_TYPE_PERCENT, 
    BTN_ID_SETTINGS_SLIDER_SOUND_LEFT_BUTTON, BTN_ID_SETTINGS_SLIDER_SOUND_RIGHT_BUTTON, true);
  register_slider(
    SDR_ID_SETTINGS_RES_SLIDER,  SDR_TYPE_OPTION, 
    BTN_ID_SETTINGS_SLIDER_RES_LEFT_BUTTON, BTN_ID_SETTINGS_SLIDER_RES_RIGHT_BUTTON, false);
  register_slider(
    SDR_ID_SETTINGS_WIN_MODE_SLIDER,  SDR_TYPE_OPTION, 
    BTN_ID_SETTINGS_SLIDER_WIN_MODE_LEFT_BUTTON, BTN_ID_SETTINGS_SLIDER_WIN_MODE_RIGHT_BUTTON, false);
  register_button(BTN_ID_SETTINGS_APPLY_SETTINGS_BUTTON, BTN_TYPE_MENU_BUTTON);
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

  event_register(EVENT_CODE_UI_SHOW_PAUSE_MENU, user_interface_on_event);
  event_register(EVENT_CODE_UI_SHOW_SETTINGS_MENU, user_interface_on_event);
  event_register(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, user_interface_on_event);
  event_register(EVENT_CODE_UI_START_FADEIN_EFFECT, user_interface_on_event);
  event_register(EVENT_CODE_UI_START_FADEOUT_EFFECT, user_interface_on_event);
}

void update_user_interface(void) {
  state->mouse_pos = GetMousePosition();
  state->offset = get_screen_offset();
  
  update_buttons();
  update_sliders();
  if (state->fade_animation_playing) {
    if(state->fade_animation_timer == state->fade_animation_duration){
      state->fade_animation_timer = 0;
      state->fade_animation_playing = false;
    }
    else state->fade_animation_timer++; 
  }
}

void update_buttons(void) {
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
          stop_sprite(&state->buttons[btn->id].reflection_anim, true);
        }
      } else {
        if (btn->state == BTN_STATE_PRESSED) { 
          btn->state = BTN_STATE_RELEASED;
        }
        else if (btn->state != BTN_STATE_HOVER) {
          btn->state = BTN_STATE_HOVER;
        }
        if (btn->state == BTN_STATE_HOVER && btn->btn_type.play_reflection) {
          update_sprite(&btn->reflection_anim);
        }
      }
    } else {
      if (btn->state != BTN_STATE_UP) { 
        if(state->buttons[btn->id].btn_type.play_reflection)  {
          reset_sprite(&btn->reflection_anim, true);
        }
        btn->state = BTN_STATE_UP;
      }
    }
    btn->on_screen = false;
  }
}
void update_sliders(void) {
  for (int i = 0; i < SDR_ID_MAX; ++i) {
    if (state->sliders[i].id == SDR_ID_UNDEFINED || !state->sliders[i].on_screen) continue;
      slider* sdr = &state->sliders[i];
      if (!sdr->is_registered) {
        TraceLog(LOG_WARNING, "user_interface::update_sliders()::Using slider didn't registered");
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

void render_user_interface(void) {
  if (state->fade_animation_playing) {
    draw_fade_effect();
    return;
  }

  if (state->b_show_pause_menu) {
    gui_draw_pause_screen();
  }
  if (state->b_show_settings_menu) {
    gui_draw_settings_screen();
  }
}
bool gui_menu_button(const char* text, button_id _id, Vector2 offset) {
  return gui_button(text, _id, 
    MENU_BUTTON_FONT, MENU_BUTTON_FONT_SIZE, 
    SPACE_BTW_V(offset.x, offset.y, *get_resolution_div2(), state->buttons[_id].btn_type.dest_frame_dim, 3.f)
  );
}
bool gui_mini_button(const char* text, button_id _id, Vector2 offset, f32 offset_scale) {
  return gui_button(text, _id, 
    MINI_BUTTON_FONT, MINI_BUTTON_FONT_SIZE, 
    SPACE_BTW_V(offset.x, offset.y, *get_resolution_div2(), state->buttons[_id].btn_type.dest_frame_dim, offset_scale)
  );
}
bool gui_slider_button(button_id _id, Vector2 pos) {
  return gui_button("", _id, (Font) {0}, 0, pos);
}

bool gui_button(const char* text, button_id _id, Font font, f32 font_size_scale, Vector2 pos) {
  if (_id >= BTN_ID_MAX || _id <= BTN_ID_UNDEFINED) {
    TraceLog(LOG_WARNING, "user_interface::gui_button()::Recieved button type out of bound");
    return false;
  }

  button* _btn = &state->buttons[_id];
  if (!_btn->is_registered) {
    TraceLog(LOG_WARNING, "user_interface::gui_button()::The button is not registered");
    return false;
  }
  button_type* _btn_type = &_btn->btn_type;
  _btn->on_screen = true;
  _btn->dest.x = pos.x;
  _btn->dest.y = pos.y;

  Vector2 text_measure;
  Vector2 text_pos;

  if (!TextIsEqual(text, "")) {
    text_measure = MeasureTextEx(font, text, font.baseSize * font_size_scale, UI_FONT_SPACING);
    text_pos = (Vector2) {
      .x = _btn->dest.x + (_btn->dest.width / 2.f)  - (text_measure.x / 2.f),
      .y = _btn->dest.y + (_btn->dest.height / 2.f) - (text_measure.y / 2.f)
    };
  }

  Vector2 draw_sprite_scale = (Vector2) {_btn->btn_type.scale,_btn->btn_type.scale};

  if (_btn->state == BTN_STATE_PRESSED) {
    draw_sprite_on_site_by_id(_btn->btn_type.ss_type, WHITE, VECTOR2(_btn->dest.x,_btn->dest.y), draw_sprite_scale, 1, false);
    if (!TextIsEqual(text, "")) {
      Vector2 pressed_text_pos = vec2_add(text_pos, _btn_type->text_offset_on_click);
      draw_text(text, pressed_text_pos, font, font.baseSize * font_size_scale, BUTTON_TEXT_PRESSED_COLOR, false);
    }
  } else {
    draw_sprite_on_site_by_id(_btn->btn_type.ss_type, WHITE, VECTOR2(_btn->dest.x,_btn->dest.y), draw_sprite_scale, 0, false);
    if (_btn->state == BTN_STATE_HOVER) {
      if(_btn_type->play_reflection) {play_sprite_on_site(&_btn->reflection_anim, WHITE, _btn->dest);};
      if (!TextIsEqual(text, "")) {
        draw_text(text, text_pos, font, font.baseSize * font_size_scale, BUTTON_TEXT_HOVER_COLOR, false);
      }
    }
    if (_btn->state != BTN_STATE_HOVER) {
      draw_text(text, text_pos, font, font.baseSize * font_size_scale, BUTTON_TEXT_UP_COLOR, false);
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
  atlas_texture* inside_tex = _get_atlas_texture_by_enum(prg_bar.type.body_inside);
  Texture2D* atlas = _get_texture_by_enum(ATLAS_TEXTURE_ID);
  if (!inside_tex) {
    TraceLog(LOG_ERROR, "user_interface::gui_player_experiance_process()::progress bar atlas is null");
    return;
  }
  f32 start_uv = inside_tex->source.x / atlas->width;
  f32 end_uv = (inside_tex->source.x + inside_tex->source.width) / atlas->width;

  f32 process = start_uv + (end_uv - start_uv) * prg_bar.progress;

  draw_atlas_texture_stretch(
    prg_bar.type.body_outside, 
    pos, 
    prg_bar.scale, 
    (Rectangle) {.x = 27, .y = 0, .width = 10, .height = 9},
    prg_bar.width_multiply,
    _should_center
  );

  BeginShaderMode(get_shader_by_enum(prg_bar.type.mask_shader_id)->handle);
  set_shader_uniform(prg_bar.type.mask_shader_id, 0, (data_pack) {.data.f32[0] = process});
  draw_atlas_texture_stretch(
    prg_bar.type.body_inside, 
    pos,
    prg_bar.scale, 
    (Rectangle) {.x = 27, .y = 0, .width = 10, .height = 9},
    prg_bar.width_multiply,
    _should_center
  );
  EndShaderMode();
}

void draw_atlas_texture_stretch(atlas_texture_id body, Vector2 pos, Vector2 scale, Rectangle stretch_part, u16 stretch_part_mltp, bool should_center) {
  if (body >= ATLAS_TEX_ID_MAX || body <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "user_interface::draw_repetitive_body_tex()::Recieved texture out of bound");
    return;
  }
  atlas_texture* body_tex = _get_atlas_texture_by_enum(body);
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
    .width = body_tex->source.width - (stretch_part.x + stretch_part.width), .height = stretch_part.height,
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
  gui_draw_atlas_texture_id_pro(body, first_source, first_dest, true);
  gui_draw_atlas_texture_id_pro(body, stretch_part, second_dest, true);
  gui_draw_atlas_texture_id_pro(body, third_source, third_dest, true);
}

void gui_slider(slider_id _id, Vector2 pos, Vector2 offset, f32 offset_scale) {
  if (_id >= SDR_ID_MAX || _id <= SDR_ID_UNDEFINED || !state) {
    TraceLog(LOG_WARNING, "user_interface::gui_slider()::One of recieved ids was out of bound");
    return;
  }
  slider* sdr = &state->sliders[_id];
  if (!sdr) {
    TraceLog(LOG_WARNING, "user_interface::gui_slider()::Slider %d returned NULL", _id);
    return;
  }
  slider_type* sdr_type = &sdr->sdr_type;
  sdr->position = SPACE_BTW_V( offset.x, offset.y, pos, sdr_type->whole_body_width, offset_scale);
  if (!sdr->is_registered || (sdr_type->id == SDR_TYPE_OPTION && sdr->max_value <= 0)) return;
 
  button* btn_left = &state->buttons[sdr->sdr_type.left_btn_id];
  button* btn_right = &state->buttons[sdr->sdr_type.right_btn_id];
  Vector2 btn_left_dest = (Vector2) { 
    .x = sdr->position.x - state->button_types[sdr_type->left_btn_type_id].dest_frame_dim.x, 
    .y = sdr->position.y 
  };
  Vector2 btn_right_dest = (Vector2) { 
    .x = sdr->position.x + sdr_type->body_width * sdr_type->width_multiply, 
    .y = sdr->position.y 
  };
  btn_left->dest.x = btn_left_dest.x;
  btn_left->dest.y = btn_left_dest.y;
  btn_right->dest.x = btn_right_dest.x;
  btn_right->dest.y = btn_right_dest.y;

  sdr->on_screen = true;
  draw_slider_body(sdr);
  if(sdr_type->left_btn_id != 0) if (gui_slider_button(sdr_type->left_btn_id, btn_left_dest)) {
    if (sdr->current_value > sdr->min_value) {
      sdr->current_value--;
    }
  }

  if(sdr_type->right_btn_id != 0) if (gui_slider_button(sdr_type->right_btn_id, btn_right_dest)) {
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
      Vector2 draw_sprite_scale = (Vector2) {sdr_type.scale, sdr_type.scale};

      for (int i = 0; i < sdr_type.width_multiply; ++i) {
        Vector2 _pos = sdr->position;
        _pos.x += i * sdr_type.body_width; 

        draw_sprite_on_site_by_id(sdr_type.ss_sdr_body, WHITE, _pos, draw_sprite_scale, FMIN(scaled_value, sdr->max_value), false);
        scaled_value -= scaled_value < sdr->max_value ? scaled_value : sdr->max_value;
      }
      break;
    }
    case SDR_TYPE_OPTION: {
      u16 total_body_width = sdr_type.body_width * sdr_type.width_multiply;
      u16 each_body_width = (total_body_width - ((sdr->max_value) * get_screen_offset().x)) / (sdr->max_value-1);
      f32 each_body_scale = (float)each_body_width / sdr_type.origin_body_width;
      Vector2 draw_sprite_scale = (Vector2) {each_body_scale, sdr_type.scale};
      Vector2 _pos_temp = (Vector2) {sdr->position.x + get_screen_offset().x, sdr->position.y};
      const char* text = TextFormat("%s", sdr->options[sdr->current_value].display_text);
      Vector2 text_measure = MeasureTextEx(state->mood_font, text, state->mood_font.baseSize, UI_FONT_SPACING);

      for (int i = 1; i < sdr->max_value; ++i) {
        Vector2 _pos = _pos_temp;
        _pos.x += (each_body_width + get_screen_offset().x) * (i-1); 
        draw_sprite_on_site_by_id(sdr_type.ss_sdr_body, WHITE, _pos, draw_sprite_scale, (i == sdr->current_value) ? 1 : 0, false);
      }
      Vector2 text_pos = (Vector2) {
        sdr->position.x + total_body_width/2.f - text_measure.x / 2.f,
        sdr->position.y + sdr_type.body_height/2.f - text_measure.y / 2.f
      };
      draw_text(text, text_pos, state->mood_font, state->mood_font.baseSize, BUTTON_TEXT_UP_COLOR, false);
      break;
    }
    default: TraceLog(LOG_WARNING, "user_interface::render_slider_body()::Unsupported slider type");
    break;
  }
}

void gui_panel(panel pan, Rectangle dest, bool _should_center) {
  
  //draw_texture_regular(pan.bg_tex_id, dest, pan.bg_tint, _should_center);
  Rectangle bg_dest = dest;
  if (_should_center) {
    bg_dest.x -= bg_dest.width / 2.f;
    bg_dest.y -= bg_dest.height / 2.f;
  }
  DrawRectanglePro(bg_dest, (Vector2) {0, 0}, 0, pan.bg_tint);
  draw_atlas_texture_npatch(pan.frame_tex_id, dest, pan.offsets, _should_center);
}
bool gui_panel_active(panel* pan, Rectangle dest, bool _should_center) {

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
    ? DrawRectanglePro(dest, (Vector2) {0, 0}, 0, pan->bg_hover_tint)  //draw_texture_regular(pan->bg_tex_id, dest, pan->bg_hover_tint, false)
    : DrawRectanglePro(dest, (Vector2) {0, 0}, 0, pan->bg_tint); //draw_texture_regular(pan->bg_tex_id, dest, pan->bg_tint, false);

  draw_atlas_texture_npatch(pan->frame_tex_id, dest, pan->offsets, false);

  return pan->current_state == pan->signal_state;
}
void gui_label(const char* text, font_type type, i32 font_size, Vector2 position, Color tint, bool _should_center) {
  switch (type) {
    case FONT_TYPE_MOOD: {
      draw_text(text, position, state->mood_font, font_size * LABEL_MOOD_FONT_SIZE, tint, _should_center);
      break;
    }
    case FONT_TYPE_MOOD_OUTLINE: {
      draw_text(text, position, state->mood_outline_font, font_size * LABEL_MOOD_OUTLINE_FONT_SIZE, tint, _should_center);
      break;
    }
    case FONT_TYPE_MINI_MOOD: {
      draw_text(text, position, state->mini_mood_font, font_size * LABEL_MINI_FONT_SIZE, tint, _should_center);
      break;
    }
    case FONT_TYPE_MINI_MOOD_OUTLINE: {
      draw_text(text, position, state->mini_mood_outline_font, font_size * LABEL_MINI_OUTLINE_FONT_SIZE, tint, _should_center);
      break;
    }
    default: TraceLog(LOG_WARNING, "user_interface::gui_label()::Unsupported font type");
    break;
  }
}
void gui_label_wrap(const char* text, font_type type, i32 font_size, Rectangle position, Color tint, bool _should_center) {
  if (_should_center) {
    position.x -= (position.width / 2.f);
    position.y -= (position.height / 2.f);
  }
  
  switch (type) {
    case FONT_TYPE_MOOD: {
      DrawTextBoxed(state->mood_font, text, position, font_size * LABEL_MOOD_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_MOOD_OUTLINE: {
      DrawTextBoxed(state->mood_outline_font, text, position, font_size * LABEL_MOOD_OUTLINE_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_MINI_MOOD: {
      DrawTextBoxed(state->mini_mood_font, text, position, font_size * LABEL_MINI_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_MINI_MOOD_OUTLINE: {
      DrawTextBoxed(state->mini_mood_outline_font, text, position, font_size * LABEL_MINI_OUTLINE_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    default: TraceLog(LOG_WARNING, "user_interface::gui_label()::Unsupported font type");
    break;
  }
}

void gui_draw_settings_screen(void) { // TODO: Return to settings later

  gui_slider(SDR_ID_SETTINGS_SOUND_SLIDER, get_app_settings()->resolution_div2, VECTOR2(0,0), 3.f);

  gui_slider(SDR_ID_SETTINGS_RES_SLIDER, get_app_settings()->resolution_div2, VECTOR2(0,5), 3.f);

  gui_slider(SDR_ID_SETTINGS_WIN_MODE_SLIDER, get_app_settings()->resolution_div2, VECTOR2(0,10), 3.f);

  if(gui_menu_button("Apply", BTN_ID_SETTINGS_APPLY_SETTINGS_BUTTON, VECTOR2(-2,15))) {
    slider sdr_win_mode = state->sliders[SDR_ID_SETTINGS_WIN_MODE_SLIDER];
    i32 window_mod = sdr_win_mode.options[sdr_win_mode.current_value].content.data.i32[0];

    if (window_mod == FLAG_BORDERLESS_WINDOWED_MODE && !IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) {
      event_fire(EVENT_CODE_TOGGLE_BORDERLESS, (event_context) {0});
    }
    else if (window_mod == FLAG_FULLSCREEN_MODE && !IsWindowFullscreen()) {    
      event_fire(EVENT_CODE_TOGGLE_FULLSCREEN, (event_context) {0});
      Vector2 new_res = pVECTOR2(SDR_CURR_VAL(SDR_ID_SETTINGS_RES_SLIDER).content.data.u16);
      set_resolution(new_res.x, new_res.y);
    }
    else if (window_mod == 0) {
      event_fire(EVENT_CODE_TOGGLE_WINDOWED, (event_context) {0});
      Vector2 new_res = pVECTOR2(SDR_CURR_VAL(SDR_ID_SETTINGS_RES_SLIDER).content.data.u16);
      SetWindowSize(new_res.x, new_res.y);
      set_resolution(new_res.x, new_res.y);
    }
  }
}
void gui_draw_pause_screen(void) {
  Rectangle dest = (Rectangle) {
    get_resolution_div2()->x,
    get_resolution_div2()->y,
    get_resolution_5div4()->x,
    get_resolution_5div4()->y
  };
  gui_panel(state->default_panel, dest, true);

  if (gui_mini_button("Inventory", BTN_ID_PAUSEMENU_BUTTON_INVENTORY, (Vector2) {-5.00f, -10.f}, 1.1f)) {
    state->b_show_pause_menu = !state->b_show_pause_menu;
  }
  if (gui_mini_button("Update", BTN_ID_PAUSEMENU_BUTTON_TECHNOLOGIES, (Vector2) {-2.50f, -10.f}, 1.1f)) {}
  if (gui_mini_button("Settings", BTN_ID_PAUSEMENU_BUTTON_SETTINGS,   (Vector2) { 0.00f, -10.f}, 1.1f)) {}
  if (gui_mini_button("Credits", BTN_ID_PAUSEMENU_BUTTON_CREDITS,     (Vector2) { 2.50f, -10.f}, 1.1f)) {}
  if (gui_mini_button("Exit", BTN_ID_PAUSEMENU_BUTTON_EXIT,           (Vector2) { 5.00f, -10.f}, 1.1f)) {
    event_fire(EVENT_CODE_APPLICATION_QUIT, (event_context) {0});
  }
}
bool gui_slider_add_option(slider_id _id, const char* _display_text, data_pack content) {
  if (_id >= SDR_ID_MAX || _id <= SDR_ID_UNDEFINED || !state) {
    TraceLog(LOG_WARNING, "user_interface::gui_slider_add_option()::Slider ids was out of bound");
    return false;
  }
  slider* sdr = &state->sliders[_id];
  if (!sdr->is_registered) {
    TraceLog(LOG_WARNING, "user_interface::gui_slider_add_option()::Given slider didn't registered");
    return false;
  }
  if (sdr->max_value < MAX_SLIDER_OPTION_SLOT) {
    sdr->options[sdr->max_value] = (slider_option) {
      .display_text = {0},
      .content = content
    };
    TextCopy(sdr->options[sdr->max_value].display_text, _display_text);
    sdr->max_value++;
    return true;
  }
  else {
    TraceLog(LOG_ERROR, "user_interface::gui_slider_add_option()::You've reached the maximum amouth of option slot");
    return false;
  }
}
void register_button_type(button_type_id _btn_type_id, spritesheet_id _ss_type, Vector2 frame_dim, Vector2 on_click_text_offset, f32 _scale, bool _play_reflection, bool _should_center) {
  if (_ss_type     >= SHEET_ID_SPRITESHEET_TYPE_MAX || _ss_type <= SHEET_ID_SPRITESHEET_UNSPECIFIED || 
      _btn_type_id >= BTN_TYPE_MAX         || _btn_type_id <= BTN_TYPE_UNDEFINED  ||
      !state) {
    TraceLog(LOG_WARNING, "user_interface::register_button_type()::Recieved id was out of bound");
    return;
  }
  button_type btn_type = {
    .id = _btn_type_id,
    .scale = _scale,
    .ss_type = _ss_type,
    .source_frame_dim = frame_dim,
    .text_offset_on_click = (Vector2) { .x = on_click_text_offset.x * _scale, .y = on_click_text_offset.y * _scale},
    .play_reflection = _play_reflection,
    .should_center = _should_center,
    .dest_frame_dim = (Vector2) {
      .x = frame_dim.x * _scale,
      .y = frame_dim.y * _scale,
    }
  };
  state->button_types[_btn_type_id] = btn_type;
}
void register_button(button_id _btn_id, button_type_id _btn_type_id) {
  if (_btn_id      >= BTN_ID_MAX   || _btn_id      <= BTN_ID_UNDEFINED   || 
      _btn_type_id >= BTN_TYPE_MAX || _btn_type_id <= BTN_TYPE_UNDEFINED || !state) 
  {
    TraceLog(LOG_WARNING, "user_interface::register_button()::One of recieved ids was out of bound");
    return;
  }

  button_type* _btn_type = &state->button_types[_btn_type_id];

  button btn = {
    .id = _btn_id,
    .btn_type = state->button_types[_btn_type_id],
    .dest = (Rectangle) {
      .x = 0, .y = 0,
      .width = _btn_type->dest_frame_dim.x, .height = _btn_type->dest_frame_dim.y
    },
    .state = BTN_STATE_UP,
    .on_screen = false,
    .is_registered = true,
  };
  btn.reflection_anim.sheet_id = SHEET_ID_BUTTON_REFLECTION_SHEET;
  if (_btn_type->play_reflection) {
    set_sprite(&btn.reflection_anim, false, true, false);
  }

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
void register_progress_bar_type(progress_bar_type_id _type_id, atlas_texture_id _body_inside, atlas_texture_id _body_outside, shader_id _mask_shader_id) {
  if (_type_id       >= PRG_BAR_TYPE_ID_MAX || _type_id       <= PRG_BAR_TYPE_ID_UNDEFINED ||
      _body_inside   >= ATLAS_TEX_ID_MAX          || _body_inside   <= ATLAS_TEX_ID_UNSPECIFIED        ||
      _body_outside  >= ATLAS_TEX_ID_MAX          || _body_outside  <= ATLAS_TEX_ID_UNSPECIFIED        ||
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
  slider_type_id _sdr_type_id, spritesheet_id _ss_sdr_body_type, f32 _scale, u16 _width_multiply,
  button_type_id _left_btn_type_id, button_type_id _right_btn_type_id, u16 _char_limit) {
    if (_ss_sdr_body_type >= SHEET_ID_SPRITESHEET_TYPE_MAX || _ss_sdr_body_type <= SHEET_ID_SPRITESHEET_UNSPECIFIED || 
        _sdr_type_id      >= SDR_TYPE_MAX         || _sdr_type_id      <= SDR_TYPE_UNDEFINED      ||
        _left_btn_type_id >= BTN_TYPE_MAX         || _left_btn_type_id <= BTN_TYPE_UNDEFINED      ||
        _right_btn_type_id>= BTN_TYPE_MAX         || _right_btn_type_id<= BTN_TYPE_UNDEFINED      ||
        !state) {
      TraceLog(LOG_WARNING, "WARNING::user_interface::register_slider_type()::One of recieved ids was out of bound");
      return;
    }

  spritesheet ss_body = *_get_spritesheet_by_enum(_ss_sdr_body_type);
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
  sdr_type.whole_body_width.x = (sdr_type.body_width * sdr_type.width_multiply);
  sdr_type.whole_body_width.y = sdr_type.body_height;

  state->slider_types[_sdr_type_id] = sdr_type;
}
/**
 * @param _is_clickable for SDR_TYPE_PERCENT type sliders. Does not affect others
 */
void register_slider(
  slider_id _sdr_id, slider_type_id _sdr_type_id, 
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
    .position = (Vector2) {0},
    .current_value = _sdr_type_id == SDR_TYPE_PERCENT ? 7 : 1,
    .max_value = _sdr_type_id == SDR_TYPE_PERCENT ? 10 : 1,
    .min_value = 1,
    .is_clickable = _is_clickable,
    .on_screen = false,
    .is_registered = true,
  };
  if(_left_btn_id != BTN_ID_UNDEFINED){
    sdr.sdr_type.left_btn_id = _left_btn_id;

    register_button(
      sdr.sdr_type.left_btn_id, sdr.sdr_type.left_btn_type_id
    );
  }
  if(_right_btn_id != BTN_ID_UNDEFINED){
    sdr.sdr_type.right_btn_id = _right_btn_id;
    register_button(
      sdr.sdr_type.right_btn_id, sdr.sdr_type.right_btn_type_id
    );
  }

  state->sliders[_sdr_id] = sdr;
}
void gui_draw_atlas_texture_to_background(atlas_texture_id _id) {
  draw_atlas_texture_regular(_id, (Rectangle) {0, 0, GetScreenWidth(), GetScreenHeight()}, WHITE, false);
}
void gui_draw_spritesheet_to_background(spritesheet_id _id, Color _tint) {
  if (_id >= SHEET_ID_SPRITESHEET_TYPE_MAX || _id <= SHEET_ID_SPRITESHEET_UNSPECIFIED || !state) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_spritesheet_to_background()::Sprite type out of bound");
    return;
  }
  if (state->ss_to_draw_bg.sheet_id != _id) {
    state->ss_to_draw_bg = *_get_spritesheet_by_enum(_id);
    set_sprite(&state->ss_to_draw_bg, true, false, false);
  }
  Rectangle dest = (Rectangle) {0, 0, GetScreenWidth(), GetScreenHeight()};
  play_sprite_on_site(&state->ss_to_draw_bg, _tint, dest);
}
/**
 * @note inline function, returns "(Rectangle) {0}" if texture type returns null pointer
 * @return (Rectangle) { .x = 0, .y = 0, .width = tex->width, .height = tex->height}; 
 */
inline Rectangle get_atlas_texture_source_rect(atlas_texture_id _id) {
  atlas_texture* tex = _get_atlas_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "user_interface::get_texture_source_rect()::Requested type was null");
    return (Rectangle) {0}; 
  }
  
  return tex->source;
}
/**
 * @brief Centers over -> dest.x -= dest.width / 2.f; 
 */
inline void draw_atlas_texture_regular(atlas_texture_id _id, Rectangle dest, Color tint, bool should_center) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, 
    "user_interface::draw_texture_regular()::ID was out of bound"); 
    return; 
  }
  atlas_texture* tex = _get_atlas_texture_by_enum(_id);
  if (!tex) { TraceLog(
  LOG_WARNING, "user_interface::draw_texture_regular()::Tex was null");
    return; 
  }
  if (should_center) {
    dest.x -= dest.width / 2.f; 
    dest.y -= dest.height / 2.f; 
  }
  DrawTexturePro(*tex->atlas_handle, 
  tex->source, 
  dest, 
  (Vector2) {0}, 0, tint);
}
/**
 * @brief Centers over -> dest.x -= dest.width / 2.f; 
 */
inline void draw_atlas_texture_npatch(atlas_texture_id _id, Rectangle dest, Vector4 offsets, bool should_center) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, 
    "user_interface::draw_texture_npatch()::ID was out of bound"); 
    return; 
  }
  atlas_texture* tex = _get_atlas_texture_by_enum(_id);
  if (!tex) { TraceLog(LOG_WARNING, 
    "user_interface::draw_texture_npatch()::Tex was null"); 
    return; 
  }

  if (should_center) {
    dest.x -= dest.width / 2.f; 
    dest.y -= dest.height / 2.f; 
  }

  NPatchInfo npatch = (NPatchInfo){
    tex->source,
    offsets.x,
    offsets.y,
    offsets.z,
    offsets.w,
    NPATCH_NINE_PATCH
  };

  DrawTextureNPatch(*tex->atlas_handle, npatch, dest, (Vector2) {0}, 0, WHITE);
}
inline Vector2 make_vector(f32 x, f32 y) {
  return (Vector2) {x,y};
}
inline void gui_draw_map_stage_pin(bool have_hovered, Vector2 screen_loc) {
  const Vector2 icon_size = NORMALIZE_VEC2(32.f, 32.f, 1280, 720);
  Rectangle icon_loc = (Rectangle) {screen_loc.x, screen_loc.y, icon_size.x * GetScreenWidth(), icon_size.y * GetScreenHeight()}; 
  icon_loc.x -= icon_loc.width  * .5f;
  icon_loc.y -= icon_loc.height * .5f;
  
  if(have_hovered) {
    gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_ICON_ATLAS, (Rectangle){64, 320, 32, 32}, icon_loc, true);
  }
  else {
    gui_draw_atlas_texture_id_pro(ATLAS_TEX_ID_ICON_ATLAS, (Rectangle){32, 320, 32, 32}, icon_loc, true);
  }
}
/**
 * @brief NOTE: Source https://github.com/raysan5/raylib/blob/master/examples/text/text_rectangle_bounds.c
 */
inline void DrawTextBoxed(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint) {
  int length = TextLength(text);  // Total length in bytes of the text, scanned by codepoints in loop
  float textOffsetY = 0;          // Offset between lines (on line break '\n')
  float textOffsetX = 0.0f;       // Offset X to next character to draw
  float scaleFactor = fontSize/(float)font.baseSize;     // Character rectangle scaling factor
  // Word/character wrapping mechanism variables
  enum { MEASURE_STATE = 0, DRAW_STATE = 1 };
  int state = wordWrap? MEASURE_STATE : DRAW_STATE;
  int startLine = -1;         // Index where to begin drawing (where a line begins)
  int endLine = -1;           // Index where to stop drawing (where a line ends)
  int lastk = -1;             // Holds last value of the character position
  for (int i = 0, k = 0; i < length; i++, k++)
  {
    // Get next codepoint from byte string and glyph index in font
    int codepointByteCount = 0;
    int codepoint = GetCodepoint(&text[i], &codepointByteCount);
    int index = GetGlyphIndex(font, codepoint);
    // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
    // but we need to draw all of the bad bytes using the '?' symbol moving one byte
    if (codepoint == 0x3f) codepointByteCount = 1;
    i += (codepointByteCount - 1);
    float glyphWidth = 0;
    if (codepoint != '\n')
    {
        glyphWidth = (font.glyphs[index].advanceX == 0) ? font.recs[index].width*scaleFactor : font.glyphs[index].advanceX*scaleFactor;
        if (i + 1 < length) glyphWidth = glyphWidth + spacing;
    }
    // NOTE: When wordWrap is ON we first measure how much of the text we can draw before going outside of the rec container
    // We store this info in startLine and endLine, then we change states, draw the text between those two variables
    // and change states again and again recursively until the end of the text (or until we get outside of the container).
    // When wordWrap is OFF we don't need the measure state so we go to the drawing state immediately
    // and begin drawing on the next line before we can get outside the container.
    if (state == MEASURE_STATE)
    {
      // TODO: There are multiple types of spaces in UNICODE, maybe it's a good idea to add support for more
      // Ref: http://jkorpela.fi/chars/spaces.html
      if ((codepoint == ' ') || (codepoint == '\t') || (codepoint == '\n')) endLine = i;
        if ((textOffsetX + glyphWidth) > rec.width)
        {
          endLine = (endLine < 1)? i : endLine;
          if (i == endLine) endLine -= codepointByteCount;
          if ((startLine + codepointByteCount) == endLine) endLine = (i - codepointByteCount);
          state = !state;
        }
        else if ((i + 1) == length){
          endLine = i;
          state = !state;
        }
        else if (codepoint == '\n') state = !state;
        if (state == DRAW_STATE)
        {
          textOffsetX = 0;
          i = startLine;
          glyphWidth = 0;
          // Save character position when we switch states
          int tmp = lastk;
          lastk = k - 1;
          k = tmp;
        }
    } else {
      if (codepoint == '\n')
      {
        if (!wordWrap)
        {
          textOffsetY += (font.baseSize + font.baseSize/2.f)*scaleFactor;
          textOffsetX = 0;
        }
      }else {
        if (!wordWrap && ((textOffsetX + glyphWidth) > rec.width)){
          textOffsetY += (font.baseSize + font.baseSize/2.f)*scaleFactor;
          textOffsetX = 0;
        }
          // When text overflows rectangle height limit, just stop drawing
          if ((textOffsetY + font.baseSize*scaleFactor) > rec.height) break;
          // Draw current character glyph
          if ((codepoint != ' ') && (codepoint != '\t'))
          {
            DrawTextCodepoint(font, codepoint, (Vector2){ 
              rec.x + textOffsetX + TEXT_SHADOW_OFFSET.x, rec.y + textOffsetY + TEXT_SHADOW_OFFSET.y}, 
              fontSize, TEXT_SHADOW_COLOR
            );
            DrawTextCodepoint(font, codepoint, (Vector2){ rec.x + textOffsetX, rec.y + textOffsetY }, fontSize, tint);
          }
      }
        if (wordWrap && (i == endLine))
        {
          textOffsetY += (font.baseSize + font.baseSize/2.f)*scaleFactor;
          textOffsetX = 0;
          startLine = endLine;
          endLine = -1;
          glyphWidth = 0;
          k = lastk;
          state = !state;
        }
      }
    if ((textOffsetX != 0) || (codepoint != ' ')) textOffsetX += glyphWidth;  // avoid leading spaces
  }
}

void draw_fade_effect() {
  if (!state) {
    TraceLog(LOG_WARNING, "user_interface::draw_fade_effect()::User interface didn't initialized");
    return;
  }
  if (!state->fade_animation_playing) {
    TraceLog(LOG_WARNING, "user_interface::draw_fade_effect()::Funtions called without starting animation");
    return;
  }
  f32 process = state->fadein 
    ? EaseQuadIn(state->fade_animation_timer,  0.f, 1.f, state->fade_animation_duration)
    : EaseQuadOut(state->fade_animation_timer, 1.f,-1.f, state->fade_animation_duration);
  BeginShaderMode(get_shader_by_enum(SHADER_ID_FADE_TRANSITION)->handle);
  set_shader_uniform(SHADER_ID_FADE_TRANSITION, 0, (data_pack) {.data.f32[0] = process});
  draw_atlas_texture_regular(ATLAS_TEX_ID_BG_BLACK, (Rectangle) {0, 0, GetScreenWidth(), GetScreenHeight()}, WHITE, false);
  EndShaderMode();
}

void gui_draw_atlas_texture_id_pro(atlas_texture_id _id, Rectangle src, Rectangle dest, bool relative) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_texture_id_pro()::ID was out of bound"); 
    return; 
  }
  atlas_texture* tex = _get_atlas_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "user_interface::gui_draw_texture_id_pro()::Tex was null");
    return; 
  }
  if (relative) {
    src.x += tex->source.x;
    src.y += tex->source.y;
  }
  DrawTexturePro(*tex->atlas_handle, src, dest, (Vector2) {0}, 0, WHITE);
}
void gui_draw_atlas_texture_id(atlas_texture_id _id, Rectangle dest) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_texture_id_pro()::ID was out of bound"); 
    return; 
  }
  atlas_texture* tex = _get_atlas_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "user_interface::gui_draw_texture_id_pro()::Tex was null");
    return; 
  }
  DrawTexturePro(*tex->atlas_handle, tex->source, dest, (Vector2) {0}, 0, WHITE);
}
void gui_draw_spritesheet_id(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, u16 frame, bool _should_center) {
  if (_id >= SHEET_ID_SPRITESHEET_TYPE_MAX || _id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_texture_id_pro()::ID was out of bound"); 
    return; 
  }
  draw_sprite_on_site_by_id(_id, _tint, pos, scale, frame, _should_center);
}
void gui_draw_atlas_texture_id_center(atlas_texture_id _id, Vector2 pos, Vector2 dim, bool should_center) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_texture_id_center()::ID was out of bound"); 
    return; 
  }
  atlas_texture* tex = _get_atlas_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "user_interface::gui_draw_texture_id_center()::Tex was null");
    return; 
  }
  if (should_center) {
    pos.x -= dim.x / 2.f; 
    pos.y -= dim.y / 2.f; 
  }

  DrawTexturePro(*tex->atlas_handle, 
  tex->source, 
  (Rectangle) {pos.x, pos.y, dim.x, dim.y}, 
  (Vector2) {0}, 0, WHITE);
}
void gui_draw_texture_id(texture_id _id, Rectangle dest) {
  if (_id >= TEX_ID_MAX || _id <= TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_texture_id()::ID was out of bound"); 
    return; 
  }
  Texture2D* tex = _get_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "user_interface::gui_draw_texture_id()::Tex was null");
    return; 
  }
  DrawTexturePro(*tex, 
  (Rectangle){0, 0, tex->width, tex->height},
  dest, 
  (Vector2) {0}, 0, WHITE);
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
panel get_default_panel(void) {
  return (panel) {
    .frame_tex_id  = ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL,
    .bg_tex_id     = ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG,
    .bg_tint       = (Color) { 30, 39, 46, 245},
    .bg_hover_tint = (Color) { 52, 64, 76, 245},
    .offsets       = (Vector4) {6, 6, 6, 6},
    .zoom          = 1.f,
    .scroll        = 0,
    .draggable     = false,
    .current_state = BTN_STATE_UP,
    .signal_state  = BTN_STATE_UNDEFINED,
    .dest          = (Rectangle) {0}
  };
}

data_pack* get_slider_current_value(slider_id id) {
  if (id >= SDR_ID_MAX || id <= SDR_ID_UNDEFINED) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_texture_id_pro()::ID was out of bound"); 
    return 0;
  }

  return &state->sliders[id].options[state->sliders[id].current_value].content;
}
bool is_ui_fade_anim_complete() {
  return state->fade_animation_timer == 0;
}
bool is_ui_fade_anim_about_to_complete() {
  return state->fade_animation_timer == state->fade_animation_duration-1;
}

void user_interface_system_destroy(void) {

}

bool user_interface_on_event(u16 code, event_context context) {
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
    case EVENT_CODE_UI_START_FADEIN_EFFECT: {
      state->fade_animation_duration = context.data.u16[0];
      state->fade_animation_playing = true;
      state->fade_animation_timer = 0;
      state->fadein = true;
      return true;
    }
    case EVENT_CODE_UI_START_FADEOUT_EFFECT: {
      state->fade_animation_duration = context.data.u16[0];
      state->fade_animation_playing = true;
      state->fade_animation_timer = 0;
      state->fadein = false;
      return true;
    }
  };

  return false;
}

#undef PSPRITESHEET_SYSTEM
#undef SPACE_BTW_V
#undef DEFAULT_MENU_BUTTON_SCALE
#undef MENU_BUTTON_FONT
#undef MENU_BUTTON_FONT_SIZE
#undef MINI_BUTTON_FONT
#undef MINI_BUTTON_FONT_SIZE
#undef LABEL_MOOD_FONT_SIZE
#undef LABEL_MOOD_OUTLINE_FONT_SIZE
#undef LABEL_MINI_FONT_SIZE
#undef LABEL_MINI_OUTLINE_FONT_SIZE
#undef draw_text
#undef SDR_CURR_VAL
#undef FADE_ANIMATION_DURATION
#undef SCREEN_POS
