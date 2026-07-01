#include "wifi_setup.h"
#include "pin_config.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
extern "C" {
#include "dhcpserver.h"
}
#include <stdio.h>
#include <string.h>

// ============================================================================
// wifi_setup.cpp
//
// Brings up the CYW43 WiFi chip in Access Point mode using lwIP poll mode.
//
// Poll mode means there are NO background IRQ threads — the stack only
// processes events when cyw43_arch_poll() is called.  This is correct for
// this project because Core 0 has nothing else to do in its loop.
// ============================================================================

// DHCP server state (lwIP bundled dhcpserver)
static dhcp_server_t s_dhcp_server;

bool wifi_init() {
    // Initialise CYW43 in poll mode (no RTOS, no threadsafe_background)
    if (cyw43_arch_init() != 0) {
        printf("[wifi] ERROR: cyw43_arch_init() failed\n");
        return false;
    }

    // Enable AP mode: SSID, password, channel, security
    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
    printf("[wifi] AP started — SSID: %s\n", WIFI_SSID);

    // Configure AP IP address and netmask for the lwIP network interface
    ip4_addr_t ip, netmask, gw;
    ip4addr_aton(WIFI_AP_IP, &ip);
    ip4addr_aton("255.255.255.0", &netmask);
    ip4addr_aton(WIFI_AP_IP, &gw);    // gateway = AP itself

    // Apply to the CYW43 AP netif
    struct netif *ap_netif = &cyw43_state.netif[CYW43_ITF_AP];
    netif_set_addr(ap_netif, &ip, &netmask, &gw);

    // Start DHCP server so the phone gets an address automatically
    dhcp_server_init(&s_dhcp_server, &ip, &netmask);
    printf("[wifi] DHCP server started — AP IP: %s\n", WIFI_AP_IP);

    return true;
}

void wifi_poll() {
    cyw43_arch_poll();
}
