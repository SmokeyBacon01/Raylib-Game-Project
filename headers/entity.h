#ifndef ENTITY_H
#define ENTITY_H
// NEW

void update_player_movement(game_world *world, time *time);
void normal_player_movement(game_world *world, time *time);
void normal_player_hitbox_collision(game_world *world);

void begin_player_dash(game_world *world);
void dash_player_movement(game_world *world, time *time);
void dash_player_hitbox_collision(game_world *world);

void damage_player(game_world *world, int damage);

// OLD

void collide_player_world(player *player);
void bounce_player_world(player *player, enum axis direction);
void dash_player(player *player, time *time);
void extend_dash (player *player);
void dash_zoom(player *player, time time, game_world *world);
bool should_player_dash (player player);
void move_player(player *player, time time, world_objects *objects);
void launch_player(player *player, Vector2 away_from);

void update_player_hitbox_collision(world_objects *objects, player *player);

void initialise_hurtboxes(world_objects *objects);
void update_hurtboxes(world_objects *objects, time time);
void remove_hurtbox(world_objects *objects, int i);
bool check_player_hurtbox_collision(world_objects objects, player player, time time);
bool check_collision_capsule_circle(Vector2 centre, double radius, capsule_data capsule);

void update_player_health(world_objects *objects, player *player, time time);

Vector2 get_input_acceleration(void);
Vector2 normalise_vector(Vector2 vector);
Vector2 vector2_project(Vector2 u, Vector2 v);

#endif