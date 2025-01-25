
#ifndef ABILITY_MANAGER_H
#define ABILITY_MANAGER_H

#include "defines.h"

bool ability_system_initialize(camera_metrics* _camera_metrics, app_settings* settings);

ability get_ability(ability_type _type);
void upgrade_ability(ability* abl, ability_play_system* system);

void update_abilities(ability_play_system* system, Character2D owner);
void render_abilities(ability_play_system* system);


#endif