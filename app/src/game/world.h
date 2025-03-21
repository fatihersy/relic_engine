
#ifndef WORLD_H
#define WORLD_H

#include "defines.h"

bool world_system_initialize(camera_metrics* _in_camera_metrics, Vector2 resolution_div2);

void set_worldmap_location(u16 id);
worldmap_stage* get_active_worldmap(void);
void set_map_tile(i32 layer, tilemap_tile* src, tilemap_tile* dst);
worldmap_stage* get_worldmap_locations(void);
tilemap* get_active_map(void);
tilemap_prop* get_map_prop_by_pos(Vector2 pos);
tilemap_prop* get_map_prop_by_id(u16 id);

void save_current_map(void);
void load_current_map(void);

tilemap_tile _get_tile_from_sheet_by_mouse_pos();
tilemap_tile _get_tile_from_map_by_mouse_pos(u16 from_layer);

bool add_prop_curr_map(tilemap_prop* prop);
bool remove_prop_cur_map_by_id(u16 id);
void update_map(void);
void drag_tilesheet(Vector2 vec);
void _render_tile(tilemap_tile* tile);
void render_map();
void render_map_view_on(Vector2 pos, f32 zoom);
void render_map_palette(f32 zoom);


#endif
