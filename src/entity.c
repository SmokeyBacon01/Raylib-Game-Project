#include "raylib.h"
#include "raymath.h"

#include "header.h"
#include "entity.h"
#include "world.h"
#include "enemy.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>


/*
TODO:

- update into a state based system (DONE)
- write new movement functions for each state (DONE)
- write collision for each state <- YOU ARE HERE
- write damage into these new states

- ensure no bugs have leaked through the cracks.

*/

void update_player(game_world *world, time *time) {
    update_player_movement(world, time);
}

void update_player_movement(game_world *world, time *time) {
    player *player = &world->player;
    switch (player->movement_state) {
        case NORMAL:
            normal_player_movement(world, time);
            collide_player_world(&world->player);
            normal_player_hitbox_collision(world);
            dash_zoom(player, *time, world);

            update_counter(&player->dash.cooldown, time);
            if (should_player_dash(world->player)) {
                player->movement_state = MANUAL_DASH;
                time->zoom_change_start = time->total;
                begin_player_dash(world);
            }
            break;
        case MANUAL_DASH:
            dash_player_movement(world, time);
            collide_player_world(&world->player);
            dash_player_hitbox_collision(world);
            dash_zoom(player, *time, world);
            
            update_counter(&player->dash.duration, time);
            if (player->dash.duration == 0 && player->slow_duration == 0) {
                player->movement_state = NORMAL;
                time->zoom_change_start = time->total;
                player->dash.is_dashing = false;
            }

            if (should_player_dash(world->player)) {
                player->movement_state = MANUAL_DASH;
                begin_player_dash(world);
            }
            break;
        case INVOLUNTARY_DASH:
            // cannot redash, slower, does not trigger time slows, can crouch.

            dash_player_movement(world, time);
            collide_player_world(&world->player);
            dash_player_hitbox_collision(world);
            
            update_counter(&player->dash.duration, time);
            if (player->dash.duration == 0) {
                player->movement_state = NORMAL;
                time->zoom_change_start = time->total;
                player->dash.is_dashing = false;
            }
            break;
    }

    update_player_health(&world->objects, player, *time);
    update_counter(&player->invincible_duration, time);
}

// THIS FUNCTION IS TOO LONG, FIX THIS SHIT.
void dash_player_hitbox_collision(game_world *world) {
    for (int i = 0; i < world->objects.enemies.count; i++) {
        struct enemy *enemy = &world->objects.enemies.list[i];
        if (!enemy->is_active) {
            continue;
        }

        if (!enemy->hitbox.is_active) {
            continue;
        }

        if (CheckCollisionCircles(world->player.movement.position, PLAYER_DASH_RADIUS, enemy->hitbox.circle.centre, enemy->hitbox.circle.radius)) {
            damage_enemy(&world->objects, &world->player, i, PLAYER_DASH_DAMAGE);

            if (enemy->type == BOMBER) {
                collide_bomber(enemy, &world->player);
            }

            if (enemy->hitbox.health > 0) {
                launch_player(&world->player, enemy->movement.position);
            } else if (world->player.movement_state == MANUAL_DASH) {
                if (world->player.slow_duration == 0) {
                    world->time->time_change_start = world->time->total;
                }

                if (enemy->should_slow_down_when_killed) {
                    world->player.slow_duration = SLOW_DURATION;
                    world->player.dash.cooldown = 0;
                }
            }
        }
    }
}

void normal_player_hitbox_collision(game_world *world) {
    if (world->player.invincible_duration > 0) {
        return;
    }

    for (int i = 0; i < world->objects.enemies.count; i++) {
        struct enemy *enemy = &world->objects.enemies.list[i];
        if (!enemy->is_active) {
            continue;
        }

        if (!enemy->hitbox.is_active) {
            continue;
        }

        if (CheckCollisionCircles(world->player.movement.position, PLAYER_RADIUS, enemy->hitbox.circle.centre, enemy->hitbox.circle.radius)) {

            if (enemy->type == BOMBER && enemy->bomber.state != BOMBER_STALKING) {
                // exploding bombers should not interact with normal player hitbox in any way.
                // hitbox exists solely for dashing interactions.
                continue;
            }

            damage_player(world, 1);
            if (enemy->hitbox.invincible_duration < ENEMY_INVINCIBLE_DURATION) {
                enemy->hitbox.invincible_duration = ENEMY_INVINCIBLE_DURATION;
            }

            // state update occurs here.
            launch_player(&world->player, enemy->movement.position);
        }
    }
}

void damage_player(game_world *world, int damage) {
    player *player = &world->player;

    if (player->invincible_duration > 0) {
        return;
    } else {
        player->health -= damage;
        player->invincible_duration = INVINCIBILITY_DURATION;
    }
}

void dash_player_movement(game_world *world, time *time) {
    player *player = &world->player;
    Vector2 direction = player->dash.direction;
    double dash_acceleration = PLAYER_ACCELERATION_COE * DASH_ACCELERATION_COE;
    double dash_resistance = PLAYER_RESISTANCE_COE * DASH_RESISTANCE_COE;

    if (world->player.movement_state == INVOLUNTARY_DASH) {
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            dash_resistance *= 2;
        }
        general_movement(&player->movement, direction, dash_acceleration, dash_resistance, time);
    } else {
        general_movement(&player->movement, direction, dash_acceleration, dash_resistance, time);
    }
}


void begin_player_dash(game_world *world) {
    struct dash *dash = &world->player.dash;

    if (dash->is_dashing) {
        extend_dash(&world->player);
    }
    dash->is_dashing = true;
    dash->direction = get_input_acceleration();
    dash->duration = DASH_DURATION;
    dash->cooldown = DASH_COOLDOWN;

    world->player.slow_duration = 0;
}

void normal_player_movement(game_world *world, time *time) {
    player *player = &world->player;
    Vector2 direction = get_input_acceleration();

    double resistance = PLAYER_RESISTANCE_COE;
    
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        resistance *= 2;
    }
    
    general_movement(&player->movement, direction, PLAYER_ACCELERATION_COE, resistance, time);
}

void initialise_hurtboxes(world_objects *objects) {
    objects->hurtbox_count = 0;
    for (int i = 0; i < MAX_HURTBOXES; i++) {
        objects->hurtboxes[i].is_active = false;
    }
}

// Decrement all timers. If it becomes negative or 0, set it to 0, make inactive, and move last active hurtbox into that slot.
void update_hurtboxes(world_objects *objects, time time) {
    for (int i = 0; i < objects->hurtbox_count; i++) {
        if (!objects->hurtboxes[i].is_active) {
            continue;
        }

        if (objects->hurtboxes[i].telegraph_duration != 0) {
            update_counter(&objects->hurtboxes[i].telegraph_duration, &time);
            continue;
        }

        update_counter(&objects->hurtboxes[i].duration, &time);
        if (objects->hurtboxes[i].duration == 0) {
            remove_hurtbox(objects, i);
            i--;
        }
    }
}

// Hurtbox at the last index (now 1 more than last index) still retains old values.
// Treat these values as garbage, never access a hurtbox if it is inactive.
void remove_hurtbox(world_objects *objects, int i) {
    if (i == -1) {
        return;
    }
    objects->hurtboxes[i] = objects->hurtboxes[objects->hurtbox_count - 1];
    objects->hurtboxes[--objects->hurtbox_count].is_active = false;
}

// ret true if any hurtbox collision
bool check_player_hurtbox_collision(world_objects objects, player player, time time) {
    for (int i = 0; i < objects.hurtbox_count; i++) {
        hurtbox hurtbox = objects.hurtboxes[i];
        if (!hurtbox.is_active) {
            continue;
        }

        if (hurtbox.telegraph_duration != 0) {
            continue;
        }

        if (hurtbox.shape == LINE) {
            if (CheckCollisionCircleLine(player.movement.position, PLAYER_RADIUS, hurtbox.line.start, hurtbox.line.end)) {
                return true;
            }

            Vector2 travel_path = Vector2Subtract(player.movement.position, Vector2Add(player.movement.position, Vector2Scale(player.movement.velocity, time.dt)));
            Vector2 player_path = Vector2Add(travel_path, player.movement.position);

            if (CheckCollisionLines(player.movement.position, player_path, hurtbox.line.start, hurtbox.line.end, NULL)) {
                return true;
            }

        } else if (hurtbox.shape == CIRCLE) {
            if (CheckCollisionCircles(player.movement.position, PLAYER_RADIUS, hurtbox.circle.centre, hurtbox.circle.radius)) {
                return true;
            }
        } else if (hurtbox.shape == RECTANGLE) {
            if (CheckCollisionCircleRec(player.movement.position, PLAYER_RADIUS, hurtbox.rectangle)) {
                return true;
            }
        } else if (hurtbox.shape == CAPSULE) {
            if (check_collision_capsule_circle(player.movement.position, PLAYER_RADIUS, hurtbox.capsule)) {
                return true;
            }
        }
    }
    return false;
}

bool check_collision_capsule_circle(Vector2 centre, double radius, capsule_data capsule) {
    Vector2 AP = Vector2Subtract(centre, capsule.start);
    Vector2 AB = Vector2Subtract(capsule.end, capsule.start);

    double t = Vector2DotProduct(AP, AB) / Vector2DotProduct(AB, AB);
    t = Clamp(t, 0, 1);

    // M is closest point
    Vector2 OM = Vector2Add(capsule.start, Vector2Scale(AB, t));
    double distance = Vector2Distance(OM, centre);

    if (distance < radius + capsule.radius) {
        return true;
    }

    return false;
}

// Deals with collisions, currently prevents player from leaving bounds.
void collide_player_world(player *player) {
    if (player->movement.position.x > MAP_SIZE - PLAYER_RADIUS) {
        player->movement.position.x = MAP_SIZE - PLAYER_RADIUS;
        bounce_player_world(player, Y);
    }
    
    if (player->movement.position.x < -MAP_SIZE + PLAYER_RADIUS) {
        player->movement.position.x = -MAP_SIZE + PLAYER_RADIUS;
        bounce_player_world(player, Y);
    }
    
    if (player->movement.position.y > MAP_SIZE - PLAYER_RADIUS) {
        player->movement.position.y = MAP_SIZE - PLAYER_RADIUS;
        bounce_player_world(player, X);
    }
    
    if (player->movement.position.y < -MAP_SIZE + PLAYER_RADIUS) {
        player->movement.position.y = -MAP_SIZE + PLAYER_RADIUS;
        bounce_player_world(player, X);
    }
}

// Bounces player.
void bounce_player_world(player *player, enum axis direction) {
    float bounciness = DASH_BOUNCINESS;

    if (!player->dash.is_dashing) {
        bounciness = NOT_DASHING_BOUNCE_COE * DASH_BOUNCINESS;
     }
    
    
    if (direction == X) {
        player->movement.velocity.y *= -bounciness;
        player->dash.direction.y *= -1;
    } 
    else {
        player->movement.velocity.x *= -bounciness;
        player->dash.direction.x *= -1;
    }
    
    extend_dash(player);
}

void extend_dash (player *player) {
    player->dash.duration *= BOUNCE_EXTENSION_COE;

    if (player->dash.duration > DASH_DURATION) {
        player->dash.duration = DASH_DURATION;
    }   
}

void dash_zoom(player *player, time time, game_world *world) { //implement accelration for camera zoom

    if (player->dash.is_dashing) {
        double t = time.total - time.zoom_change_start;
        world->camera.zoom = MAX_CAMERA_ZOOM + (DEFAULT_CAMERA_ZOOM - MAX_CAMERA_ZOOM) * exp(-CAMERA_ZOOM_TAU*t);
    } else {
        double t = time.total - time.zoom_change_start;
        world->camera.zoom = DEFAULT_CAMERA_ZOOM + (MAX_CAMERA_ZOOM - DEFAULT_CAMERA_ZOOM) * exp(-CAMERA_ZOOM_TAU*t);
    }

    if (player->dash.is_dashing) {
        world->camera.offset = Vector2Subtract(world->camera.offset, Vector2Scale(player->movement.velocity, -0.12 * time.dt));
    }
}   

// Dashes if space is hit, cooldown is 0, a direction is held, and is not currently dashing
bool should_player_dash(player player) {
    if (!IsKeyPressed(KEY_SPACE)) {
        return false;
    } else if (Vector2Length(get_input_acceleration()) == 0) {
        return false;
    } else if (player.dash.cooldown != 0 && player.slow_duration == 0) {
        return false;
    } else if (player.dash.is_dashing && player.slow_duration == 0) {
        return false;
    } else {
        return true;
    }
}

// projection of u onto v
Vector2 vector2_project(Vector2 u, Vector2 v) {
    double dot = u.x * v.x + u.y * v.y;
    double divisor = Vector2LengthSqr(v);

    Vector2 project = Vector2Scale(v, dot / divisor);
    return project;
}

void launch_player(player *player, Vector2 away_from) {
    Vector2 launch_vector = Vector2Subtract(player->movement.position, away_from);
    launch_vector = Vector2Normalize(launch_vector);
    player->dash.direction = launch_vector;
    launch_vector = Vector2Scale(launch_vector, HITBOX_COLLISION_LAUNCH_COE);
    player->movement.velocity = launch_vector;

    player->movement_state = INVOLUNTARY_DASH;
    player->dash.is_dashing = true;
    player->dash.duration = DASH_DURATION;
    player->dash.cooldown = DASH_COOLDOWN;
    player->slow_duration = 0;
}

// On kill, slow down and immediately refresh dash.

// Returns a unit vector in direction of wasd input
Vector2 get_input_acceleration(void) {
    Vector2 movement;
    movement.x = 0;
    movement.y = 0;
    if (IsKeyDown(KEY_W)) {
        movement.y -= 1;
    }
    
    if (IsKeyDown(KEY_A)) {
        movement.x -= 1;
    }
    
    if (IsKeyDown(KEY_S)) {
        movement.y += 1;
    }
    
    if (IsKeyDown(KEY_D)) {
        movement.x += 1;
    }

    movement = Vector2Normalize(movement);
    return movement;
}

// Updates player's health
void update_player_health(world_objects *objects, player *player, time time) {
    if (player->invincible_duration != 0) {
        return;
    }

    if (check_player_hurtbox_collision(*objects, *player, time)) {
        player->health--;
        player->invincible_duration = INVINCIBILITY_DURATION;
        return;
    }
}