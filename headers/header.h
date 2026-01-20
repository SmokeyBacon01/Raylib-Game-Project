#ifndef HEADER_H
#define HEADER_H

#include "raylib.h"
#include "constants.h"

struct enemy;
typedef struct enemy enemy;

enum axis {
    X,
    Y
};

enum hurtbox_shape {
    LINE,
    CIRCLE,
    RECTANGLE,
    CAPSULE
};

typedef struct line_data {
    Vector2 start;
    Vector2 end;
} line_data;

typedef struct circle_data {
    Vector2 centre;
    double radius;
} circle_data;

typedef struct capsule_data {
    Vector2 start;
    Vector2 end;
    double radius;
} capsule_data;

typedef struct hurtbox {
    bool is_active;
    enum hurtbox_shape shape;
    line_data line;
    circle_data circle;
    capsule_data capsule;
    Rectangle rectangle;
    int key;
    double telegraph_duration;
    double duration;
} hurtbox;

typedef struct hitbox {
    bool is_active;
    circle_data circle;
    int health;
    double invincible_duration;
} hitbox;

typedef struct title_text {
    bool is_active;
    const char *text;
    double time_shown;
    double fade_in;
    double fade_out;
} title_text;


typedef struct time {
    double dt;
    double true_dt;
    double total;
    double time_change_start;
    double zoom_change_start;
    double time_of_win;
} time;

typedef struct screen {
    int width;
    int height;
} screen;

typedef struct movement_data {
    Vector2 acceleration;
    Vector2 velocity;
    Vector2 position;
} movement_data;

struct dash {
    bool is_dashing;
    double duration;
    double cooldown;
    Vector2 direction;
};

typedef enum player_movement_state {
    NORMAL,
    MANUAL_DASH,
    INVOLUNTARY_DASH,
    DEAD
} player_movement_state;


typedef struct player {
    player_movement_state movement_state;
    movement_data movement;
    int health;
    double invincible_duration;
    double slow_duration;
    struct dash dash;
    int kills;
    int damage_taken;
    int deaths;
} player;

typedef enum enemy_type {
    CHASER,
    BOMBER,
    LASER,
    DRONE,
    CHARGER,
    CHARGER_WEAKPOINT,
    TESLA,
    ENEMY_COUNT
} enemy_type;

typedef enum chaser_state {
    CHASER_STALKING,
    CHASER_SCATTERING,
    CHASER_FLANKING,
    CHASER_ATTACKING
} chaser_state;

typedef struct chaser_data {
    chaser_state state;
    Vector2 stalking_point;
    float attack_cooldown;
    bool is_attacking;
    float dash_time_left;
    Vector2 dash_velocity;
} chaser_data;

typedef enum bomber_state {
    BOMBER_STALKING,
    BOMBER_CHASING,
    BOMBER_ATTACKING,
    BOMBER_EXPLODING
} bomber_state;

typedef struct bomber_data {
    bomber_state state;
    double fuse_duration;
    double aggro_check_cooldown;
} bomber_data;

typedef enum laser_state {
    LASER_STALKING,
    LASER_WAITING,
    LASER_AIMING,
    LASER_FIRING,
    LASER_ATTACK
} laser_state;

typedef struct laser_data {
    laser_state state;
    Vector2 target_position;
    movement_data aim_target;
    int laser_key;
    double attack_cooldown;
    double aim_duration;
    double fire_delay;
    double fire_duration;
} laser_data;

typedef enum drone_state {
    DRONE_STALKING,
    DRONE_POSITIONING,
    DRONE_ORBITING,
    DRONE_TRANSLATING,
    DRONE_BARRAGING,
    DRONE_GIVE_UP,
    DRONE_INITIAL
} drone_state;

typedef struct drone_data {
    int id;
    int hurtbox_key;
    Vector2 target;
    bool is_ready_to_attack;
} drone_data;

typedef struct drone_collective_data {
    drone_state state;
    int drone_count;
    double angular_velocity;
    Vector2 base_vector;
    movement_data orbit_movement;
    enemy *drone_list[MAX_ENEMIES];
    double stalking_duration;
    double orbit_duration;
    double passive_laser_cooldown;
    double active_laser_cooldown;
    double attack_interval_multiplier;
    double min_interval_duration;
    double barraging_radius;
    double barraging_interval_multiplier;
    double big_laser_telegraph_duration;
    double big_laser_angular_velocity;
    double big_laser_reverse_timer;
    int big_laser_key;
} drone_collective_data;

typedef enum charger_state {
    CHARGER_STALKING,
    CHARGER_POSITIONING,
    CHARGER_CHARGING,
    CHARGER_STUNNED,
    CHARGER_STOMPING
} charger_state;

typedef struct charger_data {
    charger_state state;
    int weakpoints[CHARGER_WEAKPOINT_COUNT];
    Vector2 facing_direction;
    int stomp_key;
    double stalking_duration;
    double stomping_duration;
    double positioning_duration;
    double stunned_duration;
    int max_bounces;
} charger_data;

typedef enum tesla_state {
    TESLA_NONE,
    TESLA_STALKING,
    TESLA_CHARGING
} tesla_state;

typedef enum tesla_group_state {
    TESLA_GROUP_NONE,
    TESLA_GROUP_STALKING,
    TESLA_GROUP_CHARGING,
    TESLA_GROUP_DISCHARGE
} tesla_group_state;

typedef struct tesla_data {
    tesla_state state;
    double stalking_duration;
    double charging_duration;
    int charging_target_key;
    int charge_hurtbox_key;
    int charging_tesla_key;
} testla_data;

typedef struct tesla_collective_data {
    tesla_group_state state;
    enemy *tesla_list[MAX_ENEMIES];
    int tesla_count;
} tesla_collective_data;

struct enemy {
    bool is_active;
    bool should_count_towards_kills;
    bool should_slow_down_when_killed;
    movement_data movement;
    hitbox hitbox;
    enum enemy_type type;
    int id;
    int key;
    int parent_key;
    double radius;
    double knockback_coefficient;
    struct chaser_data chaser;
    struct bomber_data bomber;
    struct laser_data laser;
    struct drone_data drone;
    struct charger_data charger;
    struct tesla_data tesla;
};

typedef struct enemies {
    enemy list[MAX_ENEMIES];
    int count;
    int enemy_keys;
    drone_collective_data drones;
    tesla_collective_data teslas;
} enemies;

typedef struct world_objects {
    hurtbox hurtboxes[MAX_HURTBOXES];
    int hurtbox_count;
    int hurtbox_keys;
    time *time;
    enemies enemies;
} world_objects;

typedef struct music {
    Music music;
    float volume;
} music_data;

typedef struct game_world {
    world_objects objects;
    Camera2D camera;
    player player;
    screen screen;
    movement_data camera_data;
    time *time;
    title_text title;
    int waves[MAX_WAVES][ENEMY_COUNT];
    int wave_count;
    bool is_game_won;
} game_world;

void draw_debug(time time, game_world *world);

void update_draw_frame(game_world *world, time time);
void draw_map(void);

void update_player(game_world *world, time *time);

void update_time(game_world *world, time *time, player *player);

void show_title(game_world *world, const char *text);
bool update_title(game_world *world, time *t);
void draw_title(game_world *world);
#endif