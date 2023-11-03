#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/apps/fs.h"
#include "lwip/pbuf.h"

#include <stdio.h>
#include "ff.h"
#include "sd_card.h"

#define BUF_SIZE 1024
#define PORT 80
#define MAX_PATH_LENGTH 15

const char WIFI_SSID[] = "Redmi";
const char WIFI_PASSWORD[] = "abcdef123";

typedef struct TCP_SERVER_T
{
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    char recv_data[BUF_SIZE];
    char send_data[BUF_SIZE];
} TCP_SERVER_T;

static TCP_SERVER_T *tcp_server_init()
{
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state)
    {
        printf("failed to allocate state\n");
        return NULL;
    }
    return state;
}

void initWifi()
{
    cyw43_arch_init();

    cyw43_arch_enable_sta_mode(); // Allows pico to connect to existing network
}
void connectWifi()
{
    // Connect to the WiFI network - loop until connected
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0)
    {
        printf("Attempting to connect...\n");
    }
    // Print a success message once connected with IP of the Pico
    printf("Connected! \n");
    printf("IP: %s\n", ip4addr_ntoa(netif_ip_addr4(netif_default)));
}

char* read_sd_card_file(FRESULT fr, char* filename){
    printf("%s\n", filename);
    FIL fil;
    char buf[100];

    // Dynamically allocate memory for allContents
    char* allContents = (char*)malloc(1024 * sizeof(char));
    if (allContents == NULL) {
        printf("ERROR: Memory allocation failed\r\n");
        return NULL; // Return NULL to indicate an error
    }

    allContents[0] = '\0'; // Initialize the char array with an empty string

    // Open file for reading
    fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
        free(allContents); // Free the dynamically allocated memory
        return NULL; // Return NULL to indicate an error
    }

    // Print every line in file over serial
    printf("Reading from file '%s':\r\n", filename);
    printf("---\r\n");
    while (f_gets(buf, sizeof(buf), &fil)) {
        strcat(allContents, buf);
        printf(buf);
    }
    printf("\r\n---\r\n");

    // Close file
    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
        free(allContents); // Free the dynamically allocated memory
        return NULL; // Return NULL to indicate an error
    }

    return allContents;
}

char *html_data_to_sent(char* path){
    FRESULT fr;
    FATFS fs;
    char buf[2];

    // Wait for user to press 'enter' to continue
    printf("\r\nSD card test. Press 'enter' to start.\r\n");
    while (true) {
        buf[0] = getchar();
        if ((buf[0] == 'A')) {
            break;
        }
    }

    // Initialize SD card
    if (!sd_init_driver()) {
        printf("ERROR: Could not initialize SD card\r\n");
        while (true);
    }

    // Mount drive
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
        while (true);
    }

    if (strcmp(path, "/") == 0) {
        printf("Root path requested\n");
        //TODO: read html from sd card and return value
        //  return "<!DOCTYPE html>"
        //       "<html>"
        //       "<head> <title>CNX Software's Pico W </title> </head>"
        //       " <body> <h1>Network Analyser</h1>"
        //       "</body>"
        //       "</html>";

        char* output = read_sd_card_file(fr, "index.html");
        return output;

    } else if (strcmp(path, "/save") == 0) {
        printf("Specific path requested\n");
        //TODO: read write from sd card and return value
       return "<!DOCTYPE html>"
              "<html>"
              "<head> <title>CNX Software's Pico W </title> </head>"
              " <body> <h1>Saved to pacap</h1>"
              "</body>"
              "</html>";
    } else {
        printf("Unknown path\n");
        return "<!DOCTYPE html>"
              "<html>"
              "<head> <title>CNX Software's Pico W </title> </head>"
              " <body> <h1>404</h1>"
              "</body>"
              "</html>";
    }
}

err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb)
{
    TCP_SERVER_T *state = (TCP_SERVER_T *)arg;
    
    char *html = html_data_to_sent(state->recv_data);
    printf(html);
    int content_length = strlen(html);
    
    // Create the HTTP response message
    snprintf(state->send_data, BUF_SIZE,
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"
             "\r\n"
             "%s",
             "200 OK", "text/html", content_length, html);
    
    // Send the response over the TCP connection
    tcp_write(tpcb, state->send_data, strlen(state->send_data), 0);
    tcp_output(tpcb);
    return ERR_OK;
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    TCP_SERVER_T *state = (TCP_SERVER_T *)arg;
    if (!p)
    {
        return ERR_VAL;
    }

    if (p->tot_len > 0)
    {
        //retrieve http request from buffer
        char *data = (char *)p->payload;
        int data_len = p->tot_len;
        // printf(data);
        // Check if the received data contains an HTTP GET request
        if (data_len >= 3 && strncmp(data, "GET", 3) == 0)
        {
            // Handle the HTTP GET request
            // Extract the requested path from the request
            // Find the end of the path (usually ends with HTTP/1.x)
            const char *path_start = data + 4; // Skip "GET "
            const char *path_end = strstr(path_start, " HTTP/1.");
            // char path[MAX_PATH_LENGTH];
            if (path_end == NULL)
            {
                printf("Invalid HTTP request format\n");
                return ERR_VAL;
            }
            // printf("path start: %s\n", path_start);
            // printf("path end: %s\n", path_end);

            int path_length = path_end - path_start;
            // printf("path length: %d\n",path_length);
            // Copy the path to the provided buffer
            strncpy(state->recv_data, path_start, path_length);
            state->recv_data[path_length] = '\0'; // Null-terminate the path

            printf("Extracted path: %s\n", state->recv_data);
            // Here, you can add code to parse the request and extract the path
            // printf("Received HTTP GET request for path: %s\n", requested_path);

            // You can then respond with the appropriate content
            // const char* html_response = "<html><body><h1>Hello, HTTP GET Request!</h1></body></html>";
            // http_server_send(tpcb, html_response);
        }
    }
    pbuf_free(p);

    return  tcp_server_send_data(arg, state->client_pcb);
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err)
{
    TCP_SERVER_T *state = (TCP_SERVER_T *)arg;
    if (err != ERR_OK || client_pcb == NULL)
    {
        printf("failed to accept\n");
        return ERR_VAL;
    }
    printf("client connected\n");

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    // tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);

    return ERR_OK;
}

int main()
{
    stdio_init_all();
    initWifi();
    connectWifi();

    TCP_SERVER_T *state = tcp_server_init();
    struct tcp_pcb *pcb = tcp_new();
    err_t err = tcp_bind(pcb, IP_ADDR_ANY, PORT);
    if (err)
    {
        printf("failed to bind to port %d\n");
        return false;
    }
    printf("binded to port 80\n");
    state->server_pcb = tcp_listen(pcb);
    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);
    printf("listening...\n");
    /* Infinite loop */
    while (1)
        ;
}
