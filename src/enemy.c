#include "raylib.h"
#include "raymath.h"

#include "header.h"
#include "entity.h"
#include "world.h"
#include "enemy.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// spawns a test wave
void test_spawn_wave(game_world *world) {
    for (int i = 0; i < 12; i++) {
        spawn_enemy(world, TESLA);
    }
    for (int i = 0; i < 0; i++) {
        spawn_enemy(world, CHARGER);
    }
    for (int i = 0; i < 0; i++) {
        spawn_enemy(world, DRONE);
    }
    for (int i = 0; i < 0; i++) {
        spawn_enemy(world, LASER);
    }
    for (int i = 0; i < 0; i++) {
        spawn_enemy(world, BOMBER);
    }
    for (int i = 0; i < 5; i++) {
        spawn_enemy(world, CHASER);
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (world->objects.enemies.list[i].is_active) {
            world->objects.enemies.list[i].id = i;
        }
    }
}

// general function that spawns an enemy with TYPE at location.
// returns a unique int key correlating to the enemy that has just been spawned in case of sub-enemies
int spawn_enemy(game_world *world, int type) {
    assert(world->objects.enemies.count < MAX_ENEMIES && "TOO MANY ENEMIES");

    create_enemy(world, &world->objects.enemies.list[world->objects.enemies.count++], type);
    return world->objects.enemies.enemy_keys - 1;
}

// switches an enemy's active flag and initialises all other values.
void create_enemy(game_world *world, enemy *enemy, enemy_type type) {
    enemy->is_active = true;
    enemy->should_count_towards_kills = true;
    enemy->should_slow_down_when_killed = true;
    enemy->movement.acceleration = Vector2Zero();
    enemy->movement.velocity = Vector2Zero();
    enemy->type = type;
    enemy->key = world->objects.enemies.enemy_keys++;
    enemy->parent_key = INVALID_KEY;

    enemy->hitbox.is_active = true;
    enemy->hitbox.invincible_duration = 0;

    switch (type) {
        case ENEMY_COUNT:
            // Impossible case, used to track number of enemy types
            // Silence compiler warning.
            break;
        case CHASER:
            create_chaser(world, enemy);
            break;
        case BOMBER:
            create_bomber(world, enemy);
            break;
        case LASER:
            create_laser(world, enemy);
            break;
        case DRONE:
            create_drone(world, enemy);
            break;
        case CHARGER:
            create_charger(world, enemy);
            break;
        case CHARGER_WEAKPOINT:
            create_charger_weakpoint(enemy);
            break;
        case TESLA:
            create_tesla(world, enemy);
    }
}

/*
New enemy TODO list:
- add its type to enemy_type enum
- add enemy_state enum
- add enemy_data struct
    - add said struct to enemy struct
- update create_enemy
    - add initialised values for each enemy specific parameter
- every enemy needs its unique:
    - health
    - radius
    - knockback coefficient (only really relevant for hp > 1 enemies but do it anyways)
    add these to enemy.h
- update update_enemies
- add a draw_[enemy] and update update_draw_frame

- create the update_[enemy] function
- this is the STATE MACHINE and controls enemy behaviour.
- add this behaviour
*/

/////////////////////////////// UPDATE LOGIC ///////////////////////////////

void update_enemies(game_world *world, time *time) {
    for (int i = 0; i < MAX_ENEMIES; i++){
        enemy *enemy = &world->objects.enemies.list[i];

        if (!enemy->is_active) {
            continue;
        }

        switch (enemy->type) {
            case ENEMY_COUNT:
                // Impossible case, used to track number of enemy types
                // Silence compiler warning.
                break;
            case CHASER:
                update_chaser(world, enemy, time);
                break;
            case BOMBER:
                update_bomber(world, enemy, time);
                break;
            case LASER:
                update_laser(world, enemy, time);
                break;
            case DRONE:
                update_drone(world, enemy, time);
                break;
            case CHARGER:
                update_charger(world, enemy, time);
                break;
            case CHARGER_WEAKPOINT:
                // silence compiler
                break;
            case TESLA:
                update_tesla(world, enemy, time);
                break;
        }
        update_counter(&enemy->hitbox.invincible_duration, time);
    }

    update_all_drones(world, time);
    tesla_update_all_charge_targets(world);
    kill_enemies(world);
}
/////////////////////////////// TESLA LOGIC ///////////////////////////////

/*
Support enemies that discourage player sticking to walls and stuff.
STALKING:
    Stay still until timer runs out. Surrounded by a field
CHARGING:
    Select a random, non-tesla, non-self, enemy.
    Charge an enemy, giving them a damaging AOE around them.
    Also charge the nearest wall.
    When hit, create a superlarge AOE and stop charging. Re-enter STALKING.

GROUP_IDLE:
    No other members exist.
GROUP_STALKING:
    When more than 1 tesla exist, they will chain together.
    During stalking, wait for a few seconds before forming a chain.
GROUP CHARGING:
    Form links between each other. These lasers flicker.
GROUP_DISCHARGE:
    When any member is hit, directly linked members create a small AoE. Two connected members form a new link and the hit member is disconnected.
*/


// TODO:
// when charge target dies, delete hurtbox and go stalk
// charger also charges sub parts.

// GROUP TESLA ORGY
void update_tesla(game_world *world, enemy *enemy, time *time) {
    switch (enemy->tesla.state) {
        case TESLA_NONE: 
            tesla_enter_stalking(world, enemy);
            break;
        case TESLA_STALKING:
            tesla_exit_stalking(world, enemy, time);
            break;
        case TESLA_CHARGING:
            tesla_exit_charging(world, enemy, time);
            break;
    }
}

void tesla_update_all_charge_targets(game_world *world) {
    for (int i = 0; i < world->objects.enemies.teslas.tesla_count; i++) {
        enemy *tesla = world->objects.enemies.teslas.tesla_list[i];
        enemy *target = get_enemy_from_key(world, tesla->tesla.charging_target_key);
        if (target == NULL) {
            continue;
        }

        hurtbox *hurtbox = get_hurtbox_from_key(world, tesla->tesla.charge_hurtbox_key);
        if (hurtbox == NULL) {
            continue;
        }
        
        hurtbox->circle.centre = target->movement.position;
    }
}

// change state into stalking
void tesla_enter_stalking(game_world *world, enemy *enemy) {
    enemy->tesla.state = TESLA_STALKING;
    enemy->tesla.charging_target_key = INVALID_KEY;
    enemy->tesla.stalking_duration = TESLA_STALKING_DURATION;

    enemy->tesla.charge_hurtbox_key = create_circle_hurtbox(&world->objects, enemy->movement.position, enemy->radius * TESLA_CHARGING_RADIUS_MULTIPLIER, TESLA_ATTACK_TELEGRAPH, TESLA_STALKING_DURATION - TESLA_ATTACK_TELEGRAPH);
}

void tesla_exit_stalking(game_world *world, enemy *enemy, time *time) {
    update_counter(&enemy->tesla.stalking_duration, time);
    if (enemy->tesla.stalking_duration == 0) {
        enemy->tesla.stalking_duration = TESLA_STALKING_DURATION;
        tesla_enter_charging(world, enemy);
    }
}

void tesla_enter_charging(game_world *world, enemy *enemy) {
    enemy->tesla.state = TESLA_CHARGING;
    enemy->tesla.charging_target_key = get_uncharged_enemy_key(world);
    if (enemy->tesla.charging_target_key == INVALID_KEY) {
        return;
    }

    struct enemy *target = get_enemy_from_key(world, enemy->tesla.charging_target_key);
    if (target == NULL) {
        return;
    }
    enemy->tesla.charge_hurtbox_key = create_circle_hurtbox(&world->objects, target->movement.position, target->radius * TESLA_CHARGING_RADIUS_MULTIPLIER, TESLA_CHARGING_TELEGRAPH, TESLA_CHARGING_DURATION - TESLA_CHARGING_TELEGRAPH);
    target->tesla.is_charged = true;
}

void tesla_exit_charging(game_world *world, enemy *enemy, time *time) {
    update_counter(&enemy->tesla.charging_duration, time);
    if (enemy->tesla.charging_duration == 0) {
        enemy->tesla.charging_duration = TESLA_CHARGING_DURATION;
        struct enemy *target = get_enemy_from_key(world, enemy->tesla.charging_target_key); 
        if (target == NULL) {
            tesla_enter_stalking(world, enemy);
            return;
        }
        target->tesla.is_charged = false;
        enemy->tesla.charging_target_key = INVALID_KEY;
        tesla_enter_stalking(world, enemy);
    }
}

// Yeah honestly I'm too lazy to get a better algorithm.
// Time complexity of the search is O(n^2) (actually might be O(n*m)) lmao.
int get_uncharged_enemy_key(game_world *world) {
    tesla_collective_data *teslas = &world->objects.enemies.teslas;
    enemy *enemy_list = world->objects.enemies.list;

    for (int i = 0; i < world->objects.enemies.count; i++) {
        enemy *enemy = &enemy_list[i];
        
        if (!enemy->is_active) {
            continue;
        }

        if (enemy->type == CHARGER_WEAKPOINT) {
            continue;
        }

        if (enemy->type == TESLA) {
            continue;
        }
        
        bool is_valid = true;
        for (int j = 0; j < teslas->tesla_count; j++) {
            if (teslas->tesla_list[j] == NULL) {
                continue;
            }

            if (!teslas->tesla_list[j]->is_active) {
                continue;
            }

            if (teslas->tesla_list[j]->tesla.charging_target_key == enemy->key) {
                is_valid = false;
                break;
            }
        }

        if (is_valid) {
            enemy->tesla.is_charged = true;
            return enemy->key;
        }
    }

    return INVALID_KEY;
}

void create_tesla(game_world *world, enemy *enemy) {
    enemy->hitbox.health = TESLA_HEALTH;
    enemy->movement.position = find_laser_spawn(world);
    enemy->hitbox.circle.centre = enemy->movement.position;
    enemy->hitbox.circle.radius = TESLA_RADIUS;
    enemy->radius = TESLA_RADIUS;
    enemy->knockback_coefficient = TESLA_KNOCKBACK_COEFFECIENT;

    enemy->tesla.state = TESLA_NONE;
    enemy->tesla.stalking_duration = TESLA_STALKING_DURATION;
    enemy->tesla.charging_duration = TESLA_CHARGING_DURATION;
    enemy->tesla.charge_hurtbox_key = INVALID_KEY;
    enemy->tesla.charging_target_key = INVALID_KEY;
    world->objects.enemies.teslas.tesla_list[world->objects.enemies.teslas.tesla_count++] = enemy;
}

void initialise_teslas(tesla_collective_data *teslas) {
    teslas->state = TESLA_GROUP_NONE;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        teslas->tesla_list[i] = NULL;
    }
}

/////////////////////////////// CHARGER LOGIC ///////////////////////////////

/*
Charger has 2 attacks.
It will perform a stomp attack if the player gets too close. No cooldown but can only be performed during stalking.
It will perform a charger, stopping movement, aiming at the player. Charging will have a very low acceleration but 0 resistance.
Becomes stunned after, leaving if it dies.

STALKING:
    Slowly approach the player. Charge cooldown decreases.
    Enter STOMPING if the player gets too close.
    Enter POSITIONING once charge cooldown is 0.

STOMPING:
    Don't move, let momentum carry. Don't decrease charge cooldown. Launch allies but dont hurt them.
    Enter STALKING once attack ends.

POSITIONING:
    Don't move, let momentum carry. Position duration decreases.
    Enter CHARGING when duration is 0

CHARGING:
    Low acceleration but 0 resistance, charge forward until hit a wall.
    Friendly fire during charge. UPDATE CHASERS TO AVOID THIS.
    When a wall is hit, create a hurtbox across the wall and bounce back.
    Enter STUNNED.
STUNNED:
    Stop moving and turning.
    Stay stunned until stun duration is 0 or is hit at all.
    Then enter STALKING.
    Do not STOMP for a short duration.
*/

void update_charger(game_world *world, enemy *enemy, time *time) {
    switch (enemy->charger.state) {
        case CHARGER_STALKING:
            charger_update_stalking(world, enemy, time);
            break;
        case CHARGER_POSITIONING:
            charger_update_positioning(world, enemy, time);
            break;
        case CHARGER_CHARGING:
            charger_update_charging(world, enemy, time);
            break;
        case CHARGER_STUNNED:
            charger_update_stunned(enemy, time);
            break;
        case CHARGER_STOMPING:
            charger_update_stomping(world, enemy, time);
            break;
    }
    update_charger_weakpoints(world, enemy, time);
    charger_heal(enemy);
    clamp_enemy(enemy);
}

void charger_update_stunned(enemy *enemy, time *time) {
    general_movement(&enemy->movement, Vector2Negate(enemy->charger.facing_direction), 0, 5, time);
    
    charger_exit_stunned(enemy, time);
}

void charger_exit_stunned(enemy *enemy, time *time) {
    update_counter(&enemy->charger.stunned_duration, time);
    if (enemy->charger.stunned_duration == 0) {
        enemy->charger.stunned_duration = CHARGER_STUNNED_DURATION;

        enemy->charger.state = CHARGER_STALKING;
    }
}

int get_charger_bounce_count(enemy *enemy) {
    int bounces = CHARGER_MAX_BOUNCES;
    for (int i = 0; i < CHARGER_WEAKPOINT_COUNT; i++) {
        if (enemy->charger.weakpoints[i] == INVALID_KEY) {
            bounces *= CHARGER_MAX_BOUNCE_INCREASE;
        }
    }

    return bounces;
}

void charger_update_charging(game_world *world, enemy *enemy, time *time) {
    general_movement(&enemy->movement, enemy->charger.facing_direction, CHARGER_CHARGING_ACCELERATION, CHARGER_CHARGING_RESISTANCE, time);
    bounce_charger_off_walls(world, enemy);

    charger_exit_charging(enemy);
}

void charger_exit_charging(enemy *enemy) {
    if (enemy->charger.max_bounces <= 0) {
        enemy->charger.facing_direction = Vector2Negate(enemy->charger.facing_direction);

        enemy->charger.state = CHARGER_STUNNED;
    }
}

void bounce_charger_off_walls(game_world *world, enemy *enemy) {
    if (enemy->movement.position.x > MAP_SIZE - enemy->radius) {
        enemy->movement.velocity.x *= -1;
        enemy->charger.facing_direction = Vector2Reflect(enemy->charger.facing_direction, (Vector2){1, 0});
        enemy->charger.max_bounces -= 1;

        Rectangle shape;

        shape.height = 2 * MAP_SIZE; 
        shape.width = CHARGER_WALL_HURTBOX_THICKNESS;
        shape.x = MAP_SIZE - CHARGER_WALL_HURTBOX_THICKNESS;
        shape.y = -MAP_SIZE;

        create_rectangle_hurtbox(&world->objects, shape, 0, CHARGER_WALL_HURTBOX_DURATION);
    }
    
    if (enemy->movement.position.x < -MAP_SIZE + enemy->radius) {
        enemy->movement.velocity.x *= -1;
        enemy->charger.facing_direction = Vector2Reflect(enemy->charger.facing_direction, (Vector2){1, 0});
        enemy->charger.max_bounces -= 1;

        Rectangle shape;

        shape.height = 2 * MAP_SIZE; 
        shape.width = CHARGER_WALL_HURTBOX_THICKNESS;
        shape.x = -MAP_SIZE;
        shape.y = -MAP_SIZE;

        create_rectangle_hurtbox(&world->objects, shape, 0, CHARGER_WALL_HURTBOX_DURATION);
    }
    
    if (enemy->movement.position.y > MAP_SIZE - enemy->radius) {
        enemy->movement.velocity.y *= -1;
        enemy->charger.facing_direction = Vector2Reflect(enemy->charger.facing_direction, (Vector2){0, 1});
        enemy->charger.max_bounces -= 1;

        Rectangle shape;

        shape.height = CHARGER_WALL_HURTBOX_THICKNESS;
        shape.width = 2 * MAP_SIZE;
        shape.x = -MAP_SIZE;
        shape.y = MAP_SIZE - CHARGER_WALL_HURTBOX_THICKNESS;

        create_rectangle_hurtbox(&world->objects, shape, 0, CHARGER_WALL_HURTBOX_DURATION);
    }
    
    if (enemy->movement.position.y < -MAP_SIZE + enemy->radius) {
        enemy->movement.velocity.y *= -1;
        enemy->charger.facing_direction = Vector2Reflect(enemy->charger.facing_direction, (Vector2){0, 1});
        enemy->charger.max_bounces -= 1;

        Rectangle shape;

        shape.height = CHARGER_WALL_HURTBOX_THICKNESS;
        shape.width = 2 * MAP_SIZE;
        shape.x = -MAP_SIZE;
        shape.y = -MAP_SIZE;

        create_rectangle_hurtbox(&world->objects, shape, 0, CHARGER_WALL_HURTBOX_DURATION);
    }
}

void charger_update_positioning(game_world *world, enemy *enemy, time *time) {
    general_movement(&enemy->movement, enemy->movement.velocity, 0, 1.5, time);
    charger_update_facing_direction(world, enemy, time);

    charger_exit_positioning(enemy, time);
}

void charger_exit_positioning(enemy *enemy, time *time) {
    update_counter(&enemy->charger.positioning_duration, time);
    if (enemy->charger.positioning_duration == 0) {
        enemy->charger.positioning_duration = CHARGER_POSITIONING_DURATION;

        enemy->charger.max_bounces = get_charger_bounce_count(enemy);

        enemy->charger.state = CHARGER_CHARGING;
    }
}

void charger_update_stomping(game_world *world, enemy *enemy, time *time) {
    general_movement(&enemy->movement, enemy->movement.velocity, 0, 1.5, time);
    update_stomp_hurtbox(world, enemy);

    update_counter(&enemy->charger.stomping_duration, time);
    if (enemy->charger.stomping_duration == 0) {
        enemy->charger.stomp_key = INVALID_KEY;
        enemy->charger.stomping_duration = CHARGER_STOMPING_DURATION;

        enemy->charger.state = CHARGER_STALKING;
    }
}

void update_stomp_hurtbox(game_world *world, enemy *enemy) {
    hurtbox *hurtbox = get_hurtbox_from_key(world, enemy->charger.stomp_key);
    if (hurtbox == NULL) {
        return;
    }
    hurtbox->circle.centre = enemy->movement.position;
}

void charger_update_stalking(game_world *world, enemy *enemy, time *time) {
    charger_stalking_movement(world, enemy, time);
    charger_update_facing_direction(world, enemy, time);

    charger_exit_stalking(world, enemy, time);
}

void charger_stalking_movement(game_world *world, enemy *enemy, time *time) {
    Vector2 direction = Vector2Subtract(world->player.movement.position, enemy->movement.position);

    if (Vector2Length(direction) < CHARGER_STALKING_RANGE) {
        direction = Vector2Scale(direction, -1);
    }

    direction = Vector2Normalize(direction);

    general_movement(&enemy->movement, direction, CHARGER_STALKING_ACCELERATION, CHARGER_STALKING_RESISTANCE, time);
}

void charger_exit_stalking(game_world *world, enemy *enemy, time *time) {
    if (Vector2Distance(world->player.movement.position, enemy->movement.position) < CHARGER_ENTER_STOMP_RANGE) {
        enemy->charger.stomp_key = create_circle_hurtbox(&world->objects, enemy->movement.position, CHARGER_STOMP_RADIUS, CHARGER_STOMP_TELEGRAPH, CHARGER_STOMP_DURATION);

        enemy->charger.state = CHARGER_STOMPING;
    }

    update_counter(&enemy->charger.stalking_duration, time);
    if (enemy->charger.stalking_duration == 0) {
        enemy->charger.state = CHARGER_POSITIONING;
        enemy->charger.stalking_duration = CHARGER_STALKING_DURATION;
    }
}

void charger_update_facing_direction(game_world *world, enemy *enemy, time *time) {
    // replace with proper angular momentum later.
    enemy->charger.facing_direction = Vector2Normalize(Vector2Subtract(world->player.movement.position, enemy->movement.position));
    (void)time;
}

void charger_heal(enemy *enemy) {
    if (enemy->hitbox.health < CHARGER_HEALTH && enemy->hitbox.health != 0) {
        enemy->hitbox.health = CHARGER_HEALTH;
    }
}

void update_charger_weakpoints(game_world *world, enemy *enemy, time *time) {
    Vector2 charger_butt = Vector2Rotate(enemy->charger.facing_direction, PI);
    Vector2 charger_to_weakpoint = Vector2Rotate(charger_butt, -CHARGER_WEAKPOINT_SEPERATION);
    charger_to_weakpoint = Vector2Scale(charger_to_weakpoint, CHARGER_RADIUS); 
    int dead_weakpoints = 0;
    
    for (int i = 0; i < CHARGER_WEAKPOINT_COUNT; i++) {
        struct enemy *weakpoint = get_enemy_from_key(world, enemy->charger.weakpoints[i]);
        if (weakpoint == NULL) {
            enemy->charger.weakpoints[i] = INVALID_KEY;
        }

        if (enemy->charger.weakpoints[i] == INVALID_KEY) {
            dead_weakpoints++;
            charger_to_weakpoint = Vector2Rotate(charger_to_weakpoint, CHARGER_WEAKPOINT_SEPERATION);
            continue;
        }
        weakpoint->movement.position = Vector2Add(charger_to_weakpoint, enemy->movement.position);
        charger_to_weakpoint = Vector2Rotate(charger_to_weakpoint, CHARGER_WEAKPOINT_SEPERATION);
        update_counter(&weakpoint->hitbox.invincible_duration, time);
    }
    if (dead_weakpoints == CHARGER_WEAKPOINT_COUNT) {
        enemy->hitbox.health = 0;
    }
}

// PIPELINE:
/*
create_charger -> create_charger_weakpoints -> spawn_enemy 3x -> create_charger_weakpoint
(creates the main enemy) -> (helper function to create weakpoints) -> (calls spawn_enemy to create weakpoints) -> (creates enemy)

currently will get bugged because storing a pointer to the enemy will become incorrect when that enemy moves pointer positions due to external kills.
*/
void create_charger(game_world *world, enemy *enemy) {
    enemy->hitbox.health = CHARGER_HEALTH;
    enemy->movement.position = find_charger_spawn(world);
    enemy->hitbox.circle.centre = enemy->movement.position;
    enemy->hitbox.circle.radius = CHARGER_RADIUS;
    enemy->radius = CHARGER_RADIUS;
    enemy->knockback_coefficient = CHARGER_KNOCKBACK_COEFFECIENT;

    enemy->charger.state = CHARGER_STALKING;
    enemy->charger.facing_direction = Vector2Normalize(Vector2Subtract(world->player.movement.position, enemy->movement.position));
    create_charger_weakpoints(world, enemy);
    enemy->charger.stomp_key = INVALID_KEY;
    enemy->charger.stalking_duration = CHARGER_STALKING_DURATION;
    enemy->charger.stomping_duration = CHARGER_STOMPING_DURATION;
    enemy->charger.positioning_duration = CHARGER_POSITIONING_DURATION;
    enemy->charger.max_bounces = CHARGER_MAX_BOUNCES;
    enemy->charger.stunned_duration = CHARGER_STUNNED_DURATION;
}

void create_charger_weakpoints(game_world *world, enemy *enemy) {
    Vector2 charger_butt = Vector2Rotate(enemy->charger.facing_direction, PI);
    Vector2 charger_to_weakpoint = Vector2Rotate(charger_butt, -CHARGER_WEAKPOINT_SEPERATION);
    charger_to_weakpoint = Vector2Scale(charger_to_weakpoint, CHARGER_RADIUS); 
    for (int i = 0; i < CHARGER_WEAKPOINT_COUNT; i++) {
        enemy->charger.weakpoints[i] = spawn_enemy(world, CHARGER_WEAKPOINT);
        struct enemy *weakpoint = get_enemy_from_key(world, enemy->charger.weakpoints[i]);
        assert(weakpoint != NULL && "Null returned in create_charger_weakpoitns");
        weakpoint->movement.position = Vector2Add(charger_to_weakpoint, enemy->movement.position);
        weakpoint->hitbox.circle.centre = weakpoint->movement.position;
        weakpoint->parent_key = enemy->key; 

        charger_to_weakpoint = Vector2Rotate(charger_to_weakpoint, CHARGER_WEAKPOINT_SEPERATION);
    } 
}

void create_charger_weakpoint(enemy *enemy) {
    enemy->should_count_towards_kills = false;
    enemy->should_slow_down_when_killed = false;
    enemy->hitbox.health = CHARGER_WEAKPOINT_HEALTH;
    enemy->hitbox.circle.radius = CHARGER_WEAKPOINT_RADIUS;
    enemy->radius = CHARGER_WEAKPOINT_RADIUS;
    enemy->knockback_coefficient = CHARGER_KNOCKBACK_COEFFECIENT;
}

Vector2 find_charger_spawn(game_world *world) {
    bool is_not_valid = true;

    int safeguard = 0;
    Vector2 position;
    while (is_not_valid == true) {
        position.x = GetRandomValue(-MAP_SIZE + CHARGER_RADIUS, MAP_SIZE - CHARGER_RADIUS);
        position.y = GetRandomValue(-MAP_SIZE + CHARGER_RADIUS, MAP_SIZE - CHARGER_RADIUS);

        enemy test;
        test.movement.position = position;
        test.radius = CHARGER_RADIUS;

        if (is_overlapping_with_other_enemy(world, &test)) {
            is_not_valid = true;
        } else if (Vector2Distance(world->player.movement.position, position) < CHARGER_PLAYER_MIN_SPAWN_DISTANCE) {
            is_not_valid = true;
        } else {
            is_not_valid = false;
        }

        safeguard++;
        if (safeguard > INFINITE_LOOP_GUARD) {
            return Vector2Zero();
        }
    }

    return position;    
}

bool is_overlapping_with_other_enemy(game_world *world, enemy *enemy) {
    for (int i = 0; i < world->objects.enemies.count; i++) {
        struct enemy check = world->objects.enemies.list[i];
        if (CheckCollisionCircles(check.movement.position, check.radius, enemy->movement.position, enemy->radius)) {
            return true;
        }
    }

    return false;
}

/////////////////////////////// DRONE LOGIC ///////////////////////////////
/*
Drones are quick bastards that work in packs.

STALKING:
    Drone will try to hover near the player. Their fast speed mean they will jitter about their tracing point.
    Enter ATTACK_STALKING once stalking_duration becomes 0.

ORBITING:
    Drones will begin rapidly circling the player.
    Their hitboxes are replaced with hurtboxes.
    Will perform a random attack, then return to stalking.
    Will enter an attack once attack cooldown reaches 0.

TRANSLATING:
    Drones will move together in a certain direction, preventing the player from leaving the radius. They will attempt to move to the centre of the map


BARRAGING:
    Drones will telegraph lasers within the circle and quickly attack

GIVE_UP:
    If the last drone left, it will give up and stop fighting, and will shortly self destruct.
*/

// update the state of all drones, as well as collective behaviour.
// update_drone is responsible for the physical movement of each drone.
void update_all_drones(game_world *world, time *time) {
    update_drone_id(world);

    drone_collective_data *drones = &world->objects.enemies.drones;
    if (drones->drone_count == 0) {
        return;
    }

    switch (drones->state) {
        case DRONE_INITIAL:
            drones->stalking_duration = get_drone_stalking_duration(world);
            drones->state = DRONE_STALKING;
            break;
        case DRONE_STALKING:      
            drones_exit_stalking(drones, time);
            break;
        case DRONE_POSITIONING:
            drones_exit_positioning(world, drones);
            break;
        case DRONE_ORBITING:
            drones_update_orbiting(world, drones, time);
            break;
        case DRONE_TRANSLATING:
            drones_update_translating(world, drones, time);
            break;
        case DRONE_BARRAGING:
            drones_update_barraging(world, drones, time);
            break;
        case DRONE_GIVE_UP:
            // silence compiler warning
            break;
    }

    if (drones->drone_count < 3) {
        drones->state = DRONE_GIVE_UP;
    }
}

void drones_exit_stalking(drone_collective_data *drones, time *time) {   
    update_counter(&drones->stalking_duration, time);
    if (drones->stalking_duration == 0) {
        drones->stalking_duration = DRONE_STALKING_DURATION;

        drones->state = DRONE_POSITIONING;
    }
}

void drones_exit_positioning(game_world *world, drone_collective_data *drones) {
    if (is_all_drones_ready(world)) {
        drone_reset_ready(world);
        reset_base_drone_vector(world);
        drones_disable_hitbox(world);

        drones->state = DRONE_ORBITING;
    }
}

void drones_disable_hitbox(game_world *world) {
    drone_collective_data *drones = &world->objects.enemies.drones;
    for (int i = 0; i < drones->drone_count; i++) {
        enemy *enemy = drones->drone_list[i];
        enemy->hitbox.is_active = false;
    }
}

// Drones begins orbiting the player.
void drones_update_orbiting(game_world *world, drone_collective_data *drones, time *time) {
    rotate_base_drone_vector(world, time);
    drones_update_radius(world, drones, time);
    drones_exit_orbiting(world, drones, time);
}

void drones_exit_orbiting(game_world *world, drone_collective_data *drones, time *time) {
    update_counter(&drones->orbit_duration, time);
    if (drones->orbit_duration == 0) {
        drones->orbit_duration = DRONE_ORBIT_DURATION;
        drones->orbit_movement = world->player.movement;

        drones->state = DRONE_TRANSLATING;
    } 
}

// Drones make their way to the centre of the screen while slowly expanding.
void drones_update_translating(game_world *world, drone_collective_data *drones, time *time) {
    rotate_base_drone_vector(world, time);
    drone_translating_orbit_centre_movement(world, time);
    
    drones_update_radius(world, drones, time);

    drones_exit_translating(world, drones);
}

void drones_update_radius(game_world *world, drone_collective_data *drones, time *time) {
    double target_length = get_modified_final_size(world, DRONE_TRANSLATE_RADIUS);
    double current_length = Vector2Length(drones->base_vector);

    if (target_length < current_length) {
        shrink_base_drone_vector(world, time, target_length);
    } else {
        expand_base_drone_vector(world, time, target_length);
    }
}

void drones_exit_translating(game_world *world, drone_collective_data *drones) {
    if (Vector2Distance(drones->orbit_movement.position, Vector2Zero()) > 5) {
        return;
    } else if (!is_close_enough(Vector2Length(drones->base_vector), get_modified_final_size(world, DRONE_TRANSLATE_RADIUS))) {
        return;
    }
    
    drones_replace_hitbox_with_hurtbox(world);
    drones->orbit_movement.position = Vector2Zero();
    drones->barraging_radius = get_modified_final_size(world, DRONE_BASE_BARRAGING_RADIUS);
    drones->barraging_interval_multiplier = get_modified_interval_multiplier(world);
    
    if (get_non_drone_enemy_count(world) == 0) {
        drones_create_big_laser(world);
    } 

    drones->state = DRONE_BARRAGING;
}

void drones_create_big_laser(game_world *world) {
    double angle_divider = get_drone_angle_divider(world);
    double effective_radius = world->objects.enemies.drones.barraging_radius * cosf(angle_divider / 2);

    Vector2 start = (Vector2){0, effective_radius - DRONE_BIG_LASER_THICKNESS};
    Vector2 end = (Vector2){0, -effective_radius + DRONE_BIG_LASER_THICKNESS};

    double random = GetRandomValue(LOWEST_ANGLE, HIGHEST_ANGLE);
    random = degrees_to_radians(random);

    start = Vector2Rotate(start, random);
    end = Vector2Rotate(end, random);

    world->objects.enemies.drones.big_laser_key = create_capsule_hurtbox(&world->objects, start, end, DRONE_BIG_LASER_THICKNESS, DRONE_BIG_LASER_TELEGRAPH_DURATION, ARBITRARY_LARGE_NUMBER);
}

// Drones begin firing lasers. Entered once the drones are centred.
void drones_update_barraging(game_world *world, drone_collective_data *drones, time *time) {
    rotate_base_drone_vector(world, time);
    drones_create_passive_lasers(world, time);
    drones_sync_hurtbox(world);

    if (get_non_drone_enemy_count(world) == 0) {
        update_counter(&drones->big_laser_telegraph_duration, time);
        if (drones->big_laser_telegraph_duration != 0) {
            return;
        } else {
            drones_update_big_laser(world, time);
        }
    }

    drones_fire_active_lasers(world, drones, time);
    drones_exit_barraging(world, drones, time);
}

void drones_update_big_laser(game_world *world, time *time) {
    hurtbox *hurtbox = get_hurtbox_from_key(world, world->objects.enemies.drones.big_laser_key);
    if (hurtbox == NULL) {
        return;
    }

    double acceleration = DRONE_BIG_LASER_ANGULAR_ACCELERATION;
    double *w = &world->objects.enemies.drones.big_laser_angular_velocity;

    update_counter(&world->objects.enemies.drones.big_laser_reverse_timer, time);
    if (world->objects.enemies.drones.big_laser_reverse_timer == 0) {
        acceleration = -*w * DRONE_BIG_LASER_ANGULAR_RESISTANCE;
    } else if (world->objects.enemies.drones.big_laser_reverse_timer <= DRONE_BIG_LASER_TIMER / 2) {
        acceleration *= -1;
    }

    *w += acceleration * time->dt;
    *w = Clamp(*w, -DRONE_BIG_LASER_MAX_ANGULAR_VELOCITY, DRONE_BIG_LASER_MAX_ANGULAR_VELOCITY);

    hurtbox->capsule.start = Vector2Rotate(hurtbox->capsule.start, *w * time->dt);
    hurtbox->capsule.end = Vector2Rotate(hurtbox->capsule.end, *w * time->dt);
}

void drones_fire_active_lasers(game_world *world, drone_collective_data *drones, time *time) {
    // The way C handles floats means that the lower if statement can become untrue.
    // This messes with dt, so leave the update counter outside.
    update_counter(&drones->active_laser_cooldown, time);
    if (drones->min_interval_duration > DRONE_ACTIVE_LASER_DURATION + DRONE_ACTIVE_LASER_TELEGRAPH) {
        drones_create_active_lasers(world);
    }
}

void drones_create_active_lasers(game_world *world) {
    drone_collective_data *drones = &world->objects.enemies.drones;

    if (drones->active_laser_cooldown == 0) {
        drones->active_laser_cooldown = DRONE_ACTIVE_LASER_COOLDOWN * drones->attack_interval_multiplier * drones->barraging_interval_multiplier;
    } else {
        return;
    }

    double angle_divider = get_drone_angle_divider(world);
    double effective_radius = drones->barraging_radius * cosf(angle_divider / 2);

    double start_angle = GetRandomValue(LOWEST_ANGLE, HIGHEST_ANGLE);
    double end_angle;

    int safeguard = 0;

    bool is_end_angle_too_close = true;
    while (is_end_angle_too_close) {
        end_angle = GetRandomValue(LOWEST_ANGLE, HIGHEST_ANGLE);

        if (fabs(end_angle - start_angle) < DRONE_ACTIVE_LASER_MINIMUM_ANGLE_DIFFERENCE) {
            is_end_angle_too_close = true;
        } else if (fabs(end_angle - start_angle + HIGHEST_ANGLE) < DRONE_ACTIVE_LASER_MINIMUM_ANGLE_DIFFERENCE) {
            is_end_angle_too_close = true;
        } else {
            is_end_angle_too_close = false;
        }

        safeguard++;
        if (safeguard > INFINITE_LOOP_GUARD) {
            return;
        }
    }

    start_angle = degrees_to_radians(start_angle);
    end_angle = degrees_to_radians(end_angle);

    Vector2 laser_start = Vector2Polar(effective_radius, start_angle);
    Vector2 laser_end = Vector2Polar(effective_radius, end_angle);

    create_line_hurtbox(&world->objects, laser_start, laser_end, DRONE_ACTIVE_LASER_TELEGRAPH, DRONE_ACTIVE_LASER_DURATION);
    drones->attack_interval_multiplier *= DRONE_ATTACK_INTERVAL_DECREASE;

    double min_attack_interval = DRONE_MIN_ATTACK_INTERVAL_MULTIPLIER * drones->barraging_interval_multiplier;
    double max_attack_interval = DRONE_MAX_ATTACK_INTERVAL_MULTIPLIER * drones->barraging_interval_multiplier;
    drones->attack_interval_multiplier = Clamp(drones->attack_interval_multiplier, min_attack_interval, max_attack_interval);
}

void drones_exit_barraging(game_world *world, drone_collective_data *drones, time *time) {
    if (is_close_enough(drones->attack_interval_multiplier, DRONE_MIN_ATTACK_INTERVAL_MULTIPLIER * drones->barraging_interval_multiplier)) {
        update_counter(&drones->min_interval_duration, time);
        if (drones->min_interval_duration == 0) {
            drones->attack_interval_multiplier = DRONE_MAX_ATTACK_INTERVAL_MULTIPLIER;
            drones->min_interval_duration = DRONE_MIN_ATTACK_DURATION;
            drones_clear_hurtboxes(world);
            if (drones->big_laser_key != INVALID_KEY) {
                remove_hurtbox_from_key(world, drones->big_laser_key);
            }
            drones->big_laser_key = INVALID_KEY;
            drones->big_laser_telegraph_duration = DRONE_BIG_LASER_TELEGRAPH_DURATION;
            drones->big_laser_angular_velocity = 0;
            drones->big_laser_reverse_timer = DRONE_BIG_LASER_TIMER;

            drones->state = DRONE_STALKING;
        }
    }
}

void drones_clear_hurtboxes(game_world *world) {
    drone_collective_data *drones = &world->objects.enemies.drones;
    for (int i = 0; i < drones->drone_count; i++) {
        enemy *enemy = drones->drone_list[i];
        enemy->hitbox.is_active = true;
        hurtbox *hurtbox = get_hurtbox_from_key(world, enemy->drone.hurtbox_key);
        assert(hurtbox != NULL && "Null returned in drones_clear_hurtboxes");
        int hurtbox_index = hurtbox - world->objects.hurtboxes;
        remove_hurtbox(&world->objects, hurtbox_index);
    }
}

void drones_replace_hitbox_with_hurtbox(game_world *world) {
    drone_collective_data *drones = &world->objects.enemies.drones;
    for (int i = 0; i < drones->drone_count; i++) {
        enemy *enemy = drones->drone_list[i];
        enemy->hitbox.is_active = false;
        enemy->drone.hurtbox_key = create_circle_hurtbox(&world->objects, enemy->movement.position, DRONE_RADIUS, 0, ARBITRARY_LARGE_NUMBER);
    }
}

void drones_sync_hurtbox(game_world *world) {
    drone_collective_data *drones = &world->objects.enemies.drones;
    for (int i = 0; i < drones->drone_count; i++) {
        enemy *enemy = drones->drone_list[i];
        hurtbox *hurtbox = get_hurtbox_from_key(world, enemy->drone.hurtbox_key);
        assert(hurtbox != NULL && "NULL returned in drones_sync_hurtbox");
        hurtbox->circle.centre = enemy->movement.position;
    }
}

// Lasers connect a drone to its adjacent drone and fires lasers facing outwards.
void drones_create_passive_lasers(game_world *world, time *time) {
    drone_collective_data *drones = &world->objects.enemies.drones;

    update_counter(&drones->passive_laser_cooldown, time);
    if (drones->passive_laser_cooldown == 0) {
        drones->passive_laser_cooldown = DRONE_PASSIVE_LASER_COOLDOWN;
    } else {
        return;
    }


    for (int i = 0; i < drones->drone_count; i++) {
        struct enemy *enemy = drones->drone_list[i];

        Vector2 direction = Vector2Subtract(enemy->movement.position, drones->orbit_movement.position);
        Vector2 end = Vector2Scale(direction, ARBITRARY_LARGE_NUMBER);
        end = Vector2Add(end, enemy->movement.position);

        create_line_hurtbox(&world->objects, enemy->movement.position, end, 0, DRONE_PASSIVE_LASER_DURATION);

        struct enemy *next_enemy;
        if (i + 1 >= drones->drone_count) {
            next_enemy = drones->drone_list[0];
        } else {
            next_enemy = drones->drone_list[i + 1];
        }
        create_line_hurtbox(&world->objects, enemy->movement.position, next_enemy->movement.position, 0, DRONE_PASSIVE_LASER_DURATION);
    }
}


void drone_translating_orbit_centre_movement(game_world *world, time *time) {
    general_movement(&world->objects.enemies.drones.orbit_movement, Vector2Scale(world->objects.enemies.drones.orbit_movement.position, -1), DRONE_CENTRE_TRANSLATIONAL_ACCELERATION, DRONE_CENTRE_TRANSLATIONAL_RESISTANCE, time);
    world->objects.enemies.drones.orbit_movement.position.x = Clamp(world->objects.enemies.drones.orbit_movement.position.x, -MAP_SIZE + DRONE_CENTRE_TRANSLATION_BUFFER, MAP_SIZE - DRONE_CENTRE_TRANSLATION_BUFFER);
    world->objects.enemies.drones.orbit_movement.position.y = Clamp(world->objects.enemies.drones.orbit_movement.position.y, -MAP_SIZE + DRONE_CENTRE_TRANSLATION_BUFFER, MAP_SIZE - DRONE_CENTRE_TRANSLATION_BUFFER);
}

void rotate_base_drone_vector(game_world *world, time *time) {
    drone_collective_data *drones = &world->objects.enemies.drones;

    drones->angular_velocity += DRONE_ORBITAL_ACCELERATION * time->dt;
    drones->angular_velocity = Clamp(drones->angular_velocity, -DRONE_MAX_ANGULAR_VELOCITY, DRONE_MAX_ANGULAR_VELOCITY);
    drones->base_vector = Vector2Rotate(drones->base_vector, drones->angular_velocity * time->dt);
}

void expand_base_drone_vector(game_world *world, time *time, double final_size) {
    drone_collective_data *drones = &world->objects.enemies.drones;
    Vector2 direction = Vector2Normalize(drones->base_vector);
    drones->base_vector = Vector2Add(drones->base_vector, Vector2Scale(direction, DRONE_BASE_VECTOR_MODULUS_INCREASE_RATE * time->dt));
    
    drones->base_vector = Vector2ClampValue(drones->base_vector, DRONE_MIN_RADIUS, final_size);
}

void shrink_base_drone_vector(game_world *world, time *time, double final_size) {
    drone_collective_data *drones = &world->objects.enemies.drones;
    Vector2 direction = Vector2Normalize(drones->base_vector);
    drones->base_vector = Vector2Subtract(drones->base_vector, Vector2Scale(direction, DRONE_BASE_VECTOR_MODULUS_INCREASE_RATE * time->dt));
    
    drones->base_vector = Vector2ClampValue(drones->base_vector, final_size, DRONE_MAX_RADIUS);
}

double get_modified_final_size(game_world *world, int final_size) {
    final_size -= world->objects.enemies.drones.drone_count * DRONE_RADIUS_DECREASE_PER_DRONE;
    final_size += get_non_drone_enemy_count(world) * DRONE_RADIUS_INCREASE_PER_NOT_DRONE;

    final_size = Clamp(final_size, DRONE_MIN_RADIUS, DRONE_MAX_RADIUS);

    return final_size;
}

double get_modified_interval_multiplier(game_world *world) {
    double multiplier = DRONE_BASE_BARRAGING_INTERVAL_MULTIPLIER;
    multiplier -= world->objects.enemies.drones.drone_count * DRONE_ATTACK_INTERVAL_DECREASE_PER_DRONE;
    multiplier += get_non_drone_enemy_count(world) * DRONE_ATTACK_INTERVAL_INCREASE_PER_NOT_DRONE;

    multiplier = Clamp(multiplier, DRONE_MINIMUM_ATTACK_INTERVAL_MULTIPLIER, DRONE_MAXIMUM_ATTACK_INTERVAL_MULTIPLIER);
    return multiplier;
}

int get_non_drone_enemy_count(game_world *world) {
    int count = 0;
    for (int i = 0; i < world->objects.enemies.count; i++) {
        if (world->objects.enemies.list[i].type != DRONE && world->objects.enemies.list[i].should_count_towards_kills) {
            count++;
        }
    }
    return count;
}

void reset_base_drone_vector(game_world *world) {
    drone_collective_data *drones = &world->objects.enemies.drones;
    drones->angular_velocity = 0;
    drones->base_vector = (Vector2){0, -DRONE_BASE_ORBITING_RADIUS};
}

// update the movement of a single drone
void update_drone(game_world *world, enemy *enemy, time *time) {
    switch (world->objects.enemies.drones.state) {
        case DRONE_STALKING:
            drone_stalking_movement(world, enemy, time);
            break;
        case DRONE_POSITIONING:
            drone_stalking_movement(world, enemy, time);
            break;
        case DRONE_ORBITING:
            drone_attacking_movement(world, enemy);
            break;
        case DRONE_TRANSLATING:
            drone_translating_movement(world, enemy);
            break;
        case DRONE_BARRAGING:
            drone_translating_movement(world, enemy);
            break;
        case DRONE_GIVE_UP:
            drone_give_up(world, enemy, time);
            bounce_enemy_off_walls(enemy);
            break;
        case DRONE_INITIAL:
            // silence compiler warning
            break;
    }

    clamp_enemy(enemy);
}

void drone_give_up(game_world *world, enemy *enemy, time *time) {
    general_movement(&enemy->movement, Vector2Normalize(enemy->movement.velocity), 0, DRONE_GIVE_UP_RESISTANCE, time);
    if (Vector2Length(enemy->movement.velocity) < 1) {
        create_circle_hurtbox(&world->objects, enemy->movement.position, DRONE_EXPLOSION_RADIUS, DRONE_EXPLODE_TELEGRAPH, BOMBER_EXPLOSION_DURATION);
        enemy->hitbox.health = 0;
    }
}

void drone_translating_movement(game_world *world, enemy *enemy) {
    double divider = get_drone_angle_divider(world);
    divider *= enemy->drone.id;

    Vector2 player_to_enemy = Vector2Rotate(world->objects.enemies.drones.base_vector, divider);
    enemy->movement.position = Vector2Add(player_to_enemy, world->objects.enemies.drones.orbit_movement.position);
}

// accelerate. Once at max speed and attack cooldown is at 0, STRIKE!
void drone_attacking_movement(game_world *world, enemy *enemy) {
    double divider = get_drone_angle_divider(world);
    divider *= enemy->drone.id;

    Vector2 player_to_enemy = Vector2Rotate(world->objects.enemies.drones.base_vector, divider);
    enemy->movement.position = Vector2Add(player_to_enemy, world->player.movement.position);
}

void drone_stalking_movement(game_world *world, enemy *enemy, time *time) {
    Vector2 target = get_drone_target(world, enemy);
    Vector2 enemy_to_target = Vector2Subtract(target, enemy->movement.position);

    Vector2 direction = enemy_to_target;

    Vector2 player_to_enemy = Vector2Subtract(enemy->movement.position, world->player.movement.position);
    if (Vector2Length(player_to_enemy) <= DRONE_STALKING_PLAYER_REPULSION_DISTANCE) {
        direction = Vector2Add(Vector2Scale(player_to_enemy, DRONE_STALKING_PLAYER_REPULSION_COEFFICIENT), direction);
        direction = Vector2Normalize(direction);
    }

    if (world->objects.enemies.drones.state == DRONE_STALKING) {
        general_movement(&enemy->movement, direction, PLAYER_ACCELERATION_COE, PLAYER_RESISTANCE_COE, time);
    } else {
       // for positioning, lock when close enough and move faster. Too much hassle to make a new function. 
        if (Vector2Length(enemy_to_target) < DRONE_POSITIONING_LOCK_THRESHOLD) {
            enemy->movement.position = target;
            enemy->drone.is_ready_to_attack = true;
            enemy->movement.velocity = Vector2Zero();
            return;
        } else if (Vector2Length(enemy_to_target) > DRONE_POSITIONING_UNLOCK_THRESHOLD) {
            enemy->drone.is_ready_to_attack = false;

        }
        general_movement(&enemy->movement, direction, DRONE_POSITIONING_ACCELERATION, PLAYER_RESISTANCE_COE, time);
    }
}

Vector2 get_drone_target(game_world *world, enemy *enemy) {
    double angle_divider = get_drone_angle_divider(world);
    Vector2 player_to_target = (Vector2){0, -DRONE_BASE_ORBITING_RADIUS};
    player_to_target = Vector2Rotate(player_to_target, angle_divider * enemy->drone.id);
    Vector2 target = Vector2Add(world->player.movement.position, player_to_target);
    return target;
}

double get_drone_angle_divider(game_world *world) {
    int count = world->objects.enemies.drones.drone_count;
    double angle_divider = FULL_RADIANS / count;
    return angle_divider;
}

void drone_reset_ready(game_world *world) {
    for (int i = 0; i < world->objects.enemies.drones.drone_count; i++) {
        assert(world->objects.enemies.drones.drone_list[i] != NULL);
        world->objects.enemies.drones.drone_list[i]->drone.is_ready_to_attack = false;
    }
}

// Initialises specific values for a single drone.
void create_drone(game_world *world, enemy *enemy) {
    assert(world->objects.enemies.drones.drone_count < MAX_DRONES && "Too many drones dipshit");

    enemy->hitbox.health = DRONE_HEALTH;
    enemy->movement.position = find_chaser_spawn(world);
    enemy->hitbox.circle.centre = enemy->movement.position;
    enemy->hitbox.circle.radius = DRONE_RADIUS;
    enemy->radius = DRONE_RADIUS;
    enemy->knockback_coefficient = DRONE_KNOCKBACK_COEFFICIENT;

    if (world->objects.enemies.drones.drone_count == 0) {
        reset_drones(world);
    }

    enemy->drone.id = world->objects.enemies.drones.drone_count;
    enemy->drone.is_ready_to_attack = false;

    // there is a mistake in this assignment. update_drone_id resolves this mistake. Called during update_drone_state.
    world->objects.enemies.drones.drone_list[enemy->drone.id] = enemy;
    world->objects.enemies.drones.drone_count++;
}

void initialise_drones(drone_collective_data *drones) {
    drones->drone_count = 0;
    drones->state = DRONE_STALKING;
    drones->base_vector = (Vector2){0, -DRONE_BASE_ORBITING_RADIUS};
    drones->angular_velocity = 0;
    drones->passive_laser_cooldown = DRONE_PASSIVE_LASER_COOLDOWN;
    drones->active_laser_cooldown = DRONE_ACTIVE_LASER_COOLDOWN;
    drones->attack_interval_multiplier = DRONE_MAX_ATTACK_INTERVAL_MULTIPLIER;
    drones->min_interval_duration = DRONE_MIN_ATTACK_DURATION;
    drones->barraging_interval_multiplier = DRONE_BASE_BARRAGING_INTERVAL_MULTIPLIER;
    drones->barraging_radius = DRONE_BASE_BARRAGING_RADIUS;
    drones->big_laser_telegraph_duration = DRONE_BIG_LASER_TELEGRAPH_DURATION;
    drones->big_laser_key = INVALID_KEY;
    drones->big_laser_reverse_timer = DRONE_BIG_LASER_TIMER;
    drones->angular_velocity = 0;

    drones->orbit_movement.position = Vector2Zero();
    drones->orbit_movement.velocity = Vector2Zero();
    drones->orbit_movement.acceleration = Vector2Zero();
    for (int i = 0; i < MAX_DRONES; i++) {
        drones->drone_list[i] = NULL;
    }
}

double get_drone_stalking_duration(game_world *world) {
    double duration = DRONE_STALKING_DURATION;
    duration += get_non_drone_enemy_count(world) * 3;
    duration -= world->objects.enemies.drones.drone_count * 2;

    duration = Clamp(duration, 5, 15);

    return duration;
}

// Resets collective drone information if a drone is created while no other drones exist.
void reset_drones(game_world *world) {
    drone_collective_data *drones = &world->objects.enemies.drones; 
    drones->state = DRONE_INITIAL;
    drones->stalking_duration = DRONE_STALKING_DURATION;
    drones->orbit_duration = DRONE_ORBIT_DURATION;
    drones->attack_interval_multiplier = DRONE_MAX_ATTACK_INTERVAL_MULTIPLIER;
}

// Realigns drone specific id to their position in drone_list.
void update_drone_id(game_world *world) {
    int drone_id = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        struct enemy *enemy = &world->objects.enemies.list[i];
        if (!enemy->is_active) {
            continue;
        }

        if (enemy->type != DRONE) {
            continue;
        }

        enemy->drone.id = drone_id;
        world->objects.enemies.drones.drone_list[drone_id] = enemy;

        drone_id++;
    }
}

// Checks if all drones are in position
bool is_all_drones_ready(game_world *world) {
    drone_collective_data drones = world->objects.enemies.drones;
    for (int i = 0; i < drones.drone_count; i++) {

        // debug
        assert(drones.drone_list[i] != NULL);

        if (!drones.drone_list[i]->drone.is_ready_to_attack) {
            return false;
        }
    }

    return true;
}

/////////////////////////////// LASER LOGIC ///////////////////////////////
/*
Lasers are long range and highly dangerous foes. They will try to stay away from the player. 
Then, they will try to predict the player's movement and fire there.

STALKING:
    The laser will try to find a spot on the band from 500 - 750 mapwise.
    The laser will not pick a spot that is within 500 pixels from the player.
    If the spot is within 200 pixels from the player, it will pick another spot.
    Transition into AIMING when attack cooldown reaches 0.
AIMING: 
    The laser will stop accelerating, letting momentum carry them.
    They will aim where the player will be in fire_delay + buffer second.
    This aim should have its own acceleration.
    Transition into FIRING after a few seconds.
FIRING:
    The laser will lock onto its target and stop moving.
    Transition into ATTACK once fire delay is over.
ATTACK:
    activate the hitbox.
    Transition into STALKING after attack finishes.
*/

/*
LASER 2.0


AIMING:
    Now aim directly at the player
    Now movement is based on angular movement
FIRING AND ATTACKING REMAIN THE SAME
HITBOX IS NOW BIIIG
*/

void update_laser(game_world *world, enemy *enemy, time *time) {
    switch (enemy->laser.state) {
        case LASER_STALKING:
            laser_stalking_movement(world, enemy, time);
            update_target_spot(world, enemy);

            if (enemy->laser.attack_cooldown == 0) {
                enemy->laser.state = LASER_AIMING;
                enemy->laser.aim_target.position = world->player.movement.position;
                enemy->laser.attack_cooldown = LASER_ATTACK_COOLDOWN;
            } else if (CheckCollisionPointCircle(enemy->laser.target_position, enemy->movement.position, LASER_RADIUS)) {
                enemy->laser.state = LASER_WAITING;
            }
            break;
        case LASER_WAITING:
            laser_waiting_movement(enemy, time);
            laser_update_attack_cooldown(enemy, time);
            
            if (enemy->laser.attack_cooldown == 0) {
                enemy->laser.state = LASER_AIMING;
                enemy->laser.aim_target.position = world->player.movement.position;
                enemy->laser.attack_cooldown = LASER_ATTACK_COOLDOWN;
            } else if (Vector2Distance(world->player.movement.position, enemy->movement.position) < 300) {
                enemy->laser.state = LASER_STALKING;
            }
            break;
        case LASER_AIMING:
            laser_waiting_movement(enemy, time);
            laser_aiming(world, enemy, time);

            update_counter(&enemy->laser.aim_duration, time);
            if (enemy->laser.aim_duration == 0) {
                enemy->laser.state = LASER_FIRING;
                enemy->laser.aim_duration = LASER_AIM_DURATION;
            }
            break;
        case LASER_FIRING:

            update_counter(&enemy->laser.fire_delay, time);
            if (enemy->laser.fire_delay == 0) {
                enemy->laser.state = LASER_ATTACK;
                enemy->laser.fire_delay = LASER_FIRE_DELAY;
                create_laser_attack(world, enemy);
            }
            break;
        case LASER_ATTACK:
            laser_fire_aiming(world, enemy, time);
            
            update_counter(&enemy->laser.fire_duration, time);
            if (enemy->laser.fire_duration == 0) {
                enemy->laser.state = LASER_STALKING;
                enemy->laser.laser_key = INVALID_KEY;
                enemy->laser.fire_duration = LASER_FIRE_DURATION;
                enemy->laser.target_position = find_laser_target_spot(world, enemy);
                enemy->movement.velocity = Vector2Zero();
            }
    }

    bounce_enemy_off_walls(enemy);
    clamp_enemy(enemy);
}

void create_laser(game_world *world, enemy *enemy) {
    enemy->hitbox.health = LASER_HEALTH;
    enemy->movement.position = find_laser_spawn(world);
    enemy->hitbox.circle.centre = enemy->movement.position;
    enemy->hitbox.circle.radius = LASER_RADIUS;
    enemy->radius = LASER_RADIUS;
    enemy->knockback_coefficient = LASER_KNOCKBACK_COEFFICIENT;

    enemy->laser.state = LASER_STALKING;
    enemy->laser.target_position = find_laser_target_spot(world, enemy);
    enemy->laser.aim_target.acceleration = Vector2Zero();
    enemy->laser.aim_target.velocity = Vector2Zero();
    enemy->laser.aim_target.position = Vector2Zero();
    enemy->laser.laser_key = -1;
    enemy->laser.attack_cooldown = LASER_ATTACK_COOLDOWN;
    enemy->laser.aim_duration = LASER_AIM_DURATION;
    enemy->laser.fire_delay = LASER_FIRE_DELAY;
    enemy->laser.fire_duration = LASER_FIRE_DURATION;
}

void laser_fire_aiming(game_world *world, enemy *enemy, time *time) {
    // Vector2 true_target = find_true_laser_target(world, time);
    Vector2 true_target = world->player.movement.position;
    
    Vector2 direction = Vector2Subtract(true_target, enemy->laser.aim_target.position);
    general_movement(&enemy->laser.aim_target, direction, LASER_FIRE_ACCELERATION, LASER_FIRE_RESISTANCE, time);

    Vector2 AB = Vector2Subtract(enemy->laser.aim_target.position, enemy->movement.position);
    Vector2 C = Vector2Add(enemy->movement.position, Vector2Scale(AB, ARBITRARY_LARGE_NUMBER));
    Vector2 laser_start = Vector2Add(enemy->movement.position, Vector2Scale(Vector2Normalize(AB), LASER_RADIUS * 2));

    hurtbox *laser = get_hurtbox_from_key(world, enemy->laser.laser_key);
    if (laser == NULL) {
        return;
    }

    laser->capsule.start = laser_start;
    laser->capsule.end = C;
}

void create_laser_attack(game_world *world, enemy *enemy) {
    Vector2 AB = Vector2Subtract(enemy->laser.aim_target.position, enemy->movement.position);
    Vector2 C = Vector2Add(enemy->movement.position, Vector2Scale(AB, ARBITRARY_LARGE_NUMBER));
    
    Vector2 laser_start = Vector2Add(enemy->movement.position, Vector2Scale(Vector2Normalize(AB), LASER_RADIUS * 2));
    enemy->laser.laser_key = create_capsule_hurtbox(&world->objects, laser_start, C, LASER_ATTACK_RADIUS, 0, LASER_FIRE_DURATION);
}

void laser_aiming(game_world *world, enemy *enemy, time *time) {
    Vector2 true_target = find_true_laser_target(world);
    // Vector2 true_target = world->player.movement.position;
    
    Vector2 direction = Vector2Subtract(true_target, enemy->laser.aim_target.position);
    general_movement(&enemy->laser.aim_target, direction, LASER_AIM_ACCELERATION, LASER_AIM_RESISTANCE, time);

    if (Vector2Distance(enemy->laser.aim_target.position, true_target) < LASER_AIM_LOCK) {
        enemy->laser.aim_target.position = true_target;
        enemy->laser.aim_target.velocity = Vector2Zero();
        enemy->laser.aim_target.acceleration = Vector2Zero();
    }
}

// where the player will be in a bit.
Vector2 find_true_laser_target(game_world *world) {
    player player = world->player;
    Vector2 true_target;
    Vector2 ut = Vector2Scale(player.movement.velocity, LASER_FIRE_DELAY_BUFFER);
    true_target = ut;
    true_target = Vector2Add(player.movement.position, true_target);
    true_target.x = Clamp(true_target.x, -MAP_SIZE + PLAYER_RADIUS, MAP_SIZE - PLAYER_RADIUS);
    true_target.y = Clamp(true_target.y, -MAP_SIZE + PLAYER_RADIUS, MAP_SIZE - PLAYER_RADIUS);
    return true_target;
}

void laser_update_attack_cooldown(enemy *enemy, time *time) {
    if (enemy->laser.attack_cooldown > 0) {
        enemy->laser.attack_cooldown -= time->dt;
    } else if (enemy->laser.attack_cooldown < 0) {
        enemy->laser.attack_cooldown = 0;
    }
}

void laser_waiting_movement(enemy *enemy, time *time) {
    general_movement(&enemy->movement, Vector2Zero(), 0, LASER_WAITING_RESISTANCE, time);
}

void laser_stalking_movement(game_world *world, enemy *enemy, time *time) {
    Vector2 target_direction = Vector2Subtract(enemy->laser.target_position, enemy->movement.position);

    double distance = Vector2Distance(world->player.movement.position, enemy->movement.position);
    if (distance < 300) {
        general_movement(&enemy->movement, target_direction, LASER_STALKING_FAST_ACCELERATION, LASER_STALKING_RESISTANCE, time);
    } else if (distance < 500) {
        general_movement(&enemy->movement, target_direction, LASER_STALKING_MEDIUM_ACCELERATION, LASER_STALKING_RESISTANCE, time);
    } else {
        general_movement(&enemy->movement, target_direction, LASER_STALKING_SLOW_ACCELERATION, LASER_STALKING_RESISTANCE, time);
    }
}

void update_target_spot(game_world *world, enemy *enemy) {
    double distance = Vector2Distance(world->player.movement.position, enemy->laser.target_position);

    double to_position = Vector2Distance(enemy->movement.position, enemy->laser.target_position);
    double to_player = Vector2Distance(world->player.movement.position, enemy->laser.target_position);

    if (distance < LASER_RUN_FROM_PLAYER_RADIUS || to_player < to_position) {
        enemy->laser.target_position = find_laser_target_spot(world, enemy);
    }
}

Vector2 find_laser_target_spot(game_world *world, enemy *enemy) {
    Vector2 target_candidate;

    int safeguard = 0;
    bool is_target_valid = false;
    while (!is_target_valid) {
        target_candidate.x = GetRandomValue(-MAP_SIZE + LASER_RADIUS, MAP_SIZE - LASER_RADIUS);
        target_candidate.y = GetRandomValue (-MAP_SIZE + LASER_RADIUS, MAP_SIZE - LASER_RADIUS);

        if (fabs(target_candidate.x) > LASER_LOWER_OUTER_SPAWN_BOUNDS || fabs(target_candidate.y) > LASER_LOWER_OUTER_SPAWN_BOUNDS) {
            double to_target = Vector2Distance(target_candidate, enemy->movement.position);
            double to_player = Vector2Distance(target_candidate, world->player.movement.position);

            if (to_player > to_target) {
                is_target_valid = true;
            }
        }


        safeguard++;
        if (safeguard > INFINITE_LOOP_GUARD) {
            return Vector2Zero();
        }
    }

    return target_candidate;
}

Vector2 find_laser_spawn(game_world *world) {
    Vector2 candidate;

    int safeguard = 0;
    bool is_candidate_valid = false;
    while (!is_candidate_valid) {
        candidate.x = GetRandomValue(-MAP_SIZE + LASER_RADIUS, MAP_SIZE - LASER_RADIUS);
        candidate.y = GetRandomValue (-MAP_SIZE + LASER_RADIUS, MAP_SIZE - LASER_RADIUS);

        if (Vector2Distance(world->player.movement.position, candidate) > LASER_PLAYER_DISTANCE_SPAWN_BOUNDS) {
            is_candidate_valid = true;
        }

        safeguard++;
        if (safeguard > INFINITE_LOOP_GUARD) {
            return Vector2Zero();
        }
    }

    return candidate;
}
/////////////////////////////// BOMBER LOGIC ///////////////////////////////
/*
Bombers are heavy units that explode when hit or when they decide its time to kaboom.
They spawn the same as chasers but further away.

STALKING: 
    Slowly move, trying to keep a certain distance away from the player.
    Randomly transition to CHASING. Chance increases drastically if the player is too close.

CHASING:
    Rapidly track the player.
    Transition into ATTACKING when within a certain range or if fuse is too short.

ATTACKING:
    Stop accelerating. Allow momentum to carry them.
    If fuse has more than 1 second left, set it to 1 second.

EXPLODING:
    Explode, dealing 1 damage to EVERYTHING within range.
    Destroy self as well.
*/

void update_bomber(game_world *world, enemy *enemy, time *time) {
    switch (enemy->bomber.state) {
        case BOMBER_STALKING:
            bomber_stalking_movement(world, enemy, time);
            if (should_bomber_chase(world, enemy, time)) {
                enemy->bomber.state = BOMBER_CHASING;
            }
            break;
        case BOMBER_CHASING:
            bomber_chasing_movement(world, enemy, time);
            update_bomber_fuse(enemy, time);
            if (should_bomber_attack(world, enemy)) {
                enemy->bomber.state = BOMBER_ATTACKING;
                if (enemy->bomber.fuse_duration > BOMBER_ATTACKING_FUSE_THRESHOLD) {
                    enemy->bomber.fuse_duration = BOMBER_ATTACKING_FUSE_THRESHOLD;
                }
            }
            break;
        case BOMBER_ATTACKING:
            bomber_attacking_movement(enemy, time);
            update_bomber_fuse(enemy, time);
            if (enemy->bomber.fuse_duration == 0) {
                enemy->bomber.state = BOMBER_EXPLODING;
            }
            break;
        case BOMBER_EXPLODING:
            bomber_explode(world, enemy);
            break;
    }
}

void bomber_explode(game_world *world, enemy *enemy) {
    create_circle_hurtbox(&world->objects, enemy->movement.position, BOMBER_EXPLOSION_RADIUS, 0, BOMBER_EXPLOSION_DURATION);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        struct enemy *victim = &world->objects.enemies.list[i];

        if (!victim->is_active) {
            continue;
        }

        if (!victim->hitbox.is_active) {
            continue;
        }

        if (CheckCollisionCircles(enemy->movement.position, BOMBER_EXPLOSION_RADIUS, victim->hitbox.circle.centre, victim->hitbox.circle.radius)) {
            if (victim->type == BOMBER) {
                chain_bomber(victim);
            } else {
                damage_enemy(&world->objects, &world->player, i, BOMBER_EXPLOSION_DAMAGE);
            }
        }
    }

    enemy->hitbox.health = 0;
}

void chain_bomber(enemy *victim) {
    if (victim->bomber.state != BOMBER_ATTACKING) {
        victim->bomber.state = BOMBER_ATTACKING;
    }
    if (victim->bomber.fuse_duration > BOMBER_CHAIN_FUSE_DURATION) {
        victim->bomber.fuse_duration = BOMBER_CHAIN_FUSE_DURATION;
    }
}

void update_bomber_fuse(enemy *enemy, time *time) {
    if (enemy->bomber.fuse_duration > 0) {
        enemy->bomber.fuse_duration -= time->dt;
    }

    if (enemy->bomber.fuse_duration < 0) {
        enemy->bomber.fuse_duration = 0;
    }
}

// Bomber enters ATTACKING state if they get too close or if fuse is too short.
bool should_bomber_attack(game_world *world, enemy *enemy) {
    if (Vector2Distance(enemy->movement.position, world->player.movement.position) < BOMBER_ATTACKING_RADIUS) {
        return true;
    } else if (enemy->bomber.fuse_duration <= BOMBER_ATTACKING_FUSE_THRESHOLD) {
        return true;
    } else {
        return false;
    }
}

// Bomber chases randomly. Every second, a bomber has a 1/boom_coefficient chance to attack, with chance massively increasing if player is too close.
bool should_bomber_chase(game_world *world, enemy *enemy, time *time) {
    if (enemy->bomber.aggro_check_cooldown < 0) {
        enemy->bomber.aggro_check_cooldown = BOMBER_AGGRO_CHECK_COOLDOWN;
    } else {
        enemy->bomber.aggro_check_cooldown -= time->dt;
        return false;
    }

    int boom_coefficient = BOMBER_CHASE_COEFFICIENT;

    double distance = Vector2Length(Vector2Subtract(enemy->movement.position, world->player.movement.position));

    if (distance < BOMBER_AGGRO_RANGE) {
        boom_coefficient /= BOMBER_AGRRO_COEFFICIENT;
    }

    int boom = GetRandomValue(1, boom_coefficient);
    if (boom == 1) {
        return true;
    }

    return false;
}

// Bomber ceases all external acceleration, travelling a short distance before exploding.
void bomber_attacking_movement(enemy *enemy, time *time) {
    Vector2 resistance = Vector2Scale(enemy->movement.velocity, -BOMBER_ATTACKING_RESISTANCE);

    enemy->movement.acceleration = resistance;
    enemy->movement.velocity = Vector2Add(enemy->movement.velocity, Vector2Scale(enemy->movement.acceleration, time->dt));
    enemy->movement.position = Vector2Add(enemy->movement.position, Vector2Scale(enemy->movement.velocity, time->dt));

    bounce_enemy_off_walls(enemy);
}   

// Bomber rapidly chases the player, with extremely tight movement.
void bomber_chasing_movement(game_world *world, enemy *enemy, time *time) {
    Vector2 direction = Vector2Subtract(world->player.movement.position, enemy->movement.position);
    Vector2 resistance = Vector2Scale(enemy->movement.velocity, -BOMBER_CHASING_RESISTANCE);

    direction = Vector2Normalize(direction);

    enemy->movement.acceleration = Vector2Add(Vector2Scale(direction, BOMBER_CHASING_ACCEL), resistance);
    enemy->movement.velocity = Vector2Add(enemy->movement.velocity, Vector2Scale(enemy->movement.acceleration, time->dt));
    enemy->movement.position = Vector2Add(enemy->movement.position, Vector2Scale(enemy->movement.velocity, time->dt));
    
    enemy->movement.position.x = Clamp(enemy->movement.position.x, -MAP_SIZE + CHASER_RADIUS, MAP_SIZE - CHASER_RADIUS);
    enemy->movement.position.y = Clamp(enemy->movement.position.y, -MAP_SIZE + CHASER_RADIUS, MAP_SIZE - CHASER_RADIUS);
}   

void collide_bomber(enemy *enemy, player *player) {
    enemy->hitbox.is_active = false;
    if (enemy->bomber.fuse_duration > BOMBER_ATTACKING_FUSE_THRESHOLD) {
        enemy->bomber.fuse_duration = BOMBER_ATTACKING_FUSE_THRESHOLD;
    }

    enemy->movement.velocity = player->movement.velocity;
    
    enemy->bomber.state = BOMBER_ATTACKING;
}

// Bomber stalks the player, trying to stay a consistent distance away.
void bomber_stalking_movement(game_world *world, enemy *enemy, time *time) {
    Vector2 direction = Vector2Subtract(world->player.movement.position, enemy->movement.position);
    Vector2 resistance = Vector2Scale(enemy->movement.velocity, -BOMBER_STALKING_RESISTANCE);

    if (Vector2Length(direction) < BOMBER_STALKING_RANGE) {
        direction = Vector2Scale(direction, -1);
    }

    direction = Vector2Normalize(direction);

    enemy->movement.acceleration = Vector2Add(Vector2Scale(direction, BOMBER_STALKING_ACCEL), resistance);
    enemy->movement.velocity = Vector2Add(enemy->movement.velocity, Vector2Scale(enemy->movement.acceleration, time->dt));
    enemy->movement.position = Vector2Add(enemy->movement.position, Vector2Scale(enemy->movement.velocity, time->dt));

    enemy->movement.position.x = Clamp(enemy->movement.position.x, -MAP_SIZE + BOMBER_RADIUS, MAP_SIZE - BOMBER_RADIUS);
    enemy->movement.position.y = Clamp(enemy->movement.position.y, -MAP_SIZE + BOMBER_RADIUS, MAP_SIZE - BOMBER_RADIUS);    
}   

// find a valid place to spawn a chaser. This location cannot be inside a wall, closer than 100 pixels to the player, and closer than 50 pixels to another enemy.
// Otherwise, it is random
Vector2 find_bomber_spawn(game_world *world) {
    bool is_not_valid = true;
    Vector2 position;

    int safeguard = 0;
    while (is_not_valid == true) {
        position.x = GetRandomValue(-MAP_SIZE + BOMBER_RADIUS, MAP_SIZE - BOMBER_RADIUS);
        position.y = GetRandomValue(-MAP_SIZE + BOMBER_RADIUS, MAP_SIZE - BOMBER_RADIUS);

        is_not_valid = is_spawn_too_close(world, position);

        if (Vector2Distance(position, world->player.movement.position) < BOMBER_SPAWN_DISTANCE) {
            is_not_valid = true;
        }

        safeguard++;
        if (safeguard > INFINITE_LOOP_GUARD) {
            return Vector2Zero();
        }
    }

    return position;
}

void create_bomber(game_world *world, enemy *enemy) {
    enemy->hitbox.health = BOMBER_HEALTH;
    enemy->movement.position = find_bomber_spawn(world);
    enemy->hitbox.circle.centre = enemy->movement.position;
    enemy->hitbox.circle.radius = BOMBER_RADIUS;
    enemy->radius = BOMBER_RADIUS;
    enemy->knockback_coefficient = BOMBER_KNOCKBACK_COEFFICIENT;

    enemy->bomber.fuse_duration = BOMBER_FUSE_DURATION;
    enemy->bomber.state = BOMBER_STALKING;
    enemy->bomber.aggro_check_cooldown = 0;   
}

/////////////////////////////// CHASER LOGIC ///////////////////////////////
/*
Quick and mobile, with a dangerous dash they use to slam into the player.

STALKING: 
    Chaser will stay a large radius, orbiting in a circle around the player.
    They will recalculate this radius if they are too close to the player.
    Will transition into FLANKING when attack cooldown is 0.
SCATTERING: 
    If a chaser is within an attacking bomber's radius, it will try to flee.
    Any stage except ATTACKING can transition into this stage if they are too close to a bomber.
FLANKING: 
    Chaser will rapidly approach the player.
    Will transition into ATTACKING when within a certain range.
ATTACKING:
    Dash with incredible velocity. Then, return to STALKING.

*/
void update_chaser(game_world *world, enemy *enemy, time *time) {
    switch (enemy->chaser.state) {
        case CHASER_STALKING:
            chaser_stalking_movement(enemy, &world->player, time);
            update_chaser_attack_cooldown(enemy, time);

            if (is_chaser_within_explosion_range(world, enemy)) {
                enemy->chaser.state = CHASER_SCATTERING;
            }

            if (enemy->chaser.attack_cooldown == 0) {
                enemy->chaser.state = CHASER_FLANKING;
                enemy->chaser.stalking_point = get_chaser_flanking_point(enemy, &world->player);
                enemy->chaser.attack_cooldown = CHASER_ATTACK_COOLDOWN;
            }
            break;
        case CHASER_SCATTERING:
            chaser_scattering_movement(enemy, world, time);

            if (!is_chaser_within_explosion_range(world, enemy)) {
                enemy->chaser.state = CHASER_STALKING;
            }
            break;
        case CHASER_FLANKING:
            chaser_flanking_movement(enemy, &world->player, time);

            if (is_chaser_within_explosion_range(world, enemy)) {
                enemy->chaser.state = CHASER_SCATTERING;
            }
            
            double distance_to_player = Vector2Length(Vector2Subtract(enemy->movement.position, world->player.movement.position));
            if (distance_to_player < CHASER_ATTACKING_RADIUS) {
                enemy->chaser.state = CHASER_ATTACKING;
                start_enemy_dash(world, enemy);
            }
            break;
        case CHASER_ATTACKING:
            chaser_attacking_movement(enemy, time);
            update_enemy_dash(enemy, time);
            
            if (enemy->chaser.is_attacking == false) {
                enemy->chaser.state = CHASER_STALKING;
                enemy->chaser.stalking_point = get_chaser_stalking_point(enemy, &world->player);
            }
            break;
    }

    clamp_enemy(enemy);
}

void chaser_scattering_movement(enemy *enemy, game_world *world, time *time) {
    Vector2 escape_direction = escape_bombers_range(world, enemy);
    general_movement(&enemy->movement, escape_direction, CHASER_SCATTERING_ACCELERATION, CHASER_SCATTERING_RESISTANCE, time);
}

Vector2 escape_bombers_range(game_world *world, enemy *enemy) {
    Vector2 escape_vector = Vector2Zero();
    for (int i = 0; i < MAX_ENEMIES; i++) {
        struct enemy *potential_danger = &world->objects.enemies.list[i];
        if (!potential_danger->is_active) {
            continue;
        } else if (potential_danger->type != BOMBER) {
            continue;
        } else if (potential_danger->bomber.state == BOMBER_STALKING) {
            continue;
        }

        Vector2 direction = Vector2Subtract(enemy->movement.position, potential_danger->movement.position);
        float distance = Vector2Length(direction);
        
        if (potential_danger->bomber.state == BOMBER_CHASING) {
            if (distance <= CHASER_CHASING_BOMBER_SAFETY_RADIUS) {
                escape_vector = Vector2Add(escape_vector, direction);
            }
        } else if (potential_danger->bomber.state == BOMBER_ATTACKING) {
            if (distance <= CHASER_ATTACKING_BOMBER_SAFETY_RADIUS) {
                escape_vector = Vector2Add(escape_vector, direction);
            }
        }
    }
    return escape_vector;
}

bool is_chaser_within_explosion_range(game_world *world, enemy *enemy) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        struct enemy *potential_danger = &world->objects.enemies.list[i];
        if (!potential_danger->is_active) {
            continue;
        } else if (potential_danger->type != BOMBER) {
            continue;
        } else if (potential_danger->bomber.state == BOMBER_STALKING) {
            continue;
        }

        float distance = Vector2Distance(potential_danger->movement.position, enemy->movement.position);
        if (potential_danger->bomber.state == BOMBER_CHASING) {
            float distance = Vector2Distance(potential_danger->movement.position, enemy->movement.position);
            if (distance <= CHASER_CHASING_BOMBER_SAFETY_RADIUS) {
                return true;
            }
        } else if (potential_danger->bomber.state == BOMBER_ATTACKING) {
            if (distance <= CHASER_ATTACKING_BOMBER_SAFETY_RADIUS) {
                return true;
            }
        }
    }

    return false;
}

void chaser_attacking_movement(enemy *enemy, time *time) {
    general_movement(&enemy->movement, enemy->chaser.dash_velocity, CHASER_ATTACKING_ACCELERATION, CHASER_ATTACKING_RESISTANCE, time);
}

void chaser_flanking_movement(enemy *enemy, player *player, time *time) {
    enemy->chaser.stalking_point = Vector2Rotate(enemy->chaser.stalking_point, CHASER_FLANKING_ORBITAL_VELOCITY * time->dt);
    Vector2 relative_stalking_point = Vector2Add(enemy->chaser.stalking_point, player->movement.position);

    double stalking_point_distance = Vector2Length(Vector2Subtract(relative_stalking_point, enemy->movement.position));
    double player_distance = Vector2Length(Vector2Subtract(enemy->movement.position, player->movement.position));

    if (stalking_point_distance > player_distance) {
        enemy->chaser.stalking_point = get_chaser_flanking_point(enemy, player);
        relative_stalking_point = Vector2Add(enemy->chaser.stalking_point, player->movement.position);
    }

    Vector2 direction = Vector2Subtract(relative_stalking_point, enemy->movement.position);
    general_movement(&enemy->movement, direction, CHASER_FLANKING_ACCELERATION, CHASER_FLANKING_RESISTANCE, time);
}

// returns a vector of length 50 in the same direction as the enemy from the player.
Vector2 get_chaser_flanking_point(enemy *enemy, player *player) {
    Vector2 direction = Vector2Subtract(enemy->movement.position, player->movement.position);
    direction = Vector2Normalize(direction);
    direction = Vector2Scale(direction, CHASER_FLANKING_ORBITAL_RADIUS);
    return direction;
}

void chaser_stalking_movement(enemy *enemy, player  *player, time *time) {
    enemy->chaser.stalking_point = Vector2Rotate(enemy->chaser.stalking_point, CHASER_STALKING_ORBITAL_VELOCITY * time->dt);
    Vector2 relative_stalking_point = Vector2Add(enemy->chaser.stalking_point, player->movement.position);

    double stalking_point_distance = Vector2Length(Vector2Subtract(relative_stalking_point, enemy->movement.position));
    double player_distance = Vector2Length(Vector2Subtract(enemy->movement.position, player->movement.position));

    if (stalking_point_distance > player_distance) {
        enemy->chaser.stalking_point = get_chaser_stalking_point(enemy, player);
        relative_stalking_point = Vector2Add(enemy->chaser.stalking_point, player->movement.position);
    }

    Vector2 direction = Vector2Subtract(relative_stalking_point, enemy->movement.position);
    general_movement(&enemy->movement, direction, CHASER_STALKING_ACCELERATION, CHASER_STALKING_RESISTANCE, time);
}

// returns a vector of length 200 - 400 in the same direction as the enemy from the player.
Vector2 get_chaser_stalking_point(enemy *enemy, player *player) {
    Vector2 direction = Vector2Subtract(enemy->movement.position, player->movement.position);
    direction = Vector2Normalize(direction);
    direction = Vector2Scale(direction, GetRandomValue(CHASER_STALKING_ORBITAL_UPPER_RADIUS, CHASER_STALKING_ORBITAL_LOWER_RADIUS));
    return direction;
}

void update_enemy_dash(enemy *enemy, time *time) {
    enemy->movement.position = Vector2Add(enemy->movement.position, Vector2Scale(enemy->chaser.dash_velocity, time->dt));
    enemy->chaser.dash_time_left -= time->dt;
    if (enemy->chaser.dash_time_left <= 0.0){
        enemy->chaser.dash_time_left = 0.0;
        enemy->chaser.is_attacking = false;
    }
}

// find a valid place to spawn a chaser. This location cannot be inside a wall, closer than 100 pixels to the player, and closer than 50 pixels to another enemy.
// Otherwise, it is random
Vector2 find_chaser_spawn(game_world *world) {
    bool is_not_valid = true;

    int safeguard = 0;
    Vector2 position;
    while (is_not_valid == true) {
        position.x = GetRandomValue(-MAP_SIZE + CHASER_RADIUS, MAP_SIZE - CHASER_RADIUS);
        position.y = GetRandomValue(-MAP_SIZE + CHASER_RADIUS, MAP_SIZE - CHASER_RADIUS);

        is_not_valid = is_spawn_too_close(world, position);

        safeguard++;
        if (safeguard > INFINITE_LOOP_GUARD) {
            return Vector2Zero();
        }
    }

    return position;
}

void update_chaser_attack_cooldown(enemy *enemy, time *time) {
    if (enemy->chaser.attack_cooldown > 0) {
        enemy->chaser.attack_cooldown -= time->dt;
    } else if (enemy->chaser.attack_cooldown < 0) {
        enemy->chaser.attack_cooldown = 0;
    }
}

void start_enemy_dash(game_world *world, enemy *enemy) {
    Vector2 direction = Vector2Subtract(world->player.movement.position, enemy->movement.position);
    direction = Vector2Normalize(direction);

    enemy->chaser.is_attacking = true;
    enemy->chaser.dash_time_left = CHASER_DASH_DURATION;
    enemy->chaser.dash_velocity = Vector2Scale(direction, CHASER_DASH_SPEED);
    
    enemy->chaser.attack_cooldown = CHASER_ATTACK_COOLDOWN;
}

void create_chaser(game_world *world, enemy *enemy) {
    enemy->hitbox.health = CHASER_HEALTH;
    enemy->movement.position = find_chaser_spawn(world);
    enemy->hitbox.circle.centre = enemy->movement.position;
    enemy->hitbox.circle.radius = CHASER_RADIUS;
    enemy->radius = CHASER_RADIUS;
    enemy->knockback_coefficient = CHASER_KNOCKBACK_COEFFECIENT;

    enemy->chaser.state = CHASER_STALKING;
    enemy->chaser.stalking_point = get_chaser_stalking_point(enemy, &world->player);
    enemy->chaser.attack_cooldown = CHASER_ATTACK_COOLDOWN;
    enemy->chaser.dash_time_left = 0;
    enemy->chaser.dash_velocity = Vector2Zero();
    enemy->chaser.is_attacking = false;
}

//////////////////////// miscellaneous helpers ////////////////////////

// locks enemy inside the map confines.
void clamp_enemy(enemy *enemy) {
    enemy->movement.position.x = Clamp(enemy->movement.position.x, -MAP_SIZE + enemy->radius, MAP_SIZE - enemy->radius);
    enemy->movement.position.y = Clamp(enemy->movement.position.y, -MAP_SIZE + enemy->radius, MAP_SIZE - enemy->radius);
}

void update_enemy_invincibility(enemy *enemy, time *time) {
    enemy->hitbox.invincible_duration -= time->dt;
    if (enemy->hitbox.invincible_duration < 0) {
        enemy->hitbox.invincible_duration = 0;
    }
}

void general_movement(movement_data *movement, Vector2 direction, double acceleration_coe, double resistance_coe, time *time) {
    Vector2 resistance = Vector2Scale(movement->velocity, -resistance_coe);
    direction = Vector2Normalize(direction);

    movement->acceleration = Vector2Add(Vector2Scale(direction, acceleration_coe), resistance);
    movement->velocity = Vector2Add(movement->velocity, Vector2Scale(movement->acceleration, time->dt));
    movement->position = Vector2Add(movement->position, Vector2Scale(movement->velocity, time->dt));
}

// rotates some "vector" according to angular velocity, acceleration, and min/max stats.
void angular_movement(Vector2 *vector, double *angular_velocity, double acceleration, double min, double max, time *time) {
    *angular_velocity += acceleration * time->dt;
    *angular_velocity = Clamp(*angular_velocity, min, max);
    *vector = Vector2Rotate(*vector, *angular_velocity * time->dt);
}

// check if a spawn is too close to the player or another enemy
bool is_spawn_too_close(game_world *world, Vector2 position) {
    if (Vector2Distance(world->player.movement.position, position) <= 300) {
        return true;
    }
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!world->objects.enemies.list[i].is_active) {
            continue;
        }

        Vector2 test_position = world->objects.enemies.list[i].movement.position;
        if (Vector2Distance(test_position, position) <= 100) {
            return true;
        }
    }

    return false;
}

// decrements something by dt, sets to 0 once it hits 0.
void update_counter(double *counter, time *time) {
    if (*counter > 0) {
        *counter -= time->dt;
    }
    
    if (*counter < 0) {
        *counter = 0;
    }
}

void bounce_enemy_off_walls(enemy *enemy) {
    if (enemy->movement.position.x > MAP_SIZE - enemy->radius) {
        enemy->movement.velocity.x *= -1.2;
    }
    
    if (enemy->movement.position.x < -MAP_SIZE + enemy->radius) {
        enemy->movement.velocity.x *= -1.2;
    }
    
    if (enemy->movement.position.y > MAP_SIZE - enemy->radius) {
        enemy->movement.velocity.y *= -1.2;
    }
    
    if (enemy->movement.position.y < -MAP_SIZE + enemy->radius) {
        enemy->movement.velocity.y *= -1.2;
    }
}

////////////// MORE UPDATES ///////////////////

void update_hitboxes(game_world *world) {
    world_objects *objects = &world->objects;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!objects->enemies.list[i].hitbox.is_active) {
            continue;
        }

        update_hitbox_position(&objects->enemies.list[i]);
    }
}

void update_hitbox_position(enemy *enemy) {
    enemy->hitbox.circle.centre = enemy->movement.position;
}

// kills an enemy by copying the last indexed entry over the killed index, then deactivating the last indexed entry.
// kill_[enemy] functions perform their type check in their functions
void kill_enemy(game_world *world, int kill_index) {
    enemies *enemies = &world->objects.enemies;
    assert(enemies->list[kill_index].is_active && "kill index is inactive");
    if (enemies->list[kill_index].should_count_towards_kills) {
        world->player.kills++; 
    }

    kill_laser(world, kill_index);
    kill_drone(world, kill_index);
    kill_charger_weakpoint(world, kill_index);
    kill_tesla(world, kill_index);

    enemies->count--;
    enemies->list[kill_index] = enemies->list[enemies->count];
    enemies->list[enemies->count].is_active = false;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies->list[i].id = i;
    }
}

void kill_charger_weakpoint(game_world *world, int kill_index) {
    enemy *victim = &world->objects.enemies.list[kill_index];
    if (victim->type != CHARGER_WEAKPOINT) {
        return;
    }

    enemy *parent = get_enemy_from_key(world, victim->parent_key);
    assert(parent != NULL && "weakpoint parent key misaligned");
    for (int i = 0; i < CHARGER_WEAKPOINT_COUNT; i++) {
        enemy *weakpoint = get_enemy_from_key(world, parent->charger.weakpoints[i]);
        if (weakpoint == NULL) {
            continue;
        }
        weakpoint->hitbox.invincible_duration = CHARGER_WEAKPOINT_INVINCIBILITY_DURATION;
        if (parent->charger.state == CHARGER_STUNNED) {
            parent->charger.stunned_duration = 0;
        }
    }
}

void kill_laser(game_world *world, int kill_index) {
    enemies *enemies = &world->objects.enemies;
    if (enemies->list[kill_index].type == LASER && enemies->list[kill_index].laser.state == LASER_ATTACK) {
        hurtbox *laser = get_hurtbox_from_key(world, enemies->list[kill_index].laser.laser_key);
        if (laser == NULL) {
            return;
        }

        remove_hurtbox(&world->objects, laser - world->objects.hurtboxes);
    }
}

void kill_drone(game_world *world, int kill_index) {
    enemies *enemies = &world->objects.enemies;
    if (enemies->list[kill_index].type == DRONE) {
        int remove_id = enemies->list[kill_index].drone.id;
        enemies->drones.drone_list[remove_id] = enemies->drones.drone_list[--enemies->drones.drone_count];
        enemies->drones.drone_list[enemies->drones.drone_count] = NULL;

        update_drone_id(world);
    }
}

void kill_tesla(game_world *world, int kill_index) {
    enemies *enemies = &world->objects.enemies;
    if (enemies->list[kill_index].type == TESLA) {
        enemy *dead_enemy = &enemies->list[kill_index];
        enemy *target = get_enemy_from_key(world, dead_enemy->tesla.charging_target_key);
        if (target != NULL) {
            target->tesla.is_charged = false;
        }
        remove_hurtbox_from_key(world, dead_enemy->tesla.charge_hurtbox_key);
        int index_in_tesla_list = INVALID_INDEX;
        for (int i = 0; i < enemies->teslas.tesla_count; i++) {
            if (enemies->teslas.tesla_list[i]->key == dead_enemy->key) {
                index_in_tesla_list = i;
                break;
            }
        }
        assert(index_in_tesla_list != INVALID_INDEX && "Tesla not in list");
        enemies->teslas.tesla_list[index_in_tesla_list] = enemies->teslas.tesla_list[--enemies->teslas.tesla_count];
        enemies->teslas.tesla_list[enemies->teslas.tesla_count] = NULL;
    } else if (enemies->list[kill_index].tesla.is_charged) {
        enemies->list[kill_index].tesla.is_charged = false;
        int killed_key = enemies->list[kill_index].key;
        enemy *tesla = NULL;
        for (int i = 0; i < enemies->teslas.tesla_count; i++) {
            if (enemies->teslas.tesla_list[i]->tesla.charging_target_key == killed_key) {
                tesla = enemies->teslas.tesla_list[i];
                break;
            }
        }
        assert(tesla != NULL && "Tesla charge owner not found.");
        remove_hurtbox_from_key(world, tesla->tesla.charge_hurtbox_key);
        tesla_enter_stalking(world, tesla);
    }
}

void damage_enemy(world_objects *objects, player *player, int enemy_id, int damage) {
    enemy *enemy = &objects->enemies.list[enemy_id];

    if (!enemy->is_active) {
        return;
    }

    if (!enemy->hitbox.is_active) {
        return;
    }

    if (enemy->hitbox.invincible_duration > 0) {
        return;
    }

    if (enemy->type == BOMBER && player->dash.is_dashing) {
        collide_bomber(enemy, player);
    } else {
        enemy->hitbox.health -= damage;
        enemy->hitbox.invincible_duration = ENEMY_INVINCIBLE_DURATION;
    }

    if (player->dash.is_dashing && CheckCollisionCircles(player->movement.position, PLAYER_RADIUS, enemy->movement.position, enemy->radius)) {
        enemy->movement.velocity = Vector2Scale(player->movement.velocity, enemy->knockback_coefficient);
    }
}

void kill_enemies(game_world *world) {
    enemies *enemies = &world->objects.enemies;
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies->list[i].is_active) {
            continue;
        }

        if (enemies->list[i].hitbox.health <= 0) {
            
            kill_enemy(world, i);
            i--;
        }
    }

}

double degrees_to_radians(double degree) {
    return degree * PI / 180;
}

// returns a polar vector, with 0 degrees as the positive x axis.
Vector2 Vector2Polar(double mod, double arg) {
    Vector2 v = (Vector2){1, 0};
    v = Vector2Scale(v, mod);
    v = Vector2Rotate(v, arg);
    return v;
}

// double comparison
bool is_close_enough(double x, double y) {
    if (fabs(x - y) < EPSILON) {
        return true;
    } else {
        return false;
    }
}

void remove_hurtbox_from_key(game_world *world, int key) {
    hurtbox *hurtbox = get_hurtbox_from_key(world, key);
    if (hurtbox == NULL) {
        return;
    }
    remove_hurtbox(&world->objects, get_hurtbox_index_in_array(world->objects.hurtboxes, hurtbox));
}

int get_hurtbox_index_in_array(hurtbox hurtboxes[MAX_HURTBOXES], hurtbox *hurtbox) {
    if (hurtbox == NULL) {
        return -1;
    }
    return hurtbox - hurtboxes;
}

enemy *get_enemy_from_key(game_world *world, int key) {
    for (int i = 0; i < world->objects.enemies.count; i++) {
        enemy *enemy = &world->objects.enemies.list[i];
        if (enemy->key == key) {
            return enemy;
        }
    }
    return NULL;
}


///////////////////////////// HURTBOXES //////////////////////

// returns a pointer to a specific hurtbox given its unique integer key.
hurtbox *get_hurtbox_from_key(game_world *world, int key) {
    for (int i = 0; i < MAX_HURTBOXES; i++) {
        if (world->objects.hurtboxes[i].key == key) {
            return &world->objects.hurtboxes[i];
        }
    }
    return NULL;
}

// create_hurtbox (<- ctrl + f slave)

// returns a unique key to the owner if the hurtbox needs to be adjusted after activation.
int create_line_hurtbox(world_objects *objects, Vector2 start, Vector2 target, double telegraph_duration, double duration) {
    hurtbox hurtbox;
    hurtbox.is_active = true;
    hurtbox.shape = LINE;
    hurtbox.telegraph_duration = telegraph_duration;
    hurtbox.duration = duration;
    hurtbox.key = objects->hurtbox_keys;
    hurtbox.line.start = start;
    hurtbox.line.end = target;
    if (objects->hurtbox_count < MAX_HURTBOXES) {
        objects->hurtboxes[objects->hurtbox_count++] = hurtbox;
    }

    return objects->hurtbox_keys++;
}

// returns a unique key to the owner if the hurtbox needs to be adjusted after activation.
int create_circle_hurtbox(world_objects *objects, Vector2 centre, double radius, double telegraph_duration, double duration) {
    hurtbox hurtbox;
    hurtbox.is_active = true;
    hurtbox.shape = CIRCLE;
    hurtbox.telegraph_duration = telegraph_duration;
    hurtbox.duration = duration;
    hurtbox.key = objects->hurtbox_keys;
    hurtbox.circle.centre = centre;
    hurtbox.circle.radius = radius;
    if (objects->hurtbox_count < MAX_HURTBOXES) {
        objects->hurtboxes[objects->hurtbox_count++] = hurtbox;
    }

    return objects->hurtbox_keys++;
}

// returns a unique key to the owner if the hurtbox needs to be adjusted after activation.
int create_rectangle_hurtbox(world_objects *objects, Rectangle rectangle, double telegraph_duration, double duration) {
    hurtbox hurtbox;
    hurtbox.is_active = true;
    hurtbox.shape = RECTANGLE;
    hurtbox.telegraph_duration = telegraph_duration;
    hurtbox.duration = duration;
    hurtbox.key = objects->hurtbox_keys;
    hurtbox.rectangle = rectangle;
    if (objects->hurtbox_count < MAX_HURTBOXES) {
        objects->hurtboxes[objects->hurtbox_count++] = hurtbox;
    }

    return objects->hurtbox_keys++;
}

// returns a unique key to the owner if the hurtbox needs to be adjusted after activation.
int create_capsule_hurtbox(world_objects *objects, Vector2 start, Vector2 end, double radius, double telegraph_duration, double duration) {
    hurtbox hurtbox;
    hurtbox.is_active = true;
    hurtbox.shape = CAPSULE;
    hurtbox.telegraph_duration = telegraph_duration;
    hurtbox.duration = duration;
    hurtbox.key = objects->hurtbox_keys;
    hurtbox.capsule.start = start;
    hurtbox.capsule.end = end;
    hurtbox.capsule.radius = radius;
    if (objects->hurtbox_count < MAX_HURTBOXES) {
        objects->hurtboxes[objects->hurtbox_count++] = hurtbox;
    }

    return objects->hurtbox_keys++;
}