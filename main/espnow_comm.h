#include "esp_wifi.h"

#define MY_ESPNOW_WIFI_MODE WIFI_MODE_STA
#define MY_ESPNOW_WIFI_IF ESP_IF_WIFI_STA

#define MY_ESPNOW_PMK "pmk1234567890123"
#define MY_ESPNOW_CHANNEL 1

typedef void(*espnow_comm_recv_cb_t)(const int data);

// external functions
void init_recv_comms(espnow_comm_recv_cb_t cb);

