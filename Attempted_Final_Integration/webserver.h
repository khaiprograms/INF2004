#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/apps/fs.h"
#include "lwip/pbuf.h"

// Constants
#define BUF_SIZE 1024
#define PORT 80
#define MAX_PATH_LENGTH 15

// Function prototypes
typedef struct HTTP_SERVER_T HTTP_SERVER_T;
char *get_default_data();
char *get_content_type();
void http_server_init();
void initWifi(const char *ssid, const char *password);
char *html_data_to_sent(char *path);
err_t http_server_send_data(void *arg, struct tcp_pcb *tpcb);
err_t http_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
err_t http_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err);

#endif
