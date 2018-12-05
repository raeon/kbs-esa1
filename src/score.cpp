#include "score.h"
#include "defines.h"
#include "game.h"
#include "player.h"

uint8_t total_boxes = 0;
unsigned long total_time = 0;
float boxes_per_second = 0;

void score_calculate() {
    int score_part_one = 0;
    int score_part_two = 0;

    total_time = *game_get_time();
    uint8_t lives_left = game_get_main_player()->lives;

    total_time /= GAME_UPDATE_FREQUENCY;
    boxes_per_second = (float)total_boxes / (float)total_time;

    boxes_per_second *= lives_left;

    score_part_one = boxes_per_second;
    score_part_two = (boxes_per_second - score_part_one) * 100;

    Serial.println(score_part_one);
    Serial.println(score_part_two);

    score_calculate_position(score_part_one, score_part_two);
    eeprom_show();
}

void score_set_boxes(uint8_t boxes) {
    total_boxes = boxes;
}

void score_calculate_position(uint8_t part_one, uint8_t part_two){
    for(uint16_t i=0; i<6; i+=2){
        if(eeprom_read_byte(i) < part_one || (eeprom_read_byte(i) == part_one && eeprom_read_byte(i+1) < part_two)){
            eeprom_write_byte(i, part_one);
            eeprom_write_byte(i+1, part_two);
            LOG("Highscore set to place: ");
            Serial.println(i);
            return;
        }
    }
}

void eeprom_write_byte(uint16_t addr, uint8_t data) {
    // Wait untill we are allowed to write.
    while (EECR & (1 << EEPE));
    EEAR = addr;
    EEDR = data;
    EECR |= (1 << EEMPE);
    EECR |= (1 << EEPE);
}

uint8_t eeprom_read_byte(uint16_t addr) {
    // Wait untill we are allowed to read.
    while (EECR & (1 << EEPE));
    EEAR = addr;
    EECR |= (1 << EERE);
    return EEDR;
}

void eeprom_show(){
    for(uint16_t i=0; i<6; i++){
        Serial.println(eeprom_read_byte(i));
    }
}
