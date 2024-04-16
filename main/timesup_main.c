/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"


#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      2

#define EXAMPLE_LED_NUMBERS         256
#define EXAMPLE_CHASE_SPEED_MS      10

static const char *TAG = "example";

static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
void hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}


// map x, y to led strip #
// Assumes serpentine starting top left going down/up/down/up...
// x/y start from 0,0 at bottom left
#define SIZE_X 16
#define SIZE_Y 16
uint32_t xy_to_strip(uint32_t x, uint32_t y) 
{
    // if it's an even row
    if ((x & 1) == 0) {
        return x * SIZE_Y + SIZE_Y - 1 - y;
    }
    else {
        return x * SIZE_Y + y;
    }
}


static short int spiral_to_strip[SIZE_X * SIZE_Y];
// setup spiral_to_strip map array. 
// anti-clockwise spiral from 0,0 to led strip #
void setup_spiral_to_strip()
{   
    int xmax = SIZE_X - 1;
    int ymax = SIZE_Y - 1;
    int xmin = 0;
    int ymin = 0;
    int x = 0;
    int y = 0;

    int i = 0;
    while (i < SIZE_X * SIZE_Y) {
        for (x = xmin; x <= xmax; x++) {
          spiral_to_strip[i] = xy_to_strip(x,y);
          if (i++ == SIZE_X * SIZE_Y) {
            return;
          }
        }
        ymin += 1;
        for (y = ymin; y <= ymax; y++) {
          spiral_to_strip[i] = xy_to_strip(x,y);
          if (i++ == SIZE_X * SIZE_Y) {
            return;
          }
        }
        xmax -= 1;
        for (x = xmax; x >= xmin; x--) {
          spiral_to_strip[i] = xy_to_strip(x,y);
          if (i++ == SIZE_X * SIZE_Y) {
            return;
          }
        }
        ymax -= 1;
        for (y = ymax; y >= ymin; y--) {
          spiral_to_strip[i] = xy_to_strip(x,y);
          if (i++ == SIZE_X * SIZE_Y) {
            return;
          }
        }
        xmin += 1;
    }
}

void app_main(void)
{
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;

    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_channel_handle_t led_chan = NULL;
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_LOGI(TAG, "Install led strip encoder");
    rmt_encoder_handle_t led_encoder = NULL;
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));

    ESP_LOGI(TAG, "Start LED rainbow chase");
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };

    setup_spiral_to_strip();
    
    while (1) {
        for (int j = 0; j < EXAMPLE_LED_NUMBERS; j += 1) {
            // Build RGB pixels
            hue = (hue + 2) % 360;
            hsv2rgb(hue, 100, 30, &red, &green, &blue);
            led_strip_pixels[j * 3 + 0] = green;
            led_strip_pixels[j * 3 + 1] = blue;
            led_strip_pixels[j * 3 + 2] = red;
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        }
    }
}
