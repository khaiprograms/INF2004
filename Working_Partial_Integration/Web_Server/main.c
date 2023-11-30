#include "webserver.h"
#include "sd.h"

// WIFI Credentials
const char WIFI_SSID[] = "SINGTEL-6XH8";
const char WIFI_PASSWORD[] = "678mtehbr7";


int main()
{
    stdio_init_all();
    initWifi(WIFI_SSID,WIFI_PASSWORD);
    sd_card_init();
    http_server_init();
    // Infinite loop
    while (1)
        ;
}
