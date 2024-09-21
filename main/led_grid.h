#include "freertos/FreeRTOS.h"
#include "led_strip_encoder.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"

// LED output constants
#define STRIP_LENGTH        256
#define FRAME_DELAY_MS      10
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      2

// GRID dimensions
#define SIZE_X 16
#define SIZE_Y 16

void draw_bitmap_size_offset_xy_rgb(const short int *bitmap, 
  short int size_x, short int size_y,
  short int offset_x, short int offset_y, 
  short int r, short int g, short int b);
  
void set_index_rgb(uint32_t index, uint32_t red, uint32_t green, uint32_t blue);
void grid_update_pixels();
void draw_spiral(uint16_t index);
void draw_bitmap_rgb(const short int *bitmap, short int angle, short int r, short int g, short int b);
void draw_bitmap(const short int *bitmap, short int angle);
void init_grid();