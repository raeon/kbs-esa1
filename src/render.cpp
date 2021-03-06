#include "render.h"

#include "defines.h"

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// Draw any tile.
void draw_tile(int x, int y, tile_t type) {
    x = WORLD_TILE_SIZE * x;
    y = WORLD_TILE_SIZE * y;

    switch (type) {
        case BOMB:
            draw_rect(x, y, ILI9341_LIGHTGREY);
            draw_circle(x, y, ILI9341_OLIVE);
            break;
        case WALL:
            draw_rect(x, y, ILI9341_DARKGREY);
            break;
        case BOX:
        case UPGRADE_BOX_BOMB_COUNT:
        case UPGRADE_BOX_BOMB_SIZE:
            draw_rect(x, y, ILI9341_BROWN);
            break;
        case EMPTY:
            draw_rect(x, y, ILI9341_BLACK);
            break;
        case EXPLODING_BOMB:
            draw_rect(x, y, ILI9341_WHITE);
            break;
        case BOMB_EXPLOSION:
            draw_rect(x, y, ILI9341_WHITE);
            draw_circle(x, y, ILI9341_OLIVE);
            break;
        case UPGRADE_EXPLOSION_BOMB_SIZE:
            draw_rect(x,y, ILI9341_WHITE);
            draw_circle(x,y, ILI9341_MAROON);
            break;
        case UPGRADE_BOMB_SIZE:
            draw_rect(x,y, ILI9341_BLACK);
            draw_circle(x, y, ILI9341_MAROON);
            break;
        case UPGRADE_EXPLOSION_BOMB_COUNT:
            draw_rect(x,y, ILI9341_WHITE);
            draw_circle(x,y, ILI9341_CASET);
            break;
        case UPGRADE_BOMB_COUNT:
            draw_rect(x,y, ILI9341_BLACK);
            draw_circle(x, y, ILI9341_CASET);
            break;
    }
}

void draw_rect(int x, int y, uint16_t color) {
    tft.fillRect(x, y, WORLD_TILE_SIZE, WORLD_TILE_SIZE, color);
}

void draw_rect(int x, int y, int width, int height, uint16_t color) {
    tft.fillRect(x, y, width, height, color);
}

void draw_circle(int x, int y, uint16_t color) {
    x += (WORLD_TILE_SIZE / 2);
    y += (WORLD_TILE_SIZE / 2);

    tft.fillCircle(x, y, (WORLD_TILE_SIZE / 2), color);
}

void draw_background(int color) {
    tft.fillScreen(color);
}

void draw_player(player_t *player) {
    uint16_t color = player->is_main
        ? (player->hit_duration ? ILI9341_ORANGE : ILI9341_RED)
        : (player->hit_duration ? ILI9341_CYAN : ILI9341_BLUE);
    draw_circle(WORLD_TILE_SIZE * player->x, WORLD_TILE_SIZE * player->y, color);
}

void draw_button(int index, char *text) {
    index *= (TOUCH_COMPONENT_HEIGHT + TOUCH_COMPONENT_PADDING);
    draw_rect(TOUCH_BUTTON_START_X, index, TOUCH_COMPONENT_WIDTH, TOUCH_COMPONENT_HEIGHT, ILI9341_BLUE);
    tft.setCursor((TOUCH_BUTTON_START_X + TOUCH_COMPONENT_PADDING), index + TOUCH_COMPONENT_PADDING);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.println(text);
}

void draw_label(int index, char *text) {
    index *= (TOUCH_COMPONENT_HEIGHT + TOUCH_COMPONENT_PADDING);
    tft.setTextSize(2);

    int16_t x, y;
    uint16_t w, h;
    tft.getTextBounds(text, 0, 0, &x, &y, &w, &h);
    tft.setCursor((tft.width() - w) / 2, index + TOUCH_COMPONENT_PADDING);
    tft.setTextColor(ILI9341_WHITE);
    tft.println(text);
}
