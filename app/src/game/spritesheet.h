
#ifndef SPRITESHEET_H
#define SPRITESHEET_H

#include "defines.h"
#include "raylib.h"

#if defined(PSPRITESHEET_SYSTEM) 
#define register_sprite(_type, _play_looped, _play_once, _center_sprite) \
    _register_sprite(&PSPRITESHEET_SYSTEM, _type, _play_looped, _play_once, _center_sprite)
#define update_sprite_renderqueue() \
    _update_sprite_renderqueue(&PSPRITESHEET_SYSTEM)
#define play_sprite_on_site(_id, dest, _tint) \
    _play_sprite_on_site(&PSPRITESHEET_SYSTEM, _id, _tint, dest)
#define draw_sprite_on_site(_type, _tint, pos, scale, frame, _should_center) \
    _draw_sprite_on_site(_type, _tint, pos, scale, frame, _should_center)
#define queue_sprite_change_location(queue_index, _location) \
    _queue_sprite_change_location(&PSPRITESHEET_SYSTEM, queue_index, _location)
#define stop_sprite(index, reset) \
    _stop_sprite(&PSPRITESHEET_SYSTEM, index, reset)
#define reset_sprite(_queue_index, _retrospective) \
    _reset_sprite(&PSPRITESHEET_SYSTEM, _queue_index, _retrospective)
#define is_sprite_playing(index) \
    _is_sprite_playing(&PSPRITESHEET_SYSTEM, index)
#define get_texture_by_enum(_type) \
    _get_texture_by_enum(_type)
#define get_spritesheet_by_enum(_type) \
    _get_spritesheet_by_enum(_type)
#define rs_path(_path) \
    _rs_path(_path)
#endif

u16 _register_sprite(spritesheet_play_system *system, spritesheet_id _id, bool _play_looped, bool _play_once, bool _center_sprite);
void _update_sprite_renderqueue(spritesheet_play_system *system);
Texture2D* _get_texture_by_enum(texture_id _id);
spritesheet _get_spritesheet_by_enum(spritesheet_id _id);
const char *_rs_path(const char *_path);
void _play_sprite_on_site(spritesheet_play_system *system, u16 _id, Color _tint, Rectangle dest);
void _draw_sprite_on_site(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, u16 frame, bool _should_center);
void _queue_sprite_change_location(spritesheet_play_system *system, u16 queue_index, Rectangle _location);
void _stop_sprite(spritesheet_play_system *system, u16 index, bool reset);
void _reset_sprite(spritesheet_play_system *system, u16 _queue_index, bool _retrospective);
bool _is_sprite_playing(spritesheet_play_system *system, u16 index);

#endif
