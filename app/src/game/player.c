#include "player.h"

#include "core/event.h"
#include "core/fmemory.h"

#include "game/ability.h"
#include "game/game_manager.h"


// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL+1];
static player_state* player;

#define PSPRITESHEET_SYSTEM player
#include "game/spritesheet.h"

void play_anim(spritesheet_type player_anim_sheet);

bool player_system_initialized = false;
bool player_system_on_event(u16 code, void* sender, void* listener_inst, event_context context);

bool player_system_initialize() {
    if (player_system_initialized) return false;

    player = (player_state*)allocate_memory_linear(sizeof(player_state), true);

    if (!player) {
        TraceLog(LOG_FATAL, "PLAYER_SYSTEM ALLOCATION FAILED");
        return false;
    }

    event_register(EVENT_CODE_PLAYER_ADD_EXP, 0, player_system_on_event);
    event_register(EVENT_CODE_PLAYER_SET_POSITION, 0, player_system_on_event);
    event_register(EVENT_CODE_PLAYER_DEAL_DAMAGE, 0, player_system_on_event);

    player->position.x = 0;
    player->position.y = 0;
    player->dimentions = (Vector2) {86, 86};
    player->dimentions_div2 = (Vector2) {player->dimentions.x/2, player->dimentions.y/2};

    player->ability_system = ability_system_initialize(PLAYER, player->dimentions);

    player->collision = (Rectangle)
    {
        .x = player->position.x - player->dimentions.x / 2.f,
        .y = player->position.y - player->dimentions.y / 2.f,
        .width = player->dimentions.x,
        .height = player->dimentions.y
    };

    //add_ability(&player->ability_system, FIREBALL);
    // add_ability(ability_system, salvo);
    // add_ability(ability_system, radiation);
    // add_ability(ability_system, direct_fire);

    player->is_dead = false;
    player->is_moving = false;
    player->w_direction = LEFT;
    player->is_damagable = true;
    player->damage_break_time = .2; //ms
    player->damage_break_current = player->damage_break_time;

    player->level = 1;
    player->exp_to_next_level = level_curve[player->level];
    player->exp_current = 0;
    player->health_max = 255;
    player->health_current = player->health_max;

    player->move_left_sprite_queue_index = register_sprite(PLAYER_ANIMATION_MOVE_LEFT, SCENE_IN_GAME, false, true);
    player->move_right_sprite_queue_index = register_sprite(PLAYER_ANIMATION_MOVE_RIGHT, SCENE_IN_GAME, false, true);
    player->idle_left_sprite_queue_index = register_sprite(PLAYER_ANIMATION_IDLE_LEFT, SCENE_IN_GAME, false, true);
    player->idle_right_sprite_queue_index = register_sprite(PLAYER_ANIMATION_IDLE_RIGHT, SCENE_IN_GAME, false, true);
    player->take_damage_left_sprite_queue_index = register_sprite(PLAYER_ANIMATION_TAKE_DAMAGE_LEFT, SCENE_IN_GAME, false, true);
    player->take_damage_right_sprite_queue_index = register_sprite(PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT, SCENE_IN_GAME, false, true);
    player->wreck_left_sprite_queue_index = register_sprite(PLAYER_ANIMATION_WRECK_LEFT, SCENE_IN_GAME, false, true);
    player->wreck_right_sprite_queue_index = register_sprite(PLAYER_ANIMATION_WRECK_RIGHT, SCENE_IN_GAME, false, true);
    player->last_played_sprite_id = player->idle_left_sprite_queue_index; // The position player starts. To avoid from the error when move firstly called
    
    player->initialized = true;
    player_system_initialized = true;
    return true;
}

player_state* get_player_state() {
    if (!player_system_initialized) {
        return (player_state*)0;
    }
    //TraceLog(LOG_INFO, "player.position {x:%d, y:%d}", player->position.x, player->position.y);
    return player;
}

void add_exp_to_player(u32 exp) {
    u32 curr = player->exp_current;
    u32 to_next = player->exp_to_next_level;

    if ( curr + exp >= to_next) 
    {
        player->exp_current = (curr + exp) - to_next;
        player->level++;
        player->exp_to_next_level = level_curve[player->level];
        player->player_have_skill_points = true;
        //play_sprite_on_player(player->);
    }
    else {
        player->exp_current += exp;
    }
}

bool update_player() {
    if (!player_system_initialized) return false;
    if (player->is_dead) {
        event_fire(EVENT_CODE_PAUSE_GAME, 0, (event_context){0});
    }
    player->scene_data = get_current_scene_type();
    if(!player->is_damagable) {
        if(player->damage_break_current - GetFrameTime() > 0) player->damage_break_current -= GetFrameTime();
        else
        {
            player->damage_break_current = player->damage_break_time;
            player->is_damagable = true;
        }
    }

    if (IsKeyDown(KEY_W)) {
        player->position.y -= 2;
        player->is_moving = true;
    }
    if (IsKeyDown(KEY_A)) {
        player->position.x -= 2;
        player->w_direction = LEFT;
        player->is_moving = true;
    }
    if (IsKeyDown(KEY_S)) {
        player->position.y += 2;
        player->is_moving = true;
    }
    if (IsKeyDown(KEY_D)) {
        player->position.x += 2;
        player->w_direction = RIGHT;
        player->is_moving = true;
    }
    if (IsKeyUp(KEY_W) && IsKeyUp(KEY_A) && IsKeyUp(KEY_S) && IsKeyUp(KEY_D)) {    
        player->is_moving = false;
    }

    player->collision.x = player->position.x - player->dimentions.x / 2.f;
    player->collision.y = player->position.y - player->dimentions.y / 2.f;
    player->collision.width = player->dimentions.x;
    player->collision.height = player->dimentions.y;

    update_abilities(&player->ability_system, player->position);
    update_sprite_renderqueue();

    return true;
}

bool render_player() {
    if (!player_system_initialized) { return false; }

    if(!player->is_dead) {
        if(player->is_damagable){
            if(player->is_moving) switch (player->w_direction) 
            {
                case LEFT: play_anim(PLAYER_ANIMATION_MOVE_LEFT);
                break;
                case RIGHT: play_anim(PLAYER_ANIMATION_MOVE_RIGHT);
                break;
            }
            else switch (player->w_direction) 
            {
                case LEFT:play_anim(PLAYER_ANIMATION_IDLE_LEFT);
                break;
                case RIGHT:play_anim(PLAYER_ANIMATION_IDLE_RIGHT);
                break;
            }
        }
        else{
            (player->w_direction == LEFT) 
                ? play_anim(player->take_damage_left_sprite_queue_index)
                : play_anim(player->take_damage_right_sprite_queue_index);
        }
    }
    else {
        (player->w_direction == LEFT) 
            ? play_anim(player->wreck_left_sprite_queue_index)
            : play_anim(player->wreck_right_sprite_queue_index);
    }
    
    render_abilities(&player->ability_system);
    render_sprite_renderqueue();
    #if DEBUG_COLLISIONS
        DrawRectangleLines(
            player->collision.x,
            player->collision.y,
            player->collision.width,
            player->collision.height,
            HITE);
    #endif

    return true;
}

void play_anim(spritesheet_type player_anim_sheet) {
    stop_sprite(player->last_played_sprite_id, false);
    Rectangle dest = (Rectangle) {
        .x = player->position.x,
        .y = player->position.y,
        .width = player->dimentions.x,
        .height = player->dimentions.y
    };
    switch (player_anim_sheet) {
        case SPRITESHEET_UNSPECIFIED: TraceLog(LOG_ERROR, "ERROR::player::move()::move function called with unspecified value.");
        break;
        case BUTTON_REFLECTION_SHEET: TraceLog(LOG_ERROR, "ERROR::player::move()::move function called with wrong value.");
        break;
        case BUTTON_CRT_SHEET: TraceLog(LOG_ERROR, "ERROR::player::move()::move function called with wrong value.");
        break;
        case LEVEL_UP_SHEET: TraceLog(LOG_ERROR, "ERROR::player::move()::move function called with wrong value.");
        break;
        case SPRITESHEET_TYPE_MAX: TraceLog(LOG_ERROR, "ERROR::player::move()::move function called with wrong value.");
        break;

        case PLAYER_ANIMATION_MOVE_LEFT: {
            play_sprite_on_site(player->move_left_sprite_queue_index, dest);
            player->last_played_sprite_id = player->move_left_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_MOVE_RIGHT: {
            play_sprite_on_site(player->move_right_sprite_queue_index, dest);
            player->last_played_sprite_id = player->move_right_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_IDLE_LEFT:  {
            play_sprite_on_site(player->idle_left_sprite_queue_index, dest);
            player->last_played_sprite_id = player->idle_left_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_IDLE_RIGHT:  {
            play_sprite_on_site(player->idle_right_sprite_queue_index, dest);
            player->last_played_sprite_id = player->idle_right_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_TAKE_DAMAGE_LEFT:  {
            play_sprite_on_site(player->take_damage_left_sprite_queue_index, dest);
            player->last_played_sprite_id = player->take_damage_left_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT:  {
            play_sprite_on_site(player->take_damage_right_sprite_queue_index, dest);
            player->last_played_sprite_id = player->take_damage_right_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_WRECK_LEFT:  {
            play_sprite_on_site(player->wreck_left_sprite_queue_index, dest);
            player->last_played_sprite_id = player->wreck_left_sprite_queue_index;
            break;
        }
        case PLAYER_ANIMATION_WRECK_RIGHT:  {
            play_sprite_on_site(player->wreck_right_sprite_queue_index, dest);
            player->last_played_sprite_id = player->wreck_right_sprite_queue_index;
            break;
        }
    }
}

bool player_system_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_PLAYER_ADD_EXP: {
            add_exp_to_player(context.data.u32[0]);
            return true;
        }
        case EVENT_CODE_PLAYER_SET_POSITION: {
            player->position.x = context.data.f32[0]; 
            player->position.y = context.data.f32[1]; 
            player->collision.x = player->position.x; 
            player->collision.y = player->position.y; 
            return true;
        }
        case EVENT_CODE_PLAYER_DEAL_DAMAGE: {
            if(!player->is_damagable) return true;
            if(player->health_current - context.data.u8[0] > 0)
            {
                player->health_current -= context.data.u8[0];
                player->is_damagable = false;

                return true;
            }
            player->health_current = 0;
            player->is_dead = true;
            return true;
        }
        default: return false; // TODO: Warn
    }

    return false;
}



#undef PSPRITESHEET_SYSTEM

