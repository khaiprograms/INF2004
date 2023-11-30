#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "lwip/pbuf.h"
#include "../no-OS-FatFS-SD-SPI-RPi-Pico-master/FatFs_SPI/ff14a/source/ff.h"


// Function declarations
bool increment_epoch(void);
// void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
int get_epoch(void);
int is_ipv4_tcp(unsigned char *buffer, size_t position);
int check_flag(unsigned char *buffer, size_t position, int flag_to_check);
char *parse_packet(unsigned char *buffer, size_t size, size_t currentPosition);
size_t find_next_packet(unsigned char *buffer, size_t bufferSize, size_t startFrom);
void detect_syn_flood(unsigned char *packet);
void sd_card_init();
FRESULT initialize_filesystem(FATFS *fs);
FRESULT open_file_append(FIL *file, const char *filename);
int write_to_file(FIL *file, const char *data);


#endif // PACKET_HANDLER_H
