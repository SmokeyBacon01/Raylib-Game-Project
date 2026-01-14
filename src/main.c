// gcc src\main.c src\enemy.c src\entity.c src\menu.c src\world.c -o build\game.exe -I C:\raylib\raylib\src -Iheaders -L C:\raylib\raylib\src -lraylib -lopengl32 -lgdi32 -lwinmm
// .\build\game.exe
 
 
// TODO
// Add collision logic
    // - hurtboxes
    // - try messing with active/inactive hurtboxes
 
#include "raylib.h"
#include "raymath.h"
 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
 
#include "header.h"
#include "world.h"
#include "entity.h"
#include "enemy.h"

void black_magic(game_world *world) {
    if (IsKeyPressed(KEY_L)) {
        for (int i = 0; i < world->objects.enemies.count; i++) {
            damage_enemy(&world->objects, &world->player, world->objects.enemies.list[i].id, 999);
        }
    }
}

int main(int argc, char *argv[]) {
 
    if (argc != 1) {
        InitWindow(800, 500, "debug");  
    } else {
        SetConfigFlags(FLAG_FULLSCREEN_MODE);
        InitWindow(0, 0, "raylib");
    }

    // silence compiler warning
    (void)argv[0];
 
    
    // Initialisations
    time time;
    initialise_time(&time);
    

    game_world *world = malloc(sizeof(game_world));
    double wave_cooldown = WAVE_INTERMISSION;
    
    initialise_hurtboxes(&world->objects);
    initialise_hitboxes(&world->objects);
    initialise_world(world, &time);

    InitAudioDevice(); 

    Music music = LoadMusicStream("resources/STAGER_pcm.wav");
    // if (music.ctxData == NULL) {
    //     TraceLog(LOG_ERROR, "LoadMusicStream failed for %s", rel);
    // }

    float time_played = 0.0f;
    (void)time_played; // silence compiler

    bool pause = false;    
    float pan = 0.5f;
    SetMusicPan(music, pan);

    float volume = 0.05f;
    SetMusicVolume(music, volume);
    SetMasterVolume(1.0f);

    PlayMusicStream(music);

    SetTargetFPS(FPS);  

    spawn_appropriate_wave(world);
 
    // Main gameplay loop.
    while (!WindowShouldClose()) {

        UpdateMusicStream(music);
        
        if (IsKeyPressed(KEY_M)){
            pause = !pause;
            if (pause) PauseMusicStream(music);
            else ResumeMusicStream(music);
        }

        if (world->is_game_won && !world->title.is_active) {
            show_title(world, "you dont suck");
        }

        update_time(world, &time, &world->player);

        if (world->player.health <= 0 && !world->title.is_active){
            show_title(world, "you suck");
            world->player.movement_state = DEAD;
            world->player.deaths++;
        }

        if (update_title(world, &time) && world->player.health <= 0) {
            reset_game(world, &time);
            continue;
        }

        waves(world, time, &wave_cooldown);
        update_player(world, &time);
        //DEBUG
        black_magic(world);

        update_enemies(world, &time);
        update_hitboxes(world);
        update_hurtboxes(&world->objects, time);

        update_camera(world, time);
        update_draw_frame(world, time);
    }
    
    UnloadMusicStream(music);
    CloseAudioDevice();

    CloseWindow();

    free(world);
    return 0;
}