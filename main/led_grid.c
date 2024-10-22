#include "led_grid.h"

// logging tag
static const char *TAG = "led_grid";

// pixel vector
static uint8_t led_strip_pixels[STRIP_LENGTH * 3];

// rmt variables
static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;
static rmt_transmit_config_t tx_config = {
    .loop_count = 0, // no transfer loop
};



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
uint32_t xy_to_strip(uint32_t x, uint32_t y) 
{
    // flip top to bottom
    y = SIZE_Y - 1 - y;
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

// draw a bitmap of given size, offset by given amount. 
void draw_bitmap_size_offset_xy_rgb(const short int *bitmap, 
  short int size_x, short int size_y,
  short int offset_x, short int offset_y, 
  short int r, short int g, short int b) {
  for (int j = 0; j < size_y; j++) {
    for (int i = 0; i < size_x; i++) {
      if (bitmap[j*size_x + i] == 1) {
        set_xy_rgb(i+offset_x, j + offset_y, r, g, b);
      }
    }
  } 
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

void draw_spiral(uint16_t start, uint16_t end) {
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    if (end >= STRIP_LENGTH) {
        ESP_LOGI(TAG, "spiral end %d set to %d", end, STRIP_LENGTH - 1);
        end = STRIP_LENGTH -1;
    }
    if (start > end) {
        ESP_LOGI(TAG, "spiral start %d set to end %d", end, STRIP_LENGTH - 1);
        start = end;
    }
    // clear up to start of spiral
    for (int i = 0; i < start; i++) {
        set_index_rgb(spiral_to_strip[i], 0, 0, 0);
    }
    // draw spiral from start to end
    for (int i = start; i<=end; i++) {
        // Build RGB pixels
        //hue = (hue + 2) % 360;
        hue = (i*360*2/STRIP_LENGTH) % 360;
        hsv2rgb(359 - hue, 100, 1, &red, &green, &blue);
        set_index_rgb(spiral_to_strip[i], red, green, blue);
    }
}

void grid_update_pixels() {
    // Flush RGB values to LEDs
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
}

void grid_clear() {
    for (int j = 0; j< STRIP_LENGTH; j++) {
        set_index_rgb(j,0,0,0);
    }
}

void grid_init() {
    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_LOGI(TAG, "Install led strip encoder");
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));

    ESP_LOGI(TAG, "Compute spiral to strip mapping");
    setup_spiral_to_strip();

    // start with a clear display
    for (int j = 0; j< STRIP_LENGTH; j++) {
        set_index_rgb(j,0,0,0);
    }
    grid_update_pixels();
}