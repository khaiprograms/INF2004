#include "webserver.h"
#include "sd.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "lwip/dhcp.h"
#include "lwip/init.h"
#include "lwip/apps/httpd.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "hardware/clocks.h"

#include "lwip/dhcp.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"

#include "lwip/apps/httpd.h"

#include "../pico-rmii-ethernet/src/include/rmii_ethernet/netif.h"

// WIFI Credentials
const char WIFI_SSID[] = "iPhone?";
const char WIFI_PASSWORD[] = "peepeepoo123!";

void netif_link_callback(struct netif *netif)
{
    printf("netif link status changed %s\n", netif_is_link_up(netif) ? "up" : "down");
}

void netif_status_callback(struct netif *netif)
{
    printf("netif status changed %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
}

int main() {   
    printf("test");
    stdio_init_all();
    printf("test2");
    lwip_init();
    printf("test3");
    initWifi(WIFI_SSID, WIFI_PASSWORD);
    printf("test4");
    sleep_ms(5000);
    sd_card_init();
    printf("test5");
    http_server_init();
    printf("test6");
    sleep_ms(5000);
    struct netif netif;

    //
    struct netif_rmii_ethernet_config netif_config = {
        pio0, // PIO:            0
        0,    // pio SM:         0 and 1
        6,    // rx pin start:   6, 7, 8    => RX0, RX1, CRS
        10,   // tx pin start:   10, 11, 12 => TX0, TX1, TX-EN
        14,   // mdio pin start: 14, 15   => ?MDIO, MDC
        NULL, // MAC address (optional - NULL generates one based on flash id) 
    };

    // change the system clock to use the RMII reference clock from pin 20
    clock_configure_gpin(clk_sys, 20, 50 * MHZ, 50 * MHZ);
    sleep_ms(100);

    // initialize stdio after the clock change
    printf("test");
    stdio_init_all();
    printf("test2");
    lwip_init();
    printf("test3");
    initWifi(WIFI_SSID, WIFI_PASSWORD);
    printf("test4");
    sleep_ms(5000);
    sd_card_init();
    printf("test5");
    http_server_init();
    printf("test6");
    sleep_ms(5000);
    
    printf("pico rmii ethernet - httpd\n");

    // initialize the PIO base RMII Ethernet network interface
    //netif_rmii_ethernet_init(&netif, &netif_config);
    printf("test of skill");
    
    // assign callbacks for link and status
    netif_set_link_callback(&netif, netif_link_callback);
    netif_set_status_callback(&netif, netif_status_callback);

    // set the default interface and bring it up
    netif_set_default(&netif);
    netif_set_up(&netif);

    //multicore_launch_core1(netif_rmii_ethernet_loop);

    // Start DHCP client and httpd
    //dhcp_start(&netif);
    //httpd_init();

    while (1) 
    {
        tight_loop_contents();
    }

    return 0;
}
