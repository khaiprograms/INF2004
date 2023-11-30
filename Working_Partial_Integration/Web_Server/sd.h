#ifndef SD_CARD_H
#define SD_CARD_H

#include "pico/stdlib.h"
#include "sd_card.h"
#include "ff.h"

// Function prototypes
void sd_card_init();
void write_sd_card();
char *read_sd_card(char* filename);
void remove_sd_card();

#endif