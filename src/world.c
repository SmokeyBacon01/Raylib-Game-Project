#include "raylib.h"
#include "raymath.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "header.h"
#include "world.h"
#include "entity.h"
#include "enemy.h"

void draw_debug(time time, game_world *world) {
    int n = 10;
    DrawFPS(10, n);
    n += 40;
    DrawText(TextFormat("WAVE: %d", world->wave_count + 1), 10, n, 20, BLACK);
    n += 40;
    DrawText(TextFormat("Time elapsed: %lf", time.total), 10, n, 20, BLACK);
    n += 40;
    DrawText(TextFormat("HP: %d", world->player.health), 10, n, 20, BLACK);
    n += 40;
    DrawText(TextFormat("DEATHS: %d", world->player.deaths), 10, n, 20, BLACK);
    n += 40;
}

void reset_game(game_world *world, time *time) {
    time->time_change_start = 0;
    time->zoom_change_start = 0;

    initialise_enemies(&world->objects.enemies);
    initialise_hurtboxes(&world->objects);
    initialise_hitboxes(&world->objects);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        world->objects.enemies.list[i].is_active = false;
    }

    world->objects.enemies.count = 0;
    reset_player(&world->player);

    world->title.is_active = false;
    world->title.time_shown = 0;

    spawn_appropriate_wave(world);
    show_title(world, TITLE_CARD);
}

void reset_player(player *player) {
    player->health = PLAYER_MAX_HEALTH;
    player->movement_state = NORMAL;
    player->movement.position = Vector2Zero();
    player->movement.velocity = Vector2Zero();
    player->movement.acceleration = Vector2Zero();
    player->invincible_duration = 0;
    player->slow_duration = 0;
    player->dash.is_dashing = false;
    player->dash.cooldown = 0;
    player->dash.duration = 0;
    player->dash.direction = Vector2Zero();
}

void update_time(game_world *world, time *time, player *player) {
    time->dt = GetFrameTime();
    time->true_dt = GetFrameTime();
    time->total += time->true_dt;

    if (world->title.is_active) {
        double time_taken =  world->time->total - world->time->time_change_start;
        double slow_rate = 0.1 + (1.0 - 0.2)*expf(-15.0 * time_taken);
        time->dt *= slow_rate;
        return;
    }

    if (player->slow_duration > 0) {
        slow_time(time);
    } else {
        fix_time(time);
    }

    if (player->slow_duration > 0) {
        player->slow_duration -= time->true_dt;
    } else if (player->slow_duration < 0) {
        player->slow_duration = 0;
        time->time_change_start = time->total;
        player->dash.cooldown = DASH_COOLDOWN;
    }
}

void slow_time(time *time) {
    double slow_rate;
    double t = time->total - time->time_change_start;

    slow_rate = MAXIMUM_SLOW + (1 - MAXIMUM_SLOW) * exp(-15 * t);
    time->dt *= slow_rate;
}

void fix_time(time *time) {
    double slow_rate;
    double t = time->total - time->time_change_start;

    slow_rate = 1 + (MAXIMUM_SLOW - 1) * exp(-15 * t);
    time->dt *= slow_rate;
}

// Draws what the map looks like
void draw_map(void) {
    ClearBackground(DARKGRAY);

    DrawRectangle(-MAP_SIZE, -MAP_SIZE, 2 * MAP_SIZE, 2 * MAP_SIZE, WHITE);

    for (int x = -MAP_SIZE; x <= MAP_SIZE; x += MAP_SIZE / 15) {
        DrawLine(x, -MAP_SIZE, x, MAP_SIZE, LIGHTGRAY);
    }

    for (int y = -MAP_SIZE; y <= MAP_SIZE; y += MAP_SIZE / 15) {
        DrawLine(-MAP_SIZE, y, MAP_SIZE, y, LIGHTGRAY);
    }
}
void waves(game_world *world, time time, double *cooldown) {
    if (world->is_game_won) {
        return;
    }

    if (*cooldown > 0) {
        *cooldown -= time.dt;
        return;
    } else if (*cooldown < 0) {
        *cooldown = WAVE_INTERMISSION;
    }

    if (world->objects.enemies.count == 0){
        world->wave_count++;
        spawn_appropriate_wave(world);
        if (!world->is_game_won) {
            show_title(world, "NEXT WAVE");
        }
        draw_title(world);

        world->player.health = PLAYER_MAX_HEALTH;
    }
}

void initialise_waves(game_world *world) {
    // USE IT LIKE THIS:
    //  {CHASER, BOMBERS, LASERS, DRONES, CHARGERS, 0, TESLAS}
    // the 0 is because CHARGER_WEAKPOINT is an enemy type.
    // It doesnt actually matter, it won't be used.
    // DRONE CAP IS 8
    
    
    // this big ass gap is to align the array.
    // now, you can use lines to count the waves.
    
    world->wave_count = 0;
    int waves[MAX_WAVES][ENEMY_COUNT] = {
    //  {c, B, L, D, C, 0, t}
        {3, 0, 0, 0, 0, 0, 0},
        {5, 0, 0, 0, 0, 0, 0},
        {3, 1, 0, 0, 0, 0, 0},
        {0, 5, 0, 0, 0, 0, 0},
        {0, 0, 3, 0, 0, 0, 0},
        {3, 0, 3, 0, 0, 0, 0},
        {0, 3, 5, 0, 0, 0, 0},
        {8, 0, 0, 0, 0, 0, 0},
        {0, 0, 8, 0, 0, 0, 0},
        {15, 3, 0, 0, 0, 0, 0},
        {20, 0, 5, 0, 0, 0, 0},
        {0, 0, 0, 8, 0, 0, 0},
        {0, 0, 3, 8, 0, 0, 0},
        {3, 1, 0, 5, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0},
        {0, 0, 3, 0, 3, 0, 0},
        {0, 0, 0, 8, 3, 0, 0},
        {3, 0, 0, 0, 0, 0, 5},
        {5, 0, 2, 8, 1, 0, 5}
    };

    memcpy(world->waves, waves, sizeof(world->waves));
}

void spawn_appropriate_wave(game_world *world) {
    // test_spawn_wave(world);
    // return;
    
    if (world->wave_count == MAX_WAVES) {
        world->is_game_won = true;
        world->time->time_of_win = world->time->total;
    } else {
        spawn_wave(world, world->waves[world->wave_count]);
    }
}

void spawn_wave(game_world *world, int wave_data[ENEMY_COUNT]) {
    for (int i = 0; i < ENEMY_COUNT; i++) {
        for (int j = 0; j < wave_data[i]; j++) {
            if (!should_spawn_enemy(i)) {
                continue;
            }
            spawn_enemy(world, i);
        }
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (world->objects.enemies.list[i].is_active) {
            world->objects.enemies.list[i].id = i;
        }
    }
}

bool should_spawn_enemy(enemy_type type) {
    if (type == CHARGER_WEAKPOINT) {
        return false;
    }
    return true;
}

// Updates what the camera sees
void update_draw_frame(game_world *world, time time) {
    BeginDrawing();
        BeginMode2D(world->camera);
        draw_map();
        
        // PLAYER
        Color colour;
        switch (world->player.movement_state) {
            case NORMAL: 
                colour = BLACK;
                break;
            case MANUAL_DASH:
                colour = BLUE;
                DrawCircleV(world->player.movement.position, PLAYER_DASH_RADIUS, ColorAlpha(BLUE, 0.3));
                break;
            case INVOLUNTARY_DASH:
                colour = PURPLE;
                break;
            case DEAD:
                colour = RED;
        }

        if (world->player.invincible_duration > 0) {
            colour = RED;
        }


        enemy *target = get_reticle_target(world);
        if (target != NULL) {
            DrawCircleV(target->movement.position, target->radius + 5, RED);
            DrawCircleV(target->movement.position, target->radius + 3, WHITE);
        }
        

        DrawCircleV(world->player.movement.position, PLAYER_RADIUS + 3, colour);

        //OBJECTS
        // draw_hitboxes(&world->objects);
        draw_hurtboxes(&world->objects);

        //ENEMIES
        for (int i = 0; i < world->objects.enemies.count; i++) {
            enemy enemy = world->objects.enemies.list[i];
            if (!(enemy.is_active)) {
                continue;
            }

            if (enemy.type == CHASER) {
                draw_chaser(enemy);
            } else if (enemy.type == BOMBER) {
                draw_bomber(enemy);
            } else if (enemy.type == LASER) {
                draw_laser(enemy);
            } else if (enemy.type == DRONE) {
                draw_drone(enemy);
            } else if (enemy.type == CHARGER) {
                draw_charger(world, enemy);
            } else if (enemy.type == TESLA) {
                draw_tesla(enemy);
            }
        }   

        EndMode2D();
        
        draw_debug(time, world);
        draw_title(world);    

    EndDrawing();
}


void draw_chaser(enemy enemy) {
    if (enemy.chaser.state == CHASER_STALKING || enemy.chaser.state == CHASER_SCATTERING) {
        DrawCircleV(enemy.movement.position, CHASER_RADIUS, PURPLE);
    } else if (enemy.chaser.state == CHASER_FLANKING) {
        DrawCircleV(enemy.movement.position, CHASER_RADIUS + 3, BLUE);
        DrawCircleV(enemy.movement.position, CHASER_RADIUS, PURPLE);
    } else {
        DrawCircleV(enemy.movement.position, CHASER_RADIUS + 3, RED);
        DrawCircleV(enemy.movement.position, CHASER_RADIUS, PURPLE);
    }
}

void draw_bomber(enemy enemy) {
    if (enemy.bomber.state == BOMBER_CHASING) {
        DrawCircleV(enemy.movement.position, BOMBER_RADIUS, RED);
        DrawCircleV(enemy.movement.position, CHASER_RADIUS, BLACK);
    } else if (enemy.bomber.state == BOMBER_STALKING) {
        DrawCircleV(enemy.movement.position, BOMBER_RADIUS, BLACK);
        DrawCircleV(enemy.movement.position, CHASER_RADIUS, RED);
    } else if (enemy.bomber.state == BOMBER_ATTACKING) {
        DrawCircleV(enemy.movement.position, BOMBER_RADIUS, RED);
        DrawCircleV(enemy.movement.position, BOMBER_EXPLOSION_RADIUS, ColorAlpha(RED, HURTBOX_TELEGRAPH_ALPHA));
    }
}

void draw_laser(enemy enemy) {
    if (enemy.hitbox.health == 2) {
        DrawCircleV(enemy.movement.position, LASER_RADIUS + 3, BLUE);
    }
    DrawCircleV(enemy.movement.position, LASER_RADIUS, GREEN);
    
    Vector2 AB = Vector2Subtract(enemy.laser.aim_target.position, enemy.movement.position);
    Vector2 C = Vector2Add(enemy.movement.position, Vector2Scale(AB, 50));
    Vector2 laser_start = Vector2Add(enemy.movement.position, Vector2Scale(Vector2Normalize(AB), LASER_RADIUS));
    if (enemy.laser.state == LASER_AIMING) {
        DrawLineV(laser_start, C, BLUE);
    } else if (enemy.laser.state == LASER_FIRING) {
        DrawLineV(laser_start, C, RED);
    }

    // DrawCircleV(enemy.laser.aim_target.position, 10, BLUE);
}

void draw_drone(enemy enemy) {
    DrawCircleV(enemy.movement.position, DRONE_RADIUS, GRAY);
}

void draw_charger(game_world *world, enemy enemy) {
    for (int i = 0; i < CHARGER_WEAKPOINT_COUNT; i++) {
        struct enemy *weakpoint = get_enemy_from_key(world, enemy.charger.weakpoints[i]);
        if (weakpoint == NULL) {
            continue;
        }

        if (weakpoint->hitbox.invincible_duration == 0) {
            DrawCircleV(weakpoint->movement.position, CHARGER_WEAKPOINT_RADIUS, RED);
        } else {
            DrawCircleV(weakpoint->movement.position, CHARGER_WEAKPOINT_RADIUS, LIGHTGRAY);
        }
    }
    DrawCircleV(enemy.movement.position, CHARGER_RADIUS, LIGHTGRAY);
}

void draw_tesla(enemy enemy) {
    DrawCircleV(enemy.movement.position, TESLA_RADIUS, BLUE);
}

void update_camera(game_world *world, time time) {
    world->camera.target = world->player.movement.position;
    Vector2 centre = {world->screen.width / 2, world->screen.height / 2};

    if (Vector2Equals(world->camera.offset, centre)) {
        world->camera_data.acceleration = Vector2Zero();
        world->camera_data.velocity = Vector2Zero();
        return;
    }

    if (world->player.dash.is_dashing) {
        return;
    }

    Vector2 direction = Vector2Subtract(centre, world->camera.offset);
    Vector2 resistance = Vector2Scale(world->camera_data.velocity, -CAMERA_RESISTANCE_COE);

    if (Vector2Length(direction) < 10) {
        world->camera.offset = centre;
        world->camera_data.acceleration = Vector2Zero();
        world->camera_data.velocity = Vector2Zero();
        return;
    }
    
    world->camera_data.acceleration = Vector2Add(Vector2Scale(direction, CAMERA_ACCELERATION_COE * time.true_dt), resistance);
    world->camera_data.velocity = Vector2Add(world->camera_data.velocity, Vector2Scale(world->camera_data.acceleration, time.true_dt));
    world->camera.offset = Vector2Add(world->camera.offset, Vector2Scale(world->camera_data.velocity, time.true_dt));
}

// Initialises values
void initialise_world(game_world *world, time *time) {
    world->screen.width = GetScreenWidth();
    world->screen.height = GetScreenHeight();

    world->camera.target = world->player.movement.position;
    world->camera.offset = (Vector2){world->screen.width / 2, world->screen.height / 2};
    world->camera.zoom = DEFAULT_CAMERA_ZOOM;
    world->camera.rotation = 0.0f;

    world->camera_data.velocity = Vector2Zero();
    world->camera_data.acceleration = Vector2Zero();
    world->camera_data.position = Vector2Zero();

    world->time = time;
    world->objects.time = time;

    world->title.is_active = false;
    world->title.text = "welcome to fuck you";
    world->title.time_shown = 0;
    world->title.fade_in = 0.2;
    world->title.fade_out = 0.2;

    world->is_game_won = false;

    initialise_player(&world->player);
    initialise_enemies(&world->objects.enemies);
    initialise_waves(world);

    show_title(world, TITLE_CARD);
    
}

void initialise_player(player *player) {
    player->health = PLAYER_MAX_HEALTH;
    player->movement_state = NORMAL;
    player->movement.position = Vector2Zero();
    player->movement.velocity = Vector2Zero();
    player->movement.acceleration = Vector2Zero();
    player->invincible_duration = 0;
    player->slow_duration = 0;
    player->dash.is_dashing = false;
    player->dash.cooldown = 0;
    player->dash.duration = 0;
    player->dash.direction = Vector2Zero();
    player->kills = 0;
    player->damage_taken = 0;
    player->deaths = 0;
}

void initialise_time(time *time) {
    time->time_of_win = 0;
    time->total = 0;
    time->time_change_start = 0;
    time->zoom_change_start = 0;
}

void initialise_enemies(enemies *enemy) {
    enemy->count = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemy->list[i].is_active = false;
        enemy->list[i].hitbox.is_active = false;
        enemy->list[i].tesla.charging_tesla_key = INVALID_KEY;
    }

    initialise_drones(&enemy->drones);
    enemy->enemy_keys = 0;
}

void initialise_hitboxes(world_objects *objects) {
    objects->enemies.count = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        objects->enemies.list[i].hitbox.is_active = false;
    }
}

void draw_hurtboxes(world_objects *objects) {
    for (int i = 0; i < objects->hurtbox_count; i++) {
        if(objects->hurtbox_count >= MAX_HURTBOXES) {
            break;
        }
        hurtbox hurtbox = objects->hurtboxes[i];
        if (!hurtbox.is_active) {
            continue;
        }

        Color colour;
        if (hurtbox.telegraph_duration == 0) {
            colour = RED;
        } else {
            colour = ColorAlpha(RED, HURTBOX_TELEGRAPH_ALPHA);
        }


        if (hurtbox.shape == LINE) {
            if (hurtbox.telegraph_duration != 0) {
                colour = GRAY;
            }
            DrawLineV(hurtbox.line.start, hurtbox.line.end, colour);
        } else if (hurtbox.shape == CIRCLE) {
            DrawCircleV(hurtbox.circle.centre, hurtbox.circle.radius, colour);
        } else if (hurtbox.shape == RECTANGLE) {
            DrawRectangleRec(hurtbox.rectangle, colour);
        } else if (hurtbox.shape == CAPSULE) {
            draw_capsule(hurtbox.capsule, colour);
        }
    }    
}

void draw_capsule(capsule_data capsule, Color colour) {
    DrawCircleV(capsule.start, capsule.radius, colour);
    DrawCircleV(capsule.end, capsule.radius, colour);
    DrawLineEx(capsule.start, capsule.end, 2 * capsule.radius, colour);
}

void draw_hitboxes(world_objects *objects) {
    for (int i = 0; i < objects->enemies.count; i++) {
        hitbox hitbox = objects->enemies.list[i].hitbox;

        if (!hitbox.is_active) {
            continue;
        }

        DrawCircleV(hitbox.circle.centre, hitbox.circle.radius, BLUE);
    }    
}

//////////////////////////////////////////////////////////////

/////////////////////////// MUSIC ///////////////////////////

