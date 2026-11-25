#include <REG51.H>

sbit DATA_595 = P3^1;
sbit SHIFT_595 = P3^0;
sbit LATCH_595 = P3^2;

static unsigned char code heart[8] = {
    0x00, 0x66, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0x00
};
static unsigned char code row_select[8] = {
    0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F
};

static void write_595(unsigned char value)
{
    unsigned char bit_index;

    LATCH_595 = 0;
    for (bit_index = 0; bit_index < 8; ++bit_index) {
        SHIFT_595 = 0;
        DATA_595 = (value & 0x80) != 0;
        SHIFT_595 = 1;
        value <<= 1;
    }
    SHIFT_595 = 0;
    LATCH_595 = 1;
    LATCH_595 = 0;
}

static void hold_row(void)
{
    TH0 = 0xF7;
    TL0 = 0x68;
    TF0 = 0;
    TR0 = 1;
    while (!TF0) {
    }
    TR0 = 0;
}

void main(void)
{
    unsigned char row;

    P2 = 0xFF;
    SHIFT_595 = 0;
    LATCH_595 = 0;
    TMOD = 0x01;
    for (;;) {
        for (row = 0; row < 8; ++row) {
            P2 = 0xFF;
            write_595(heart[row]);
            P2 = row_select[row];
            hold_row();
        }
    }
}
