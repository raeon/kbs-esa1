#include "game.h"

#include "defines.h"
#include "network.h"
#include "player.h"
#include "render.h"
#include "score.h"
#include "segments.h"
#include "touch.h"
#include "world.h"

volatile bool should_poll = false;
static int should_update = 0;
static world_t *world;

static uint8_t input_buttons = 0;
static int16_t input_joy_x = 0;
static int16_t input_joy_y = 0;

bool multiplayer;
// Keep track of how many game ticks the game has been running.
unsigned long game_time = 0;

game_state_t game_state = GAME_STATE_RUNNING;

/*******************
 * Local functions *
 *******************/
inline player_t *get_opponent() {
    for (int i = 0; i < world->player_count; i++) {
        if (!world->players[i]->is_main)
            return world->players[i];
    }
    return NULL;
}

inline bool has_game_ended() {
    // Check if the player has died.
    if (!(game_get_local_player())->lives)
        game_state = GAME_STATE_LOST;

    if (multiplayer) {
        if (!(get_opponent())->lives) {
            game_state = GAME_STATE_WON;
        }
    } else {
        // End the game if there are no boxes remaining.
        if (!world_get_box_count(world))
            game_state = GAME_STATE_WON;
    }

    return game_state != GAME_STATE_RUNNING;
}

inline void opponent_move(uint8_t x, uint8_t y) {
    player_t *player = get_opponent();

    uint8_t oldx = player->x;
    uint8_t oldy = player->y;

    player->x = x;
    player->y = y;

    tile_t tile = world_get_tile(world, x, y);

    if (tile & TILE_MASK_IS_UPGRADE) {
        world_set_tile(world, x, y, (tile_t)(tile & TILE_MASK_IS_EXPLODING));
    }

    world_redraw_tile(world, oldx, oldy);
    draw_player(player);
}

inline void opponent_place_bomb(uint8_t size) {
    player_t *player = get_opponent();
    player->bomb_size = size;
    int bomb_index = bomb_allowed(player, world);

    if (bomb_index < MAX_BOMB_COUNT) {
        player_place_bomb(world, player, bomb_index);
    }
}

inline void opponent_lose_live(uint8_t x, uint8_t y) {
    player_t *player = get_opponent();
    player->hit_duration = 0;

    player_on_hit(player);

    opponent_move(x, y);
}

inline void collect_nunchuck_inputs() {
    // Collect inputs.
    if (nunchuck_get_data()) {
        uint8_t x = nunchuck_joyx();
        uint8_t y = nunchuck_joyy();

        int16_t delta_x = x - (INPUT_JOY_MAX / 2);
        int16_t delta_y = (INPUT_JOY_MAX / 2) - y;

        // Make sure minute movements are not registered (deadzone).
        // We track how much we deviate from the (theoretical) center.
        if (delta_x >= INPUT_JOY_DEADZONE || delta_x <= -INPUT_JOY_DEADZONE)
            input_joy_x += delta_x;
        if (delta_y >= INPUT_JOY_DEADZONE || delta_y <= -INPUT_JOY_DEADZONE)
            input_joy_y += delta_y;

        // Make sure we register button presses.
        input_buttons |= nunchuck_cbutton() << INPUT_BUTTON_C;
        input_buttons |= nunchuck_zbutton() << INPUT_BUTTON_Z;
    }
}

inline void receive_networking_data() {
    if (multiplayer && network_available()) {
        packet_t *packet = network_receive();

        if (packet) {
            switch (packet->id) {
                case PACKET_MOVE:
                    opponent_move(packet->x, packet->y);
                    break;
                case PACKET_LOSE_LIFE:
                    opponent_lose_live(packet->x, packet->y);
                    break;
                case PACKET_PLACE_BOMB:
                    opponent_place_bomb(packet->size);
                    break;
                default:
                    break;
            }
        }
    }
}

/************************
 * Accessible functions *
 ************************/

// Initialize the game state.
void game_init(button_mode_t game_mode) {
    // Reset variables when a game is restarting.
    game_state = GAME_STATE_RUNNING;

    multiplayer = game_mode == BUTTON_MODE_MULTIPLAYER;

    game_time = 0;

    // Initialize the nunchuck.
    nunchuck_send_request();

    // Draw the world with blocks and walls.
    world = world_new(multiplayer ? 2 : 1);

    bool player1_is_host = true;
    if (multiplayer) {
        player1_is_host = world_multiplayer_generate(world, TCNT0);
    } else {
        world_generate(world, TCNT0, game_mode);
    }

    score_set_box_count(world_get_box_count(world));

    // Create the local player and show the lives on the 7-segment display.
    player_t *player1 = player_new(1, 1, player1_is_host);
    player_show_lives(player1);

    draw_player(player1);

    world->players[0] = player1;

    // Create the opponent if playing in multiplayer mode.
    if (multiplayer) {
        player_t *player2 = player_new(15, 11, !player1_is_host);

        // Set the bomb count for the opponent to the maximum amount.
        player1_is_host ? player2->bomb_count = MAX_BOMB_COUNT : player1->bomb_count = MAX_BOMB_COUNT;
        draw_player(player2);

        world->players[1] = player2;
    }
}

player_t *game_get_local_player() {
    for (int i = 0; i < world->player_count; i++) {
        if (world->players[i]->is_main)
            return world->players[i];
    }
    return NULL;
}

void game_free() {
    world_free(world);
    network_disable();
    network_clear();
    segments_hide();
}

// Update the game, or do nothing if an update hasn't been triggered.
bool game_update() {
    receive_networking_data();

    // Don't poll or update unless the timer tells us to.
    if (!should_poll)
        return false;

    if (has_game_ended())
        return false;

    should_poll = false;
    should_update++;

    collect_nunchuck_inputs();

    // Don't update unless it's time.
    if (should_update < GAME_INPUT_FACTOR)
        return false;

    // Increment game time each game update.
    game_time++;

    should_update = 0;

    // Collect the definitive inputs. These are the button inputs
    // and the most significant joystick input in one byte.
    uint8_t inputs = input_buttons;

    // Sign bit mask used to get the absolute value of the X and Y movement.
    uint16_t x_mask = input_joy_x >> 15;
    uint16_t y_mask = input_joy_y >> 15;

    // Determine what axis is more prevalent.
    if (((input_joy_x ^ x_mask) + x_mask) >= ((input_joy_y ^ y_mask) + y_mask)) {
        // The X-axis is more or equally prevalent.
        inputs |= (input_joy_x < -INPUT_JOY_THRESHOLD) << INPUT_JOY_LEFT;
        inputs |= (input_joy_x > INPUT_JOY_THRESHOLD) << INPUT_JOY_RIGHT;
    } else {
        // The Y-axis is more prevalent.
        inputs |= (input_joy_y < -INPUT_JOY_THRESHOLD) << INPUT_JOY_UP;
        inputs |= (input_joy_y > INPUT_JOY_THRESHOLD) << INPUT_JOY_DOWN;
    }

    // Reset the input trackers.
    input_buttons = 0;
    input_joy_x = 0;
    input_joy_y = 0;

    // Update the world.
    world_update(world, inputs);

    return true;
}

game_state_t game_get_state() {
    return game_state;
}

// Trigger a game-update the next time game_update() is called.
void game_trigger_update() {
    should_poll = true;
}

bool game_is_multiplayer() {
    return multiplayer;
}

unsigned long *game_get_time() {
    return &game_time;
}
