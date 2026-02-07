#include "raylib.h"
#include "raymath.h"

#include "header.h"
#include "boss.h"
#include "enemy.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// function that initiates the boss wave.
// Create a new "boss" enemy that is invisible with no position.
// This entity is alive at all times and force killed at the end of the fight.

void create_boss(enemy *enemy) {
    enemy->hitbox.health = CHASER_HEALTH;
    enemy->movement.position = (Vector2){10000, 10000};
    enemy->hitbox.circle.centre = enemy->movement.position;
    enemy->hitbox.circle.radius = CHASER_RADIUS;
    enemy->radius = CHASER_RADIUS;
    enemy->knockback_coefficient = CHASER_KNOCKBACK_COEFFICIENT;

    enemy->boss.state = BOSS_ONE;
    enemy->boss.health = BOSS_HEALTH;
    enemy->boss.bomber_cooldown = BOSS_SPAWN_BOMBER_COOLDOWN;

    for (int i = 0; i < BOSS_TOTAL_ATTACKS; i++) {
        enemy->boss.attacks[i].duration = 0;
    }
}

// BOSS ENEMY
// bullet hell-esque battle. Create 4 weakpoints on each corner. Objective is to simply survive. Hitting the pillar does small damage. Blowing it with bombers deals much more damage.
// STAGES.
// ENTER BOSS
// immediately initailised with a wave of attacks.

// / \ | - pattern lasers
// All  pillars fire lasers 
// Waterfall like attacks.

// arena is filled with circular attacks.

// Release a bomber every now and again.

// at 2/3 hp, attacks get more complicated.
// Attacks are now X and t. May also be centred.
// Waterfall attacks can happen horizontally and vertically. 


// at 1/3 hp, decrease attack interval.

void debug_boss_attacks(game_world *world) {
    if (IsKeyPressed(KEY_ONE)) {
        begin_vertical_laser(world, &world->objects.enemies.list[0]);
    }

    if (IsKeyPressed(KEY_TWO)) {
        begin_horizontal_laser(world, &world->objects.enemies.list[0]);
    }

    if (IsKeyPressed(KEY_THREE)) {
        begin_random_barrage(&world->objects.enemies.list[0]);
    }

    if (IsKeyPressed(KEY_FOUR)) {
        begin_focus_barrage(&world->objects.enemies.list[0]);
    }

    if (IsKeyPressed(KEY_FIVE)) {

    }
}

void update_boss(game_world *world, enemy *enemy, time *time) {
    boss_decay(enemy, time);
    // spawn_boss_bomber(world, enemy, time);
    update_attack_durations(enemy, time);

    update_random_barrage(world, enemy, time);
    update_focus_barrage(world, enemy, time);

    debug_boss_attacks(world);
}

// fires a ball that releases 3 pellets towards the player. Ball is fired from an edge. Once close enough, fire the shots.
void begin_shotgun_ball(enemy *enemy) {
    enemy->boss.attacks[BOSS_SHOTGUN_BALL].duration = BOSS_SHOTGUN_BALL_DURATION;
}

void begin_focus_barrage(enemy *enemy) {
    enemy->boss.attacks[BOSS_FOCUS_BARRAGE].duration = BOSS_FOCUS_BARRAGE_DURATION;
    enemy->boss.attacks[BOSS_FOCUS_BARRAGE].focus_barrage.interval = BOSS_FOCUS_BARRAGE_INTERVAL;
}

void update_focus_barrage(game_world *world, enemy *enemy, time *time) {
    if (enemy->boss.attacks[BOSS_FOCUS_BARRAGE].duration == 0) {
        return;
    }

    update_counter(&enemy->boss.attacks[BOSS_FOCUS_BARRAGE].focus_barrage.interval, time);
    if (enemy->boss.attacks[BOSS_FOCUS_BARRAGE].focus_barrage.interval == 0) {
        enemy->boss.attacks[BOSS_FOCUS_BARRAGE].focus_barrage.interval = BOSS_FOCUS_BARRAGE_INTERVAL;
        focus_barrage(world);
    }
}

void focus_barrage(game_world *world) {
    int mod = GetRandomValue(BOSS_FOCUS_INNER_RADIUS, BOSS_FOCUS_OUTER_RADIUS);
    float arg = GetRandomValue(LOWEST_ANGLE, HIGHEST_ANGLE);
    arg = degrees_to_radians(arg);

    Vector2 offset = Vector2Polar(mod, arg);
    Vector2 centre = Vector2Add(world->player.movement.position, offset);

    create_circle_hurtbox(&world->objects, centre, BOSS_FOCUS_BARRAGE_ATTACK_RADIUS, BOSS_TELEGRAPH, BOSS_FOCUS_BARRAGE_ATTACK_DURATION);
}

void begin_random_barrage(enemy *enemy) {
    enemy->boss.attacks[BOSS_RANDOM_BARRAGE].duration = BOSS_RANDOM_BARRAGE_DURATION;
    enemy->boss.attacks[BOSS_RANDOM_BARRAGE].random_barrage.interval = BOSS_RANDOM_BARRAGE_INTERVAL;
}

void update_random_barrage(game_world *world, enemy *enemy, time *time) {
    if (enemy->boss.attacks[BOSS_RANDOM_BARRAGE].duration == 0) {
        return;
    }

    update_counter(&enemy->boss.attacks[BOSS_RANDOM_BARRAGE].random_barrage.interval, time);
    if (enemy->boss.attacks[BOSS_RANDOM_BARRAGE].random_barrage.interval == 0) {
        enemy->boss.attacks[BOSS_RANDOM_BARRAGE].random_barrage.interval = BOSS_RANDOM_BARRAGE_INTERVAL;
        random_barrage(world);
    }
}

void random_barrage(game_world *world) {
    circle_data barrage;
    barrage.centre.x = GetRandomValue(-MAP_SIZE + BOSS_RANDOM_BARRAGE_ATTACK_RADIUS, MAP_SIZE - BOSS_RANDOM_BARRAGE_ATTACK_RADIUS);
    barrage.centre.y = GetRandomValue(-MAP_SIZE + BOSS_RANDOM_BARRAGE_ATTACK_RADIUS, MAP_SIZE - BOSS_RANDOM_BARRAGE_ATTACK_RADIUS);
    barrage.radius = BOSS_RANDOM_BARRAGE_ATTACK_RADIUS;

    create_circle_hurtbox(&world->objects, barrage.centre, barrage.radius, BOSS_RANDOM_BARRAGE_TELEGRAPH, BOSS_RANDOM_BARRAGE_ATTACK_DURATION);
}

void begin_vertical_laser(game_world *world, enemy *enemy) {
    enemy->boss.attacks[BOSS_VERTICAL_LASER].duration = BOSS_BIG_LASER_DURATION;
    vertical_laser(world);
}

void vertical_laser(game_world *world) {
    Rectangle rect;
    rect.height = MAP_SIZE * 2;
    rect.width = BOSS_BIG_LASER_WIDTH;
    rect.x = -BOSS_BIG_LASER_WIDTH / 2;
    rect.y = -MAP_SIZE;
    create_rectangle_hurtbox(&world->objects, rect, BOSS_BIG_LASER_TELEGRAPH, BOSS_BIG_LASER_DURATION);
}

void begin_horizontal_laser(game_world *world, enemy *enemy) {
    enemy->boss.attacks[BOSS_HORIZONTAL_LASER].duration = BOSS_BIG_LASER_DURATION;
    horizontal_laser(world);
}

void horizontal_laser(game_world *world) {
    Rectangle rect;
    rect.width = MAP_SIZE * 2;
    rect.height = BOSS_BIG_LASER_WIDTH;
    rect.y = -BOSS_BIG_LASER_WIDTH / 2;
    rect.x = -MAP_SIZE;
    create_rectangle_hurtbox(&world->objects, rect, BOSS_BIG_LASER_TELEGRAPH, BOSS_BIG_LASER_DURATION);
}

void begin_t_laser(game_world *world, enemy *enemy) {
    enemy->boss.attacks[BOSS_T_LASER].duration = BOSS_BIG_LASER_DURATION;
    t_laser(world);
}

void t_laser(game_world *world) {
    vertical_laser(world);
    horizontal_laser(world);
}

void boss_decay(enemy *enemy, time *time) {
    enemy->boss.health -= BOSS_HEALTH_DECAY * time->dt;
}

void update_attack_durations(enemy *enemy, time *time) {
    for (int i = 0; i < BOSS_TOTAL_ATTACKS; i++) {
        update_counter(&enemy->boss.attacks[i].duration, time);
    }
}

void spawn_boss_bomber(game_world *world, enemy *enemy, time *time) {
    update_counter(&enemy->boss.bomber_cooldown, time);
    if (enemy->boss.bomber_cooldown == 0) {
        spawn_enemy(world, BOMBER);
        enemy->boss.bomber_cooldown = BOSS_SPAWN_BOMBER_COOLDOWN;
    }
}