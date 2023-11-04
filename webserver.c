#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/apps/fs.h"
#include "lwip/pbuf.h"

#define BUF_SIZE 1024
#define PORT 80
#define MAX_PATH_LENGTH 15

typedef struct HTTP_SERVER_T
{
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    char recv_path[BUF_SIZE];
    char send_data[BUF_SIZE];
} HTTP_SERVER_T;

char *get_default_data()
{
    return "<!DOCTYPE html><html><head><title>Default HTML Page </title></head><body><h1>Default HTML Page</h1></body></html>";
}

// Function to determine the content type based on file extension or path
char *get_content_type(char *file_path) {
    if (strcmp(file_path, "/") == 0) {
        return "text/html"; // Default content type for the root path
    }

    const char *extension = strrchr(file_path, '.'); // Find the last dot in the file path

    if (extension) {
        extension++; // Move past the dot
        if (strcmp(extension, "html") == 0) {
            return "text/html";
        } else if (strcmp(extension, "css") == 0) {
            return "text/css";
        } else if (strcmp(extension, "js") == 0) {
            return "application/javascript";
        } else if (strcmp(extension, "jpg") == 0 || strcmp(extension, "jpeg") == 0) {
            return "image/jpeg";
        } else if (strcmp(extension, "png") == 0) {
            return "image/png";
        } else if (strcmp(extension, "gif") == 0) {
            return "image/gif";
        } else if (strcmp(extension, "pdf") == 0) {
            return "application/pdf";
        }
    }

    // Default content type for unknown file extensions
    return "application/octet-stream";
}

err_t http_server_send_data(void *arg, struct tcp_pcb *tpcb)
{
    HTTP_SERVER_T *state = (HTTP_SERVER_T *)arg;

    char *response_data = get_default_data();
    char *content_type = get_content_type(state->recv_path);

    printf("data:%s\n",response_data);
    printf("content-type:%s\n",content_type);
    
    // Create the HTTP response message
    snprintf(state->send_data, BUF_SIZE,
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"
             "\r\n"
             "%s",
             "200 OK", content_type, strlen(response_data), response_data);

    // Send the response over the TCP connection
    tcp_write(tpcb, state->send_data, strlen(state->send_data), 0);
    tcp_output(tpcb);
    return ERR_OK;
}

err_t http_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    HTTP_SERVER_T *state = (HTTP_SERVER_T *)arg;
    if (!p)
    {
        return ERR_VAL;
    }

    if (p->tot_len > 0)
    {
        // retrieve http request from buffer
        char *data = (char *)p->payload;
        int data_len = p->tot_len;
        // printf(data);
        // Check if the received data contains an HTTP GET request
        if (data_len >= 3 && strncmp(data, "GET", 3) == 0)
        {
            // Handle the HTTP GET request
            // Extract the requested path from the request
            // Find the end of the path (usually ends with HTTP/1.x)
            char *path_start = data + 4; // Skip "GET "
            char *path_end = strstr(path_start, " HTTP/1.");
            char path[MAX_PATH_LENGTH];
            if (path_end == NULL)
            {
                printf("Invalid HTTP request format\n");
                return ERR_VAL;
            }

            int path_length = path_end - path_start;
            // Copy the path to the provided buffer
            strncpy(state->recv_path, path_start, path_length);
            state->recv_path[path_length] = '\0'; // Null-terminate the path
        }
    }
    pbuf_free(p);

    return http_server_send_data(arg, state->client_pcb);
}

static err_t http_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err)
{
    HTTP_SERVER_T *state = (HTTP_SERVER_T *)arg;
    if (err != ERR_OK || client_pcb == NULL)
    {
        printf("failed to accept\n");
        return ERR_VAL;
    }
    printf("client connected\n");

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_recv(client_pcb, http_server_recv);

    return ERR_OK;
}

void http_server_init()
{
    HTTP_SERVER_T *state = calloc(1, sizeof(HTTP_SERVER_T));
    if (!state)
    {
        printf("failed to allocate state\n");
        exit(1);
    }

    struct tcp_pcb *pcb = tcp_new();
    err_t err = tcp_bind(pcb, IP_ADDR_ANY, PORT);
    if (err)
    {
        printf("failed to bind to port %d\n");
        exit(1);
    }
    printf("binded to port 80\n");
    state->server_pcb = tcp_listen(pcb);
    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, http_server_accept);
    printf("listening...\n");
}

void initWifi(const char *ssid, const char *password)
{
    cyw43_arch_init();

    cyw43_arch_enable_sta_mode(); // Allows pico to connect to existing network
    // Connect to the WiFI network - loop until connected
    while (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0)
    {
        printf("Attempting to connect...\n");
    }
    // Print a success message once connected with IP of the Pico
    printf("Connected! \n");
    printf("IP: %s\n", ip4addr_ntoa(netif_ip_addr4(netif_default)));
}
