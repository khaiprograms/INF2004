#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/apps/fs.h"
#include "lwip/pbuf.h"
#include "sd.h"

#define BUF_SIZE 4096
#define PORT 80
#define MAX_PATH_LENGTH 15
#define MAX_SETS 10         // Maximum number of sets
#define LINES_PER_SET 10    // Number of lines per set
// Max length for the resulting string containing the HTML content
#define MAX_HTML_STRING_LENGTH 300

// Structure to hold server and client state
typedef struct HTTP_SERVER_T
{
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    char recv_path[BUF_SIZE];
    char send_data[BUF_SIZE];
} HTTP_SERVER_T;

// Function to determine the content type based on file extension or path
char *get_default_data() {
    return "<!DOCTYPE html><html><head><title>Default HTML Page </title></head><body><h1>Default HTML Page</h1></body></html>";
}

char *get_alert_data_sd() {
    char *alert_output = read_sd_card("alert.txt");
    if (alert_output == NULL) {
        fprintf(stderr, "Error reading alert file.\n");
        return get_default_data();
    }

    char *html_string = read_sd_card("webpage2.html");
    if (html_string == NULL) {
        return get_default_data();
    }

    // HTML templates
    char *row_html_string = "<tr><td>%d</td><td>%s</td></tr>";
    char *remaining_html_string = "</tbody></table></div></div></div></body></html>";

    // Processing alert data to construct HTML rows
    int count = 0;
    size_t total_length = 0;
    char *concatenated_html = (char *)malloc(BUF_SIZE * sizeof(char));
    if (concatenated_html == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    char *line = strtok(alert_output, "\n");
    while (line != NULL) {
        count++;

        int row_length = snprintf(concatenated_html + total_length, BUF_SIZE - total_length, row_html_string, count, line);
        if (row_length < 0 || (size_t)row_length >= BUF_SIZE - total_length) {
            fprintf(stderr, "Row length error or insufficient buffer size.\n");
            free(concatenated_html);
            return NULL;
        }

        total_length += row_length;
        line = strtok(NULL, "\n");
    }

    // Allocate memory for the final HTML
    size_t final_length = total_length + strlen(html_string) + strlen(remaining_html_string) + 1;
    char *html = (char *)malloc(final_length * sizeof(char));
    if (html == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(concatenated_html);
        return NULL;
    }

    // Construct the final HTML
    snprintf(html, final_length, "%s%s%s", html_string, concatenated_html, remaining_html_string);

    // Clean up and return the HTML string
    free(concatenated_html);
    return html;
}

char **get_data_sd() {
    char *log_output = read_sd_card("log_output.txt");
    if (log_output == NULL) {
        fprintf(stderr, "Error reading output file.\n");
        return NULL;
    }

    char src_addr[20]; // Assuming IP addresses can be up to 20 characters
    char dst_addr[20];
    char flag[10]; // Assuming packet type can be up to 10 characters
    char timestamp[20];
    int src_port, dst_port;

    char** sets_array = malloc(MAX_SETS * sizeof(char*)); // Allocate memory for MAX_SETS pointers to strings
    int set_count = 0;

    char *row_html_string = "<tr><td>%d</td><td>%s</td><td>%s</td><td>%d</td><td>%s</td><td>%d</td><td>%s</td></tr>";

    int count = 0;
    size_t total_length = 0;
    size_t buffer_size = BUF_SIZE; // Initial buffer size
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

        if (row_length < 0 || (size_t)row_length >= buffer_size - total_length) {
            fprintf(stderr, "Row length error or insufficient buffer size.\n");
            free(concatenated_html);
            return NULL;
        }

        total_length += row_length;

        // Check if 10 lines have been processed or if it's the last iteration
        if (count % LINES_PER_SET == 0 || line == NULL) {
            // Allocate memory for the concatenated_html string for this set
            sets_array[set_count] = (char *)malloc((total_length + 1) * sizeof(char));
            if (sets_array[set_count] == NULL) {
                fprintf(stderr, "Memory allocation failed.\n");
                free(concatenated_html);
                // free(html_string);
                return NULL;
            }

            // Copy concatenated_html to the sets_array
            strncpy(sets_array[set_count], concatenated_html, total_length);
            sets_array[set_count][total_length] = '\0';  // Null-terminate the string

            // Reset total_length for the next set
            total_length = 0;

            set_count++;  // Increment set count

            if (set_count >= MAX_SETS) {
                fprintf(stderr, "Maximum number of sets reached.\n");
                break;  // Break the loop if the maximum number of sets is reached
            }
        }
        
        line = strtok(NULL, "\n"); // Move to the next line
    }

        // Check if we've processed all lines and it's not a complete set
    if (line == NULL && count % LINES_PER_SET != 0) {
        // Allocate memory for the partial set
        sets_array[set_count] = (char *)malloc((total_length + 1) * sizeof(char));
        if (sets_array[set_count] == NULL) {
            fprintf(stderr, "Memory allocation failed.\n");
            free(concatenated_html);
            // free(html_string);
            return NULL;
        }

        // Copy concatenated_html to the sets_array for the partial set
        strncpy(sets_array[set_count], concatenated_html, total_length);
        sets_array[set_count][total_length] = '\0';  // Null-terminate the string

        set_count++;  // Increment set count
    }
    return sets_array;
}

char *access_page(int current_page){
    char *html_string = read_sd_card("webpage.html");
    if (html_string == NULL) {
        return get_default_data();
    }

    char **sets_array = get_data_sd();
    int max_page = sizeof(sets_array)-1;
    char *lines_of_logs = sets_array[current_page-1];
    free(sets_array);
    char *html_table_tags = "</tbody></table>";

    // Create a string to hold the HTML content
    char html_page_btn[MAX_HTML_STRING_LENGTH];
    // Format the HTML string with the page number
    snprintf(html_page_btn, MAX_HTML_STRING_LENGTH, "<div class='d-flex justify-content-center mt-3'><a href='page_%d.html' class='btn btn-primary ml-2'>Previous</a><div class='align-self-center'> %d/%d </div><a href='page_%d.html' class='btn btn-primary ml-2'>Next</a></div>", current_page-1, current_page, max_page, current_page+1);
    char *remaining_html_string = "</div></div></div></body></html>";

    // Allocate memory for the final HTML
    size_t final_length = strlen(lines_of_logs) + strlen(html_string) + strlen(html_table_tags) + strlen(html_page_btn) + strlen(remaining_html_string) + 1;
    char *html = (char *)malloc(final_length);
    if (html == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(lines_of_logs);
        return NULL;
    }

    snprintf(html, final_length, "%s%s%s%s%s", html_string, lines_of_logs, html_table_tags, html_page_btn, remaining_html_string);

    // Free memory allocated for lines_of_logs
    free(html_string);
    free(lines_of_logs);

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

    char *path = state->recv_path;
    printf("RECV PATH %s\n", path);
    char *search_page = "/page_";

    if (strcmp(path, "/") == 0) {
        // response_data = get_data_sd();
        response_data = access_page(1);
    } else if (strncmp(path, search_page, strlen(search_page)) == 0 && strstr(path, ".html") != NULL) {
        char *value_str = path + strlen(search_page); // Get the substring after "/page_"
        int page_number = atoi(value_str); // Convert the substring to an integer
        printf("The page number is: %d\n", page_number);
        response_data = access_page(page_number);
    } else if (strcmp(path, "/alert_page.html") == 0) {
        response_data = get_alert_data_sd();
    }


    char *content_type = get_content_type(path);

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
    response_data = NULL;
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
