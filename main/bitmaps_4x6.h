// 4x6 bitmaps

const short int bitmap_blank_4x6[24] = {
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
};

const short int bitmap_0_4x6[24] = {
    0,1,1,0,
    1,0,0,1,
    1,0,0,1,
    1,0,0,1,
    1,0,0,1,
    0,1,1,0,
};

const short int bitmap_1_4x6[24] = {
    0,0,1,0,
    0,1,1,0,
    1,0,1,0,
    0,0,1,0,
    0,0,1,0,
    1,1,1,1,
};

const short int bitmap_2_4x6[24] = {
    0,1,1,0,
    1,0,0,1,
    0,0,0,1,
    0,0,1,0,
    0,1,0,0,
    1,1,1,1,
};

const short int bitmap_3_4x6[24] = {
    1,1,1,0,
    0,0,0,1,
    0,1,1,0,
    0,0,0,1,
    0,0,0,1,
    1,1,1,0,
};

const short int bitmap_4_4x6[24] = {
    0,0,0,1,
    0,0,1,1,
    0,1,0,1,
    1,1,1,1,
    0,0,0,1,
    0,0,0,1,
};

const short int bitmap_5_4x6[24] = {
    1,1,1,1,
    1,0,0,0,
    1,1,1,0,
    0,0,0,1,
    1,0,0,1,
    0,1,1,0,
};

const short int bitmap_6_4x6[24] = {
    0,0,1,1,
    0,1,0,0,
    1,0,0,0,
    1,1,1,0,
    1,0,0,1,
    0,1,1,0,
};

const short int bitmap_7_4x6[24] = {
    1,1,1,1,
    0,0,0,1,
    0,0,1,0,
    0,1,0,0,
    1,0,0,0,
    1,0,0,0,
};

const short int bitmap_8_4x6[24] = {
    0,1,1,0,
    1,0,0,1,
    0,1,1,0,
    1,0,0,1,
    1,0,0,1,
    0,1,1,0,
};

const short int bitmap_9_4x6[24] = {
    0,1,1,0,
    1,0,0,1,
    0,1,1,1,
    0,0,0,1,
    0,0,1,0,
    1,1,0,0,
};

// to get a bitmap digit by number, like digits_4x6[0] for the 0 bitmap
const short int *digits_4x6[10] = {
    bitmap_0_4x6,
    bitmap_1_4x6,
    bitmap_2_4x6,
    bitmap_3_4x6,
    bitmap_4_4x6,
    bitmap_5_4x6,
    bitmap_6_4x6,
    bitmap_7_4x6,
    bitmap_8_4x6,
    bitmap_9_4x6,
};