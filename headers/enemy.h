#ifndef ENEMY_H
#define ENEMY_H

// MAIN UPDATE FUNCTION
void update_enemies(game_world *world, time *time);

// SPAWNING
int spawn_enemy(game_world *world, enemy_type type);
bool is_spawn_too_close(game_world *world, Vector2 position);
void create_enemy(game_world *world, enemy *enemy, enemy_type type);

// CHASERS
void create_chaser(game_world *world, enemy *enemy);
void update_chaser(game_world *world, enemy *enemy, time *time);
Vector2 find_chaser_spawn(game_world *world);
void chaser_stalking_movement(enemy *enemy, player  *player, time *time);
Vector2 get_chaser_stalking_point(enemy *enemy, player *player);
void chaser_flanking_movement(enemy *enemy, player  *player, time *time);
Vector2 get_chaser_flanking_point(enemy *enemy, player *player);
void chaser_attacking_movement(enemy *enemy, time *time);
bool is_chaser_within_explosion_range(game_world *world, enemy *enemy);
Vector2 escape_bombers_range(game_world *world, enemy *enemy);
void chaser_scattering_movement(enemy *enemy, game_world *world, time *time);
void start_enemy_dash(game_world *world, enemy *enemy);
void update_enemy_dash(enemy *enemy, time *time);
void update_chaser_attack_cooldown(enemy *enemy, time *time);

// BOMBERS
void create_bomber(game_world *world, enemy *enemy);
void update_bomber(game_world *world, enemy *enemy, time *time);
void bomber_stalking_movement(game_world *world, enemy *enemy, time *time);
void bomber_chasing_movement(game_world *world, enemy *enemy, time *time);
void bomber_attacking_movement(enemy *enemy, time *time);
Vector2 find_bomber_spawn(game_world *world);
bool should_bomber_chase(game_world *world, enemy *enemy, time *time);
bool should_bomber_attack(game_world *world, enemy *enemy);
void update_bomber_fuse(enemy *enemy, time *time);
void bomber_explode(game_world *world, enemy *enemy);
void collide_bomber(enemy *enemy, player *player);
void chain_bomber(enemy *victim);

// LASERS
void create_laser(game_world *world, enemy *enemy);
void update_laser(game_world *world, enemy *enemy, time *time);
Vector2 find_laser_spawn(game_world *world);
Vector2 find_laser_target_spot(game_world *world, enemy *enemy);
void update_target_spot(game_world *world, enemy *enemy);
void laser_stalking_movement(game_world *world, enemy *enemy, time *time);
void laser_waiting_movement(enemy *enemy, time *time);
void laser_update_attack_cooldown(enemy *enemy, time *time);
Vector2 find_true_laser_target(game_world *world);
void laser_aiming(game_world *world, enemy *enemy, time *time);
void laser_fire_aiming(game_world *world, enemy *enemy, time *time);
void create_laser_attack(game_world *world, enemy *enemy);

// DRONES
void create_drone(game_world *world, enemy *enemy);
void initialise_drones(drone_collective_data *drones);
void update_drone(game_world *world, enemy *enemy, time *time);
void update_all_drones(game_world *world, time *time);
void reset_drones(game_world *world);
void update_drone_id(game_world *world);
void drone_stalking_movement(game_world *world, enemy *enemy, time *time);
bool is_all_drones_ready(game_world *world);
void drone_reset_ready(game_world *world);
Vector2 get_drone_target(game_world *world, enemy *enemy);
double get_drone_angle_divider(game_world *world);
void drone_attacking_movement(game_world *world, enemy *enemy);
void rotate_base_drone_vector(game_world *world, time *time);
void reset_base_drone_vector(game_world *world);
void drone_translating_orbit_centre_movement(game_world *world, time *time);
void drone_translating_movement(game_world *world, enemy *enemy);
void drones_update_radius(game_world *world, drone_collective_data *drones, time *time);
void expand_base_drone_vector(game_world *world, time *time, double final_size);
void shrink_base_drone_vector(game_world *world, time *time, double final_size);
void drones_replace_hitbox_with_hurtbox(game_world *world);
void drones_sync_hurtbox(game_world *world);
void drones_create_passive_lasers(game_world *world, time *time);
void drones_create_active_lasers(game_world *world);
void drones_clear_hurtboxes(game_world *world);
double get_modified_final_size(game_world *world, int final_size);
double get_modified_interval_multiplier(game_world *world);
int get_non_drone_enemy_count(game_world *world);
void drones_exit_stalking(drone_collective_data *drones, time *time);
void drones_exit_positioning(game_world *world, drone_collective_data *drones);
void drones_update_orbiting(game_world *world, drone_collective_data *drones, time *time);
void drones_exit_orbiting(game_world *world, drone_collective_data *drones, time *time);
void drones_update_translating(game_world *world, drone_collective_data *drones, time *time);
void drones_exit_translating (game_world *world, drone_collective_data *drones);
void drones_update_barraging(game_world *world, drone_collective_data *drones, time *time);
void drones_fire_active_lasers(game_world *world, drone_collective_data *drones, time *time);
void drones_exit_barraging(game_world *world, drone_collective_data *drones, time *time);
void drones_create_big_laser(game_world *world);
void drones_update_big_laser(game_world *world, time *time);
void drones_disable_hitbox(game_world *world);
void drone_give_up(game_world *world, enemy *enemy, time *time);
double get_drone_stalking_duration(game_world *world);

// CHARGERS
void create_charger(game_world *world, enemy *enemy);
void update_charger(game_world *world, enemy *enemy, time *time);
void update_charger_weakpoints(game_world *world, enemy *enemy, time *time);
Vector2 find_charger_spawn(game_world *world);
bool is_overlapping_with_other_enemy(game_world *world, enemy *enemy);
void create_charger_weakpoints(game_world *world, enemy *enemy);
void create_charger_weakpoint(enemy *enemy);
void charger_heal(enemy *enemy);
void charger_update_facing_direction(game_world *world, enemy *enemy, time *time);
void charger_update_stalking(game_world *world, enemy *enemy, time *time);
void charger_stalking_movement(game_world *world, enemy *enemy, time *time);
void charger_exit_stalking(game_world *world, enemy *enemy, time *time);
void charger_update_stomping(game_world *world, enemy *enemy, time *time);
void update_stomp_hurtbox(game_world *world, enemy *enemy);
void charger_update_positioning(game_world *world, enemy *enemy, time *time);
void charger_exit_positioning(enemy *enemy, time *time);
void charger_update_charging(game_world *world, enemy *enemy, time *time);
void bounce_charger_off_walls(game_world *world, enemy *enemy);
int get_charger_bounce_count(enemy *enemy);
void charger_exit_charging(enemy *enemy);
void charger_update_stunned(enemy *enemy, time *time);
void charger_exit_stunned(enemy *enemy, time *time);

// TESLAS
void create_tesla(game_world *world, enemy *enemy);
void update_tesla(game_world *world, enemy *enemy, time *time);
void initialise_teslas(tesla_collective_data *teslas);
void tesla_enter_stalking(game_world *world, enemy *enemy);
void tesla_exit_stalking(game_world *world, enemy *enemy, time *time);
void tesla_enter_charging(game_world *world, enemy *enemy);
void tesla_exit_charging(game_world *world, enemy *enemy, time *time);
void tesla_update_all_charge_targets(game_world *world);
int get_uncharged_enemy_key(game_world *world);

// ENEMY UPDATES
void kill_enemies(game_world *world);
void kill_enemy(game_world *world, int kill_index);
void kill_tesla(game_world *world, int kill_index);
void kill_drone(game_world *world, int kill_index);
void kill_laser(game_world *world, int kill_index);
void kill_charger_weakpoint(game_world *world, int kill_index);
void update_hitbox_position(enemy *enemy);

// ENEMY UTILITIES
void damage_enemy(world_objects *objects, player *player, int enemy_id, int damage);
hurtbox *get_hurtbox_from_key(game_world *world, int key);
enemy *get_enemy_from_key(game_world *world, int key);

// HITBOX STUFF
void update_hitboxes(game_world *world);
void test_spawn_wave(game_world *world);

// CREATE HURTBOX
int create_line_hurtbox(world_objects *objects, Vector2 start, Vector2 target, double telegraph_duration, double duration);
int create_circle_hurtbox(world_objects *objects, Vector2 centre, double radius, double telegraph_duration, double duration);
int create_rectangle_hurtbox(world_objects *objects, Rectangle rectangle, double telegraph_duration, double duration);
int create_capsule_hurtbox(world_objects *objects, Vector2 start, Vector2 end, double radius, double telegraph_duration, double duration);

// GENERAL UTILITIES
void clamp_enemy(enemy *enemy);
void update_enemy_invincibility(enemy *enemy, time *time);
void general_movement(movement_data *movement, Vector2 direction, double acceleration_coe, double resistance_coe, time *time);
void update_counter(double *counter, time *time);
void bounce_enemy_off_walls(enemy *enemy);
int get_hurtbox_index_in_array(hurtbox hurtboxes[MAX_HURTBOXES], hurtbox *hurtbox);
void remove_hurtbox_from_key(game_world *world, int key);
Vector2 Vector2Polar(double mod, double arg);
double degrees_to_radians(double degree);
bool is_close_enough(double x, double y);

#endif