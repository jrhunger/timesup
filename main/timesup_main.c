/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


// for input
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "esp_timer.h"

// to pick random direction
#include "esp_random.h"

// bitmaps!!!
#include "bitmaps_12x12.h"
#include "bitmaps_5x6.h"
#include "bitmaps_4x6.h"

// led grid
#include "led_grid.h"

// esp-now communications
#include "espnow_comm.h"

// control input values, must match CtrlNow
#define INPUT_UP 0
#define INPUT_RIGHT 1
#define INPUT_DOWN 2
#define INPUT_LEFT 3
// this is used internally, not sent by CtrlNow
#define INPUT_NONE 99

// logging tag
#define TAG "timesup"

void draw_score(short int s) {
  if (s > 99) {
    s = 99;
  }
  if (s < 0) {
    s = 0;
  }
  draw_bitmap_size_offset_xy_rgb(digits_5x6[s/10], 5, 6, 2, 1, 2, 2, 2);
  draw_bitmap_size_offset_xy_rgb(digits_5x6[s%10], 5, 6, 8, 1, 2, 2, 2);
}

void draw_time(short int t) {
  if (t > 999) {
    t = 999;
  }
  if (t < 0) {
    t = 0;
  }
  draw_bitmap_size_offset_xy_rgb(digits_4x6[t/100],    4, 6, 1, 8, 2, 0, 0);
  draw_bitmap_size_offset_xy_rgb(digits_4x6[t%100/10], 4, 6, 6, 8, 0, 2, 0);
  draw_bitmap_size_offset_xy_rgb(digits_4x6[t%10],     4, 6, 11, 8, 0, 0, 2);
}

static uint16_t input_enabled = 0;
static uint16_t last_input = INPUT_NONE;
static uint64_t last_input_received = 0;
// input handler, will be passed to espnow_comm_init and called
// with any incoming espnow_comm input
void handle_input(int gpio_num) {
    if(input_enabled == 1) {
        input_enabled = 0;
        last_input_received = esp_timer_get_time();
        ESP_LOGI(TAG, "received %d at %lld", (int) gpio_num, last_input_received);
        last_input = gpio_num;
    }
}

void app_main(void)
{
    init_recv_comms(handle_input);
    grid_init();

    uint32_t time_limit = 8000000; // 8 seconds worth of micros
    uint32_t elapsed_time = 0;
    int64_t enable_start = 0;
    int64_t delay_start = 0;
    uint16_t glyph_displayed = 0;
    uint16_t angle = 0;
    uint16_t game_on = 0;
    uint16_t score = 0;
    int64_t min_reaction = 999;
    // enable input since using it to start game
    input_enabled = 1;
    ESP_LOGI(TAG, "Begin main loop");
    int64_t now = 0;
    draw_score(0);
    draw_time(999);
    grid_update_pixels();
    vTaskDelay(pdMS_TO_TICKS(3000)); // 3 second delay before erasing initial scores
    while (1) {
        now = esp_timer_get_time();
        // idle state between games
        if (game_on == 0) { 
          // no input, not time to start the game
          if (last_input == INPUT_NONE) { 
            while (last_input == INPUT_NONE) {
//                for (int j = 0; j < STRIP_LENGTH - 1; j++) {
//                    if (last_input != INPUT_NONE) { break; }
//                    draw_spiral(0, j);
//                    grid_update_pixels();
//                    vTaskDelay(pdMS_TO_TICKS(FRAME_DELAY_MS));
//                }
//                for (int j = 0; j < STRIP_LENGTH - 1; j++) {
//                    if (last_input != INPUT_NONE) { break; }
//                    draw_spiral(j, STRIP_LENGTH - 1);
//                    grid_update_pixels();
//                    vTaskDelay(pdMS_TO_TICKS(FRAME_DELAY_MS));
//                }
                for (int j = 0; j < STRIP_LENGTH - 1; j++) {
                    if (last_input != INPUT_NONE) { break; }
                    draw_spiral(j, j+4);
                    grid_update_pixels();
                    vTaskDelay(pdMS_TO_TICKS(FRAME_DELAY_MS));
                }
                for (int j = STRIP_LENGTH - 1; j > 0; j--) {
                    if (last_input != INPUT_NONE) { break; }
                    grid_clear();
                    draw_spiral(j, j+4);
                    grid_update_pixels();
                    vTaskDelay(pdMS_TO_TICKS(FRAME_DELAY_MS));
                }
                grid_clear();
            }
          }
          // input received, ignore value but start the game
          else { 
            game_on = 1;
            last_input = INPUT_NONE;
            input_enabled = 0;
            grid_clear();
            grid_update_pixels();
            delay_start = now;
          }
        }
        // counting time and total time is > limit: end game
        else if (enable_start > 0 && now - enable_start + elapsed_time >= time_limit) {
            ESP_LOGI(TAG, "TIME's UP!! %lld %lld, score %d, best-time %lld", enable_start, now, score, min_reaction);
            grid_clear();
            draw_score(score);
            draw_time(min_reaction);
            grid_update_pixels();
            input_enabled = 0;
            elapsed_time = 0;
            enable_start = 0;
            vTaskDelay(pdMS_TO_TICKS(3000)); // 5 second delay to avoid starting new game prematurely
            glyph_displayed = 0;
            game_on = 0;
            score = 0;
            min_reaction = 999;
            last_input = INPUT_NONE;
            input_enabled = 1;
        }
        // game on and a glyph is displayed
        else if (glyph_displayed == 1) {
            // there is some input
            if (last_input != INPUT_NONE) { 
                grid_clear();
                glyph_displayed = 0;
                // check the input vs what was displayed
                if ((angle == 0 && last_input == INPUT_LEFT) ||
                    (angle == 90 && last_input == INPUT_UP) ||
                    (angle == 180 && last_input == INPUT_RIGHT) ||
                    (angle == 270 && last_input == INPUT_DOWN)) 
                {
                    ESP_LOGI(TAG, "CORRECT INPUT");
                    score++;
                    elapsed_time += now - enable_start;
                    if (min_reaction > (last_input_received - enable_start)/1000) {
                        min_reaction = (last_input_received - enable_start)/1000;
                    }
                    ESP_LOGI(TAG, "reaction = %lld", last_input_received - enable_start);
                    enable_start = 0;
                    draw_spiral(0, (uint16_t) ((elapsed_time) * STRIP_LENGTH / time_limit));
                    draw_bitmap_rgb(bitmap_check12x12,0,0,2,0);
                }
                else {
                    ESP_LOGI(TAG, "WRONG INPUT");
                    draw_spiral(0, (uint16_t) ((now - enable_start + elapsed_time) * STRIP_LENGTH / time_limit));
                    draw_bitmap_rgb(bitmap_x12x12,0,2,0,0);
                }
                delay_start = now;
            }
            // no input, redraw the glyph and update the spiral
            else {
                draw_bitmap(bitmap_left12x12, angle);
                draw_spiral(0, (uint16_t) ((now - enable_start + elapsed_time) * STRIP_LENGTH / time_limit));
            }
        }
        // glyph not displayed (and time not up)
        else { 
            // time to display glyph
            if (delay_start == 0 || now - delay_start > 1000000) {
                // set up for next one
                //angle = (angle + 90) % 360;
                angle = (esp_random() & 3) * 90;
                ESP_LOGI(TAG, "new angle = %d", angle);
                last_input = INPUT_NONE;  // clear last input
                grid_clear();
                if (enable_start == 0) { // start counting time if not already
                    enable_start = now;
                    ESP_LOGI(TAG, "start enabled %lld", now);
                }
                else { // time is enabled so update the spiral
                    draw_spiral(0, (uint16_t) ((now - enable_start + elapsed_time) * STRIP_LENGTH / time_limit));
                }
                glyph_displayed = 1;
                delay_start = 0;
                input_enabled = 1;
            }
            // not time to display glyph. draw spiral
            else {
                // not including current delay
                if (enable_start == 0) {
                    draw_spiral(0, (uint16_t)((elapsed_time) * STRIP_LENGTH / time_limit));
                } 
                // including current delay (previous input wrong)
                else {
                    draw_spiral(0, (uint16_t)((now - enable_start + elapsed_time) * STRIP_LENGTH / time_limit));
                }
            }
        }

        // Flush RGB values to LEDs
        grid_update_pixels();
        vTaskDelay(pdMS_TO_TICKS(FRAME_DELAY_MS));
    }
}