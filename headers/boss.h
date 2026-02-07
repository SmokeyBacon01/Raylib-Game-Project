#ifndef BOSS_H
#define BOSS_H

void create_boss(enemy *enemy);
void update_boss(game_world *world, enemy *enemy, time *time);
void boss_decay(enemy *enemy, time *time);
void spawn_boss_bomber(game_world *world, enemy *enemy, time *time);

void debug_boss_attacks(game_world *world);
void update_attack_durations(enemy *enemy, time *time);

void begin_vertical_laser(game_world *world, enemy *enemy);
void vertical_laser(game_world *world);

void begin_horizontal_laser(game_world *world, enemy *enemy);
void horizontal_laser(game_world *world);

void begin_t_laser(game_world *world, enemy *enemy);
void t_laser(game_world *world);

void begin_random_barrage(enemy *enemy);
void update_random_barrage(game_world *world, enemy *enemy, time *time);
void random_barrage(game_world *world);

void begin_focus_barrage(enemy *enemy);
void update_focus_barrage(game_world *world, enemy *enemy, time *time);
void focus_barrage(game_world *world);

#endif