#ifndef RENDER_H
#define RENDER_H

#include "player.h"
#include "world.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

extern Adafruit_ILI9341 tft;

void draw_player(player_t *player);
void draw_rect(int x, int y, uint16_t color);
void draw_rect(int x, int y, int width, int height, uint16_t color);
void draw_circle(int x, int y, uint16_t color);
void draw_background(int color);

void draw_tile(int x, int y, tile_t type);

void draw_button(int index, char *text);
void draw_label(int index, char *text);

#endif /* RENDER_H */
