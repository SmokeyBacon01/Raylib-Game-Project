#ifndef WORLD_H
#define WORLD_H

void initialise_world(game_world *world, time *time);
void initialise_player(player *player);
void initialise_time(time *time);
void initialise_enemies(enemies *enemy);
void initialise_hitboxes(world_objects *objects);
void initialise_waves(game_world *world);

void slow_time(time *time);
void fix_time(time *time);

void waves(game_world *world, time time, double *cooldown);
void spawn_appropriate_wave(game_world *world);
void spawn_wave(game_world *world, int wave_data[ENEMY_COUNT]);
bool should_spawn_enemy(enemy_type type);

void update_camera(game_world *world, time time);

void draw_hitboxes(world_objects *objects);

void draw_hurtboxes(world_objects *objects);

void draw_capsule(capsule_data capsule, Color colour);

void draw_chaser(enemy enemy);
void draw_bomber(enemy enemy);
void draw_laser(enemy enemy);
void draw_drone(enemy enemy);
void draw_charger(game_world *world, enemy enemy);
void draw_tesla(enemy enemy);

void reset_game(game_world *world, time *time);
void reset_player(player *player);

#endif