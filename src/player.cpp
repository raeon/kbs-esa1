#include "player.h"
#include "game.h"
#include "bomb.h"
#include "defines.h"
#include "render.h"
#include "segments.h"
#include "packet.h"
#include "world.h"

// Create a new player struct.
player_t *player_new(uint8_t x, uint8_t y, uint8_t is_main) {
    player_t *player = (player_t *)malloc(sizeof(player_t));
    if (!player)
        return player;

    player->x = x;
    player->y = y;
    player->lives = 3;
    player->bomb = NULL;
    player->hit_duration = 0;
    player->is_main = is_main;
    return player;
}

// Delete a player struct.
void player_free(player_t *player) {
    if (!player)
        return;

    bomb_free(player->bomb);
    free(player);
}

// Process user input and optionally rerender the player.
void player_update(world_t *world, player_t *player, uint8_t inputs) {
    uint8_t redraw = 0;

    // Decrease our invincibility.
    if (player->hit_duration) {
        player->hit_duration--;

        // If we are no longer invincible, redraw.
        if (!player->hit_duration) {
            redraw = 1;
        }
    }

    if (player->is_main) {
        if(player_move(player, inputs, world))
            redraw = 1;

        // Place a bomb if necessary.
        if (!player->bomb && inputs & (1 << INPUT_BUTTON_C)) {
            player_place_bomb(world, player);
            redraw = 1;
        }
    }

    // Redraw our player only if we have to.
    if (redraw)
        draw_player(player);
}

uint8_t player_move(player_t *player, uint8_t inputs, world_t *world){
    // Check where we are going.
    uint8_t new_x = player->x;
    uint8_t new_y = player->y;

    // Process user input.
    if (inputs & (1 << INPUT_JOY_LEFT)) {
        new_x--;
    } else if (inputs & (1 << INPUT_JOY_RIGHT)) {
        new_x++;
    } else if (inputs & (1 << INPUT_JOY_UP)) {
        new_y--;
    } else if (inputs & (1 << INPUT_JOY_DOWN)) {
        new_y++;
    }

    // Check the tile where we are going.
    tile_t new_tile = world_get_tile(world, new_x, new_y);

    // Check if we want to and can move into the new tile.
    if ((new_x != player->x || new_y != player->y)
    && (new_tile == EMPTY || new_tile == EXPLODING_BOMB)) {
        // Store where we were, so we can rerender the tile once we've moved.
        uint8_t old_x = player->x;
        uint8_t old_y = player->y;

        // Update the player position.
        player->x = new_x;
        player->y = new_y;

        // Send move player packet.
        if(player->is_main && game_is_multiplayer())
            packet_send(MOVE, player);

        // Damage the player if they are walking into an exploding bomb,
        // but only if they are not already invincible.
        if (new_tile == EXPLODING_BOMB && player_on_hit(player)) {
            LOGLN("Damage from walking into a bomb");
        }

        // Rerender the tile we came from, and render the player on top of the new tile.
        world_redraw_tile(world, old_x, old_y);
        return 1;
    } else if (world_get_tile(world, player->x, player->y) == EXPLODING_BOMB && player_on_hit(player)) {
        // If we don't want to move or we are unable to, we should check if we
        // are standing inside an explosion. If we are, we might have to take damage.
        LOGLN("Damage from standing in explosion");
    }
}

// Whenever the player should take damage, we check if they are invincible and
// deal the damage if they are not.
uint8_t player_on_hit(player_t *player) {
    if (player->hit_duration)
        return 0;

    player->hit_duration = HIT_DURATION;
    if (player->lives) {
        player->lives--;

        // Send lose live packet and update the lives display for the local player.
        if(player->is_main) {
            player_show_lives(player);
            if (game_is_multiplayer())
                packet_send(LOSE_LIVE, player);
        }
    }
    
    return 1;
}

// Show the lives of the given player on the seven segment display using TWI.
void player_show_lives(player_t *player) {
    segments_show(player->lives);
}

// Place a bomb on the map if the player doesn't already have one.
void player_place_bomb(world_t *world, player_t *player) {
    if (!player->bomb) {
        player->bomb = bomb_new(player->x, player->y);
        world_set_tile(world, player->bomb->x, player->bomb->y, BOMB);

        // Send place bomb packet.
        if(player->is_main && game_is_multiplayer())
            packet_send(PLACE_BOMB, player);
    }
}
