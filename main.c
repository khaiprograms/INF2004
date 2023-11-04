#include "webserver.h"

// WIFI Credentials
const char WIFI_SSID[] = "SINGTEL-CB36";
const char WIFI_PASSWORD[] = "thumaejeye";


int main()
{
    stdio_init_all();
    initWifi(WIFI_SSID,WIFI_PASSWORD);
    http_server_init();
    // Infinite loop
    while (1)
        ;
}