#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "esp_mac.h"

#include "espnow_comm.h"

static const char *TAG = "TimesUp";

static QueueHandle_t s_recv_queue;

typedef struct {
    uint8_t sender_mac_addr[ESP_NOW_ETH_ALEN];
    int data;
} recv_packet_t;


static void queue_process_task(void *p)
{
    static recv_packet_t recv_packet;

    ESP_LOGI(TAG, "Listening");
    for(;;)
    {
        if(xQueueReceive(s_recv_queue, &recv_packet, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }

        // Refer to user function
        //my_data_receive(recv_packet.sender_mac_addr, &recv_packet.data);
        ESP_LOGI(TAG, "received something");
    }
}

static void recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    static recv_packet_t recv_packet;
    uint8_t * mac_addr = recv_info->src_addr;

    ESP_LOGI(TAG, "%d bytes incoming from " MACSTR, len, MAC2STR(mac_addr));
    
    if(len != sizeof(int))
    {
        ESP_LOGE(TAG, "Unexpected data length: %d != %u", len, sizeof(int));
        return;
    }

    memcpy(&recv_packet.sender_mac_addr, mac_addr, sizeof(recv_packet.sender_mac_addr));
    memcpy(&recv_packet.data, data, len);
    if (xQueueSend(s_recv_queue, &recv_packet, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Queue full, discarded");
        return;
    }
}

static void init_espnow_master(void)
{
    const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    ESP_ERROR_CHECK( esp_netif_init());
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(MY_ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start() );
#if MY_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(MY_ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(recv_cb) );
    ESP_ERROR_CHECK( esp_now_set_pmk((const uint8_t *)MY_ESPNOW_PMK) );
}

void recv_comms(void)
{
    s_recv_queue = xQueueCreate(10, sizeof(recv_packet_t));
    assert(s_recv_queue);
    BaseType_t err = xTaskCreate(queue_process_task, "recv_task", 8192, NULL, 4, NULL);
    assert(err == pdPASS);
    
    init_espnow_master();
}