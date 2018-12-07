#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include "player.h"
#include "world.h"

typedef enum {
    MOVE = 0b000,
    LOSE_LIVE = 0b010,
    PLACE_BOMB = 0b001,
} method_t;

typedef struct {
    method_t method;
    uint8_t x;
    uint8_t y;
} packet_t;

packet_t* packet_new(method_t method, uint8_t x, uint8_t y);
void packet_free(packet_t packet);
void packet_setup(uint16_t map_seed);
void packet_send(method_t method, player_t *player);
packet_t* packet_decode(uint16_t packet);
uint8_t has_even_parity(uint16_t packet);

#endif /* PACKET_H */