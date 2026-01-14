#include "raylib.h"
#include "raymath.h"
 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
 
#include "header.h"
#include "world.h"
#include "entity.h"
#include "enemy.h"

void show_title(game_world *world, const char *text) {
    world->title.is_active = true;
    world->title.text = text;
    world->title.time_shown = 0;
    world->title.fade_in = 0.25;
    world->title.fade_out = 0.25;

    world -> time -> time_change_start = world -> time -> total;
}

// returns false if the title should be shown?
// returns true when title should stop showing.
bool update_title(game_world *world, time *time) {
    if (!world->title.is_active) {
        return false;
    }

    world->title.time_shown += time->true_dt;

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        if (!world->is_game_won) {
            world->title.is_active = false;
            time->time_change_start = time->total;   
            return true;   
        }
    }

    return false;
}

void draw_title(game_world *world) {
    if (!world->title.is_active){
        return;
    }

    float alpha = 1.0;
    if (world->title.fade_in > 0.0) {
        float k = (float)(world->title.time_shown / world->title.fade_in);
        k = Clamp(k, 0, 1);
        alpha = k;
    }

    Color BG_Colour = (Color){60, 60, 60, (unsigned char)(alpha * 200)};
    DrawRectangle(0, 0, world->screen.width, world->screen.height, BG_Colour);

    int font_size = 60;
    int text_width = MeasureText(world->title.text, font_size);
    if (world->is_game_won) {
        DrawText("you don't suck", (world->screen.width - text_width) / 2, (world->screen.height) / 2 - font_size / 2, font_size, (Color){220, 220, 220, (unsigned char)(alpha * 255)});
        DrawText("GGs", (world->screen.width - MeasureText("GGs", font_size)) / 2, (world->screen.height) / 2 - font_size / 2 - 60, font_size, (Color){220, 220, 220, (unsigned char)(alpha * 255)});
        DrawText(TextFormat("Deaths: %d  Damage Taken: %d  Kills: %d", world->player.deaths, world->player.damage_taken, world->player.kills), (world->screen.width - MeasureText(TextFormat("Deaths: %d  Damage Taken: %d  Kills: %d", world->player.deaths, world->player.damage_taken, world->player.kills), font_size)) / 2, (world->screen.height) / 2 - font_size / 2 + 60, font_size, (Color){220, 220, 220, (unsigned char)(alpha * 255)});
        DrawText(TextFormat("Time Taken: %.2lf", world->time->time_of_win), (world->screen.width - MeasureText(TextFormat("Time Taken: %.2lf", world->time->time_of_win), font_size)) / 2, (world->screen.height) / 2 - font_size / 2 + 120, font_size, (Color){220, 220, 220, (unsigned char)(alpha * 255)});
    
        DrawText(
            "Press ESC to quit",
            20,
            world->screen.height - 40,
            20,
            (Color){200,200,200,(unsigned char)(255*alpha)}
        );
    } else {
        DrawText(
            world->title.text,
            (world->screen.width - text_width) / 2, 
            (world->screen.height) / 2 - font_size / 2,
            font_size,
            (Color){220, 220, 220, (unsigned char)(alpha * 255)}
        );
        DrawText(
            "Press ENTER or SPACE",
            20,
            world->screen.height - 40,
            20,
            (Color){200,200,200,(unsigned char)(255*alpha)}
        );
    }
}