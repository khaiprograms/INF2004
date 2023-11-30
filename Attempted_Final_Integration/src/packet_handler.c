#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "../no-OS-FatFS-SD-SPI-RPi-Pico-master/FatFs_SPI/ff14a/source/ff.h"

#define FIN_FLAG 0b00000001
#define SYN_FLAG 0b00000010
#define RST_FLAG 0b00000100
#define PSH_FLAG 0b00001000
#define ACK_FLAG 0b00010000
#define URG_FLAG 0b00100000
#define SYN_RST_FLAG (SYN_FLAG | RST_FLAG)
#define SYN_ACK_FLAG (SYN_FLAG | ACK_FLAG)
#define RST_ACK_FLAG (RST_FLAG | ACK_FLAG)
#define FIN_ACK_FLAG (FIN_FLAG | ACK_FLAG)
#define PSH_ACK_FLAG (PSH_FLAG | ACK_FLAG)
#define FIN_PSH_URG_FLAG (PSH_FLAG | FIN_FLAG | URG_FLAG)

#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_SG_TIME 28800

#define SSID "iPhone?"
#define PASSWORD "peepeepoo123!"

volatile uint32_t epoch_time;
struct repeating_timer epoch_timer;

// Structure to hold flag-value pairs
struct FlagMap
{
    int value;
    const char *name;
};

// Mapping of flags to their values and names
struct FlagMap flag_map[] = {
    {FIN_FLAG, "FIN"},
    {SYN_FLAG, "SYN"},
    {RST_FLAG, "RST"},
    {PSH_FLAG, "PSH"},
    {ACK_FLAG, "ACK"},
    {URG_FLAG, "URG"},
    {SYN_RST_FLAG, "SYN_RST"},
    {SYN_ACK_FLAG, "SYN_ACK"},
    {RST_ACK_FLAG, "RST_ACK"},
    {FIN_ACK_FLAG, "FIN_ACK"},
    {PSH_ACK_FLAG, "PSH_ACK"},
    {FIN_PSH_URG_FLAG, "FIN_PSH_URG"}};

// structure to hold ntp connection
typedef struct NTP_T_
{
    ip_addr_t ntp_server_address[4];
    struct udp_pcb *ntp_pcb;
    bool ntp_get;
} NTP_T;

// to increment time
bool increment_epoch()
{
    epoch_time++;
    return true;
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    NTP_T *state = (NTP_T *)arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if ((ip_addr_cmp(addr, &state->ntp_server_address[0]) || ip_addr_cmp(addr, &state->ntp_server_address[1]) || ip_addr_cmp(addr, &state->ntp_server_address[2]) || ip_addr_cmp(addr, &state->ntp_server_address[3])) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
        mode == 0x4 && stratum != 0)
    {
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        epoch_time = seconds_since_1970 + NTP_SG_TIME;
        state->ntp_get = 1;
    }

    pbuf_free(p);
}

// get epoch value from ntp server
int get_epoch()
{
    if (cyw43_arch_init())
    {
        printf("failed to initialise\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    while (cyw43_arch_wifi_connect_timeout_ms(SSID, PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0)
    {
        printf("Attempting to connect...\n");
    }
    NTP_T *state = (NTP_T *)calloc(1, sizeof(NTP_T));
    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    udp_recv(state->ntp_pcb, ntp_recv, state);

    ip4addr_aton("162.159.200.123", &state->ntp_server_address[0]);
    ip4addr_aton("52.148.114.188", &state->ntp_server_address[1]);
    ip4addr_aton("162.159.200.1", &state->ntp_server_address[2]);
    ip4addr_aton("167.172.70.21", &state->ntp_server_address[3]);
    state->ntp_get = 0;

    int i = 0;
    while (state->ntp_get == 0)
    {
        // printf("IP: %s\n", ip4addr_ntoa(&state->ntp_server_address[i]));
        struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
        uint8_t *req = (uint8_t *)p->payload;
        memset(req, 0, NTP_MSG_LEN);
        req[0] = 0x1b;
        udp_sendto(state->ntp_pcb, p, &state->ntp_server_address[i], NTP_PORT);
        pbuf_free(p);
        i++;
        if (i == 4)
        {
            i = 0;
        }
    }
    // printf("epoch: %d\n",epoch_time);
    free(state);
    cyw43_arch_deinit();
    add_repeating_timer_ms(-1000, increment_epoch, NULL, &epoch_timer);
    return 0;
}

// Function to check if packet is TCP with specific flag
int is_ipv4_tcp(unsigned char *buffer, size_t position)
{
    return (buffer[position] == 0x08 && buffer[position + 1] == 0x00 &&
            buffer[position + 11] == 0x06);
}

int check_flag(unsigned char *buffer, size_t position, int flag_to_check)
{
    return (buffer[position] == 0x08 && buffer[position + 1] == 0x00 &&
            buffer[position + 11] == 0x06 && buffer[position + 35] == flag_to_check);
}

// Convert selected packet fields into ASCII
char *parse_packet(unsigned char *buffer, size_t size, size_t currentPosition)
{
    int i;
    char *output = (char *)malloc(128); // Allocate memory for the output array
    time_t time = (time_t)epoch_time;
    struct tm *utc = gmtime(&time);

    for (i = currentPosition; i < size - 13; ++i)
    {
        for (int j = 0; j < sizeof(flag_map) / sizeof(flag_map[0]); ++j)
        {
            if (check_flag(buffer, i, flag_map[j].value))
            {
                snprintf(output, 128, "%02d/%02d/%04d %02d:%02d:%02d,%d.%d.%d.%d,%d,%d.%d.%d.%d,%d,%s\n", utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
                         utc->tm_hour, utc->tm_min, utc->tm_sec,
                         buffer[i + 14], buffer[i + 15], buffer[i + 16], buffer[i + 17],
                         (buffer[i + 22] << 8) | buffer[i + 23],
                         buffer[i + 18], buffer[i + 19], buffer[i + 20], buffer[i + 21],
                         (buffer[i + 24] << 8) | buffer[i + 25],
                         flag_map[j].name);
                printf("%s", output);
                return output;
            }
        }
    }
    free(output);
    return NULL;
}

size_t find_next_packet(unsigned char *buffer, size_t bufferSize, size_t startFrom)
{
    for (size_t i = startFrom + 1; i < bufferSize - 1; ++i) // start searching from startFrom + 1
    {
        if (is_ipv4_tcp(buffer, i))
        {
            return i;
        }
    }
    return bufferSize; // Indicates no more packets found
}

// Function to detect SYN packet
void detect_syn_flood(unsigned char *packet)
{
    if ((strstr(packet, "SYN") != NULL) && (strstr(packet,"SYN_RST")==NULL)&& (strstr(packet,"SYN_ACK")==NULL))
    {
        static int sync_counter = 0;
        static char first_time[20];
        static char first_dest_ip[16];
        char current_time[20];
        char src_ip[16];
        int src_port;
        char dest_ip[16];
        int dest_port;
        char flag[4];
        sscanf(packet, "%19[^,],%15[^,],%d,%15[^,],%d,%3s", &current_time, &src_ip, src_port, &dest_ip, dest_port, flag);
        if ((sync_counter == 0) || (strcmp(first_time, current_time) != 0))
        {
            strcpy(first_time, current_time);
            strcpy(first_dest_ip, dest_ip);
            sync_counter = 0;
        }
        if (strcmp(first_dest_ip, dest_ip) == 0)
        {
            sync_counter++;
        }
        // uncomment to see information
         printf("Time: %d\n", time);
         printf("Source IP: %s\n", src_ip);
         printf("Source Port: %d\n", src_port);
         printf("Destination IP: %s\n", dest_ip);
         printf("Destination Port: %d\n", dest_port);
         printf("Flag: %s\n", flag);
    }
}