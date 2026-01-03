CC = gcc
CCFLAGS = -Wall -Wextra -std=c99
CFLAGS = -IC:/raylib/raylib/src -Iheaders
LDFLAGS = -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm

SRC = src/main.c src/enemy.c src/entity.c src/menu.c src/world.c
OUT = build/game.exe

all:
	$(CC) $(CCFLAGS) $(SRC) -o $(OUT) $(CFLAGS) $(LDFLAGS)

clean:
	del build\game.exe