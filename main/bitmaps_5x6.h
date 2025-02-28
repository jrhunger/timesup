// 5x6 bitmaps

const short int bitmap_blank_5x6[30] = {
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
};

const short int bitmap_0_5x6[30] = {
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,
};

const short int bitmap_1_5x6[30] = {
    0,0,1,0,0,
    0,1,1,0,0,
    1,0,1,0,0,
    0,0,1,0,0,
    0,0,1,0,0,
    1,1,1,1,1,
};

const short int bitmap_2_5x6[30] = {
    0,1,1,1,1,
    1,0,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,1,0,0,0,
    1,1,1,1,1,
};

const short int bitmap_3_5x6[30] = {
    0,1,1,1,0,
    1,0,0,0,1,
    0,0,1,1,0,
    0,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,
};

const short int bitmap_4_5x6[30] = {
    0,0,0,1,0,
    0,0,1,1,0,
    0,1,0,1,0,
    1,1,1,1,1,
    0,0,0,1,0,
    0,0,0,1,0,
};

const short int bitmap_5_5x6[30] = {
    1,1,1,1,1,
    1,0,0,0,0,
    1,1,1,1,0,
    0,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,
};

const short int bitmap_6_5x6[30] = {
    0,0,0,1,1,
    0,0,1,0,0,
    0,1,0,0,0,
    1,0,1,1,0,
    1,0,0,0,1,
    0,1,1,1,0,
};

const short int bitmap_7_5x6[30] = {
    1,1,1,1,1,
    0,0,0,0,1,
    0,0,0,1,0,
    0,0,1,0,0,
    0,1,0,0,0,
    1,0,0,0,0,
};

const short int bitmap_8_5x6[30] = {
    0,1,1,1,0,
    1,0,0,0,1,
    0,1,1,1,0,
    1,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,0,
};

const short int bitmap_9_5x6[30] = {
    0,1,1,1,0,
    1,0,0,0,1,
    0,1,1,1,1,
    0,0,0,0,1,
    0,0,0,1,0,
    1,1,1,0,0,
};

// to get a bitmap digit by number, like digits_5x6[0] for the 0 bitmap
const short int *digits_5x6[10] = {
    bitmap_0_5x6,
    bitmap_1_5x6,
    bitmap_2_5x6,
    bitmap_3_5x6,
    bitmap_4_5x6,
    bitmap_5_5x6,
    bitmap_6_5x6,
    bitmap_7_5x6,
    bitmap_8_5x6,
    bitmap_9_5x6,
};