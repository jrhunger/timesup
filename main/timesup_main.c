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
// for input
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "esp_timer.h"
// to pick random direction
#include "esp_random.h"
// bitmaps!!!
#include "bitmaps12x12.h"
#include "bitmaps5x6.h"

// LED output constants
#define STRIP_LENGTH        256
#define FRAME_DELAY_MS      10
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      2

// game input GPIOs
#define GPIO_UP 3
#define GPIO_DOWN 0
#define GPIO_LEFT 10
#define GPIO_RIGHT 1
static const char *TAG = "timesup";

static uint8_t led_strip_pixels[STRIP_LENGTH * 3];

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
//          printf("%d = (%d, %d)\n", i, x, y);
          if (i++ == SIZE_X * SIZE_Y) {
            return;
          }
        }
        x--;
        ymin += 1;
        for (y = ymin; y <= ymax; y++) {
          spiral_to_strip[i] = xy_to_strip(x,y);
//          printf("%d = (%d, %d)\n", i, x, y);
          if (i++ == SIZE_X * SIZE_Y) {
            return;
          }
        }
        y--;
        xmax -= 1;
        for (x = xmax; x >= xmin; x--) {
          spiral_to_strip[i] = xy_to_strip(x,y);
//          printf("%d = (%d, %d)\n", i, x, y);
          if (i++ == SIZE_X * SIZE_Y) {
            return;
          }
        }
        x++;
        ymax -= 1;
        for (y = ymax; y >= ymin; y--) {
          spiral_to_strip[i] = xy_to_strip(x,y);
//          printf("%d = (%d, %d)\n", i, x, y);
          if (i++ == SIZE_X * SIZE_Y) {
            return;
          }
        }
        y++;
        xmin += 1;
    }
}

void set_index_rgb(uint32_t index, uint32_t red, uint32_t green, uint32_t blue) {
    led_strip_pixels[index * 3 + 0] = green;
    led_strip_pixels[index * 3 + 1] = red;
    led_strip_pixels[index * 3 + 2] = blue;
}

void set_xy_rgb(uint32_t x, uint32_t y, uint32_t red, uint32_t green, uint32_t blue) {
    set_index_rgb(xy_to_strip(x,y), red, green, blue);
}

#define OFFSET_X (SIZE_X - 12) / 2
#define OFFSET_Y (SIZE_Y - 12) / 2
void draw_bitmap_rgb(const short int *bitmap, short int angle, short int r, short int g, short int b)
{
    if (angle == 90) {
        //ESP_LOGI(TAG, "draw bitmap 90");
        for (int j = 0; j < 12; j++) {
            for (int i = 0; i < 12; i++) {
                // 90: (x,y) -> (y, -x)
                if (bitmap[(11-i) * 12 + j] == 1) {
                    set_xy_rgb(i + OFFSET_X, j + OFFSET_Y, r, g, b);
                }
            }
        }
    }
    else if (angle == 180) { 
        //ESP_LOGI(TAG, "draw bitmap 180");
        for (int j = 0; j < 12; j++) {
            for (int i = 0; i < 12; i++) {
                // 180: (x,y) -> (-x, -y)
                if (bitmap[(11-j) * 12 + (11 - i)] == 1) {
                    set_xy_rgb(i + OFFSET_X, j + OFFSET_Y, r, g, b);
                }
            }
        }
    }
    else if (angle == -180) { 
        //ESP_LOGI(TAG, "draw bitmap flipped horizontal");
        for (int j = 0; j < 12; j++) {
            for (int i = 0; i < 12; i++) {
                // 180: (x,y) -> (-x, -y)
                if (bitmap[j * 12 + (11 - i)] == 1) {
                    set_xy_rgb(i + OFFSET_X, j + OFFSET_Y, r, g, b);
                }
            }
        }
    }
    else if (angle == 270) {
        //ESP_LOGI(TAG, "draw bitmap 270");
        for (int j = 0; j < 12; j++) {
            for (int i = 0; i < 12; i++) {
                // 270: (x,y) -> (-y, x)
                if (bitmap[i * 12 + 11 - j] == 1) {
                    set_xy_rgb(i + OFFSET_X, j + OFFSET_Y, r, g, b);
                }
            }
        }
    }
    else {
        //ESP_LOGI(TAG, "draw bitmap 0");
        for (int j = 0; j < 12; j++) {
            for (int i = 0; i < 12; i++) {
                if (bitmap[j * 12 + i] == 1) {
                    set_xy_rgb(i + OFFSET_X, j + OFFSET_Y, r, g, b);
                }
            }
        }
    }
}

void draw_bitmap(const short int *bitmap, short int angle) {
    draw_bitmap_rgb(bitmap, angle, 1,1,1);
}

void draw_spiral(uint16_t index) {
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    if (index >= STRIP_LENGTH) {
        ESP_LOGI(TAG, "spiral index %d set to %d", index, STRIP_LENGTH - 1);
        index = STRIP_LENGTH -1;
    }
    for (int i = 0; i<index; i++) {
        // Build RGB pixels
        hue = (hue + 2) % 360;
        hsv2rgb(359 - hue, 100, 1, &red, &green, &blue);
        set_index_rgb(spiral_to_strip[i], red, green, blue);
    }
}

// Queue for inputs
static QueueHandle_t gpio_evt_queue = NULL;

// ISR handler needs to be short and sweet
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static uint16_t input_enabled = 0;
static uint16_t last_input = 99;
// process events from queue
static void gpio_task(void* arg) {
    uint32_t gpio_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY)) {
            if(input_enabled == 1) {
                ESP_LOGI(TAG, "received %d", (int) gpio_num);
                last_input = gpio_num;
                input_enabled = 0;
            }
        }
    }
}


void app_main(void)
{

    ESP_LOGI(TAG, "enable GPIO inputs");
    // 3 = up
    gpio_pullup_en(GPIO_UP);
    gpio_set_direction(GPIO_UP, GPIO_MODE_INPUT);
    gpio_set_intr_type(GPIO_UP, GPIO_INTR_NEGEDGE);
    gpio_intr_enable(GPIO_UP);
    // 0 = down
    gpio_pullup_en(GPIO_DOWN);
    gpio_set_direction(GPIO_DOWN, GPIO_MODE_INPUT);
    gpio_set_intr_type(GPIO_DOWN, GPIO_INTR_NEGEDGE);
    gpio_intr_enable(GPIO_DOWN);
    // 10 = left
    gpio_pullup_en(GPIO_LEFT);
    gpio_set_direction(GPIO_LEFT, GPIO_MODE_INPUT);
    gpio_set_intr_type(GPIO_LEFT, GPIO_INTR_NEGEDGE);
    gpio_intr_enable(GPIO_LEFT);
    // 1 = right
    gpio_pullup_en(GPIO_RIGHT);
    gpio_set_direction(GPIO_RIGHT, GPIO_MODE_INPUT);
    gpio_set_intr_type(GPIO_RIGHT, GPIO_INTR_NEGEDGE);
    gpio_intr_enable(GPIO_RIGHT);

    ESP_LOGI(TAG, "add GPIO isr service");
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(0); // TODO - find out what the 0 means
    //hook isr handler for specific gpio pins
    gpio_isr_handler_add(GPIO_UP, gpio_isr_handler, (void*) GPIO_UP);
    gpio_isr_handler_add(GPIO_DOWN, gpio_isr_handler, (void*) GPIO_DOWN);
    gpio_isr_handler_add(GPIO_LEFT, gpio_isr_handler, (void*) GPIO_LEFT);
    gpio_isr_handler_add(GPIO_RIGHT, gpio_isr_handler, (void*) GPIO_RIGHT);

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

    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };

    ESP_LOGI(TAG, "Compute spiral to strip mapping");
    setup_spiral_to_strip();

    // print out the left bitmap (remove later)
    for (int j = 0; j < 12; j++) {
        for (int i = 0; i < 12; i++) {
            printf("%d,", bitmap_left12x12[j*(SIZE_X - OFFSET_X*2) + i]);
        }
    }

    // start with a clear display
    for (int j = 0; j< STRIP_LENGTH; j++) {
        set_index_rgb(j,0,0,0);
    }
    // Flush RGB values to LEDs
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

    uint32_t time_limit = 5000000; // 5 seconds worth of micros
    uint32_t elapsed_time = 0;
    int64_t enable_start = 0;
    int64_t delay_start = 0;
    uint16_t glyph_displayed = 0;
    uint16_t angle = 0;
    uint16_t game_on = 0;
    uint16_t score = 0;
    // enable input since using it to start game
    input_enabled = 1;
    ESP_LOGI(TAG, "Begin main loop");
    int64_t now = 0;
    while (1) {
        now = esp_timer_get_time();
        // counting time and total time is > limit
        if (game_on == 0) {
          if (last_input == 99) {
            vTaskDelay(pdMS_TO_TICKS(FRAME_DELAY_MS));
          }
          else {
            game_on = 1;
            last_input = 99;
            input_enabled = 0;
            delay_start = now;
          }
        }
        else if (enable_start > 0 && now - enable_start + elapsed_time >= time_limit) {
            ESP_LOGI(TAG, "TIME's UP!! %lld %lld, score %d", enable_start, now, score);
            for (int j = 0; j< STRIP_LENGTH; j++) {
                set_index_rgb(j,0,0,0);
            }
            draw_bitmap(bitmap_bang12x12, 0);
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
            input_enabled = 0;
            elapsed_time = 0;
            enable_start = 0;
            vTaskDelay(pdMS_TO_TICKS(3000)); // 5 second delay
            glyph_displayed = 0;
            game_on = 0;
            score = 0;
            last_input = 99;
            input_enabled = 1;
        }
        else if (glyph_displayed == 1) {
            if (last_input != 99) { // there is some input
                // clear the bitmap part
                for (int j = 0; j< STRIP_LENGTH; j++) {
                    set_index_rgb(j,0,0,0);
                }
                glyph_displayed = 0;
                if ((angle == 0 && last_input == GPIO_LEFT) ||
                    (angle == 90 && last_input == GPIO_DOWN) ||
                    (angle == 180 && last_input == GPIO_RIGHT) ||
                    (angle == 270 && last_input == GPIO_UP)) 
                {
                    ESP_LOGI(TAG, "CORRECT INPUT");
                    score++;
                    elapsed_time += now - enable_start;
                    enable_start = 0;
                    draw_spiral((uint16_t) ((elapsed_time) * STRIP_LENGTH / time_limit));
                    draw_bitmap_rgb(bitmap_check12x12,0,0,2,0);
                }
                else {
                    ESP_LOGI(TAG, "WRONG INPUT");
                    draw_spiral((uint16_t) ((now - enable_start + elapsed_time) * STRIP_LENGTH / time_limit));
                    draw_bitmap_rgb(bitmap_x12x12,0,2,0,0);
                }
                delay_start = now;
            }
            else {
                draw_bitmap(bitmap_left12x12, angle);
                draw_spiral((uint16_t) ((now - enable_start + elapsed_time) * STRIP_LENGTH / time_limit));
            }
        }
        else { // glyph not displayed (and time not up)
            if (delay_start == 0 || now - delay_start > 1000000) {
                // set up for next one
                //angle = (angle + 90) % 360;
                angle = (esp_random() & 3) * 90;
                ESP_LOGI(TAG, "new angle = %d", angle);
                last_input = 99;  // clear last input
                for (int j = 0; j< STRIP_LENGTH; j++) {
                   set_index_rgb(j,0,0,0);
                }
                if (enable_start == 0) { // start counting time if not already
                    enable_start = now;
                    ESP_LOGI(TAG, "start enabled %lld", now);
                }
                else { // time is enabled so update the spiral
                    draw_spiral((uint16_t) ((now - enable_start + elapsed_time) * STRIP_LENGTH / time_limit));
                }
                glyph_displayed = 1;
                delay_start = 0;
                input_enabled = 1;
            }
            else {
                if (enable_start == 0) {
                    draw_spiral((uint16_t)((elapsed_time) * STRIP_LENGTH / time_limit));
                } 
                else {
                    draw_spiral((uint16_t)((now - enable_start + elapsed_time) * STRIP_LENGTH / time_limit));
                }
            }
        }
        // Flush RGB values to LEDs
        ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
        vTaskDelay(pdMS_TO_TICKS(FRAME_DELAY_MS));
    }
}
