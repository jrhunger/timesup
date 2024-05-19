# timesup
Eventually a twitch game on a smallish LED matrix.

## Notes:
* Base LED driver stolen from [esp-idf rmt/led_strip example](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/rmt/led_strip)
* Drawing a bitmap:
    * store as pixel on-off and apply colors later
    * redraw with color 0 to clear it
    * start with 12x12 in center of 16x16
    * store as array should be easier to draw
