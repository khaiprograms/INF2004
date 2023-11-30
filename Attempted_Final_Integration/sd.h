#ifndef SD_CARD_H
#define SD_CARD_H

#include "pico/stdlib.h"
#include "../pico-rmii-ethernet/no-OS-FatFS-SD-SPI-RPi-Pico-master/FatFs_SPI/sd_driver/sd_card.h"
#include "../pico-rmii-ethernet/no-OS-FatFS-SD-SPI-RPi-Pico-master/FatFs_SPI/ff14a/source/ff.h"

// Function prototypes
void sd_card_init();
void write_sd_card();
char *read_sd_card(char* filename);
void remove_sd_card();

#endif