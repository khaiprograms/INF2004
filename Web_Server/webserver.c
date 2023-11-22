#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/apps/fs.h"
#include "lwip/pbuf.h"
#include "sd.h"

#define BUF_SIZE 4096
#define PORT 80
#define MAX_PATH_LENGTH 15

// Structure to hold server and client state
typedef struct HTTP_SERVER_T
{
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    char recv_path[BUF_SIZE];
    char send_data[BUF_SIZE];
} HTTP_SERVER_T;

// Function to determine the content type based on file extension or path
char *get_default_data()
{
    return "<!DOCTYPE html><html><head><title>Default HTML Page </title></head><body><h1>Default HTML Page</h1></body></html>";
}

char *get_alert_data_sd() {
    char *alert_output = read_sd_card("alert.txt");

    if (alert_output == NULL) {
        fprintf(stderr, "Error reading alert file.\n");
        return NULL;
    }
    
    char *html_string = read_sd_card("webpage2.html");
    if (html_string == NULL){
        return get_default_data();
    }

    char *row_html_string = "<tr><td>%d</td><td>%s</td></tr>";
    char *remaining_html_string = "</tbody></table></div></div></div></body></html>";

    int count = 0;
    size_t total_length = 0;
    size_t buffer_size = 4096; // Initial buffer size
    char *concatenated_html = (char *)malloc(buffer_size * sizeof(char));

    if (concatenated_html == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    char *line = strtok(alert_output, "\n"); // Get the first line

    while (line != NULL) {
        count++;

        // Format each row
        int row_length = snprintf(concatenated_html + total_length, buffer_size - total_length, row_html_string, count, line);

        if (row_length < 0 || (size_t)row_length >= buffer_size - total_length) {
            fprintf(stderr, "Row length error or insufficient buffer size.\n");
            free(concatenated_html);
            return NULL;
        }

        total_length += row_length;

        line = strtok(NULL, "\n"); // Move to the next line
    }


    // Allocate memory for the final HTML
    size_t final_length = total_length + strlen(html_string) + strlen(remaining_html_string) + 1;
    char *html = (char *)malloc(final_length * sizeof(char));
    if (html == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(concatenated_html);
        return NULL;
    }

    // Construct the final HTML by combining html_string, concatenated_html, and remaining_html_string
    snprintf(html, final_length, "%s%s%s", html_string, concatenated_html, remaining_html_string);

    // Free memory allocated for concatenated_html
    free(concatenated_html);

    return html;
}

char *get_data_sd() {
    char *log_output = read_sd_card("log_output.txt");
    // char *alert_output = read_sd_card("alert.txt");

    if (log_output == NULL) {
        fprintf(stderr, "Error reading output file.\n");
        return NULL;
    }

    char src_addr[20]; // Assuming IP addresses can be up to 20 characters
    char dst_addr[20];
    char flag[10]; // Assuming packet type can be up to 10 characters
    char timestamp[20];
    int src_port, dst_port;

    char *html_string = read_sd_card("webpage.html");
    if (html_string == NULL){
        return get_default_data();
    }

    char *row_html_string = "<tr><td>%d</td><td>%s</td><td>%s</td><td>%d</td><td>%s</td><td>%d</td><td>%s</td></tr>";
    char *remaining_html_string = "</tbody></table></div></div></div></body></html>";

    int count = 0;
    size_t total_length = 0;
    size_t buffer_size = 4096; // Initial buffer size
    char *concatenated_html = (char *)malloc(buffer_size * sizeof(char));

    if (concatenated_html == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    char *line = strtok(log_output, "\n"); // Get the first line

    while (line != NULL) {
        count++;
        sscanf(line, "%19[^,], %19[^,], %d, %19[^,], %d, %9s", timestamp, src_addr, &src_port, dst_addr, &dst_port, flag);

        // Format each row
        int row_length = snprintf(concatenated_html + total_length, buffer_size - total_length, row_html_string, count, timestamp, src_addr, src_port, dst_addr, dst_port, flag);
        printf("TIMESTAMP: %s\n", timestamp);
        if (row_length < 0 || (size_t)row_length >= buffer_size - total_length) {
            fprintf(stderr, "Row length error or insufficient buffer size.\n");
            free(concatenated_html);
            return NULL;
        }

        total_length += row_length;

        line = strtok(NULL, "\n"); // Move to the next line
    }


    // Allocate memory for the final HTML
    size_t final_length = total_length + strlen(html_string) + strlen(remaining_html_string) + 1;
    char *html = (char *)malloc(final_length * sizeof(char));
    if (html == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(concatenated_html);
        return NULL;
    }

    // Construct the final HTML by combining html_string, concatenated_html, and remaining_html_string
    snprintf(html, final_length, "%s%s%s", html_string, concatenated_html, remaining_html_string);

    // Free memory allocated for concatenated_html
    free(concatenated_html);

    return html;
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

// Callback function to send HTTP response data
err_t http_server_send_data(void *arg, struct tcp_pcb *tpcb)
{
    HTTP_SERVER_T *state = (HTTP_SERVER_T *)arg;
    char *response_data;
    // char *response_data = get_default_data();
    printf("RECV PATH %s\n", state->recv_path);

    if (strcmp(state->recv_path, "/") == 0){
        response_data = get_data_sd();
    }else if (strcmp(state->recv_path, "/alert_page.html") == 0){
        response_data = get_alert_data_sd();
    }

    char *content_type = get_content_type(state->recv_path);

    printf("data:%s\n",response_data);
    printf("content-type:%s\n",content_type);
    printf("strlen:%d", strlen(response_data));

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

// Callback function for receiving HTTP requests
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
        // Check if the received data contains an HTTP GET request
        if (data_len >= 3 && strncmp(data, "GET", 3) == 0)
        {
            // Handle the HTTP GET request
            // Extract the requested path from the request
            // Find the end of the path (usually ends with HTTP/1.x)
            char *path_start = data + 4; // Skip "GET "
            char *path_end = strstr(path_start, " HTTP/1.");
            //char path[MAX_PATH_LENGTH];
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

// Callback function for handling accepted connections
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

// Initialize the HTTP server
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

// Initialize Wi-Fi connection
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
