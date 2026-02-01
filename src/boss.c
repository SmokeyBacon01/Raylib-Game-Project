#include "raylib.h"
#include "raymath.h"

#include "header.h"
#include "boss.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// function that initiates the boss wave.
// Create a new "boss" enemy that is invisible with no position.
// This entity is alive at all times and force killed at the end of the fight.

void create_boss(game_world *world, enemy *enemy) {
    enemy->hitbox.health = CHASER_HEALTH;
    enemy->movement.position = (Vector2){10000, 10000};
    enemy->hitbox.circle.centre = enemy->movement.position;
    enemy->hitbox.circle.radius = CHASER_RADIUS;
    enemy->radius = CHASER_RADIUS;
    enemy->knockback_coefficient = CHASER_KNOCKBACK_COEFFICIENT;
}