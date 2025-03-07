
#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include "defines.h"

void user_interface_system_initialize(void);

void update_user_interface(void);
void render_user_interface(void);

Font* ui_get_font(font_type font);
panel get_default_panel(void);
data_pack* get_slider_current_value(slider_id id);
bool is_ui_fade_anim_complete();
bool is_ui_fade_anim_about_to_complete();

bool gui_slider_add_option(slider_id _id, const char* _display_text, data_pack content);

bool gui_menu_button(const char* text, button_id _id, Vector2 offset);
bool gui_mini_button(const char* text, button_id _id, Vector2 offset, f32 offset_scale);
bool gui_slider_button(button_id _id, Vector2 pos);
void gui_slider(slider_id _id, Vector2 pos, Vector2 offset, f32 offset_scale);
void gui_draw_texture_to_background(texture_id _id);
void gui_draw_spritesheet_to_background(spritesheet_id _id, Color _tint);
void gui_progress_bar(progress_bar_id bar_id, Vector2 pos, bool _should_center);
void gui_panel(panel pan, Rectangle dest, bool _should_center);
bool gui_panel_active(panel* panel, Rectangle dest, bool _should_center);
void gui_label(const char* text, font_type type, i32 font_size, Vector2 position, Color tint, bool _should_center);

void gui_draw_pause_screen(void);
void gui_draw_texture_id_pro(texture_id _id, Rectangle src, Rectangle dest); 
void gui_draw_texture_id(texture_id _id, Rectangle dest); 
void gui_draw_spritesheet_id(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, u16 frame, bool _should_center); 
void gui_draw_texture_id_center(texture_id _id, Vector2 pos, Vector2 dim, bool should_center); 

#define gui_label_format(FONT, FONT_SIZE, X,Y, COLOR, CENTER, TEXT, ...) gui_label(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, (Vector2){X,Y}, COLOR, CENTER)
#define gui_label_format_v(FONT, FONT_SIZE, VEC, COLOR, CENTER, TEXT, ...) gui_label(TextFormat(TEXT, __VA_ARGS__), FONT, FONT_SIZE, VEC, COLOR, CENTER)

#define gui_panel_scissored(PANEL, CENTER, CODE)        \
    gui_panel(PANEL, PANEL.dest, CENTER);                           \
    BeginScissorMode(PANEL.dest.x, PANEL.dest.y, PANEL.dest.width, PANEL.dest.height);\
    CODE                                                      \
    EndScissorMode();

void user_interface_system_destroy(void);

#endif
