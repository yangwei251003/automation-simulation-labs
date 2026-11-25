#include <REG52.H>
#include <intrins.h>
typedef unsigned char u8;
typedef unsigned int u16;

sbit LCD_RS=P2^7;
sbit LCD_RW=P2^6;
sbit LCD_E=P2^5;
sbit PWM_A=P3^4;
sbit PWM_B=P3^7;
sbit LOW_A=P3^0;
sbit LOW_B=P3^1;

static volatile u8 duty=50, tick=0, frames=0, ms_pending=0;
static volatile u16 pulses=0, rpm=0;
static volatile bit ready=1, running=0, forward=1;
static u8 current_keys=0, previous_keys=0, equal_samples=0;
static u16 key_repeat=0;

#define BRAKE() { PWM_A=1; PWM_B=1; _nop_(); _nop_(); LOW_A=1; LOW_B=1; }
static void apply_bridge(void) {
    if(running && tick<duty*2) {
        if(forward) {
            LOW_A=0; PWM_B=1;
            _nop_(); _nop_();
            LOW_B=1; PWM_A=0;
        } else {
            LOW_B=0; PWM_A=1;
            _nop_(); _nop_();
            LOW_A=1; PWM_B=0;
        }
    } else { BRAKE(); }
}

void encoder(void) interrupt 2 {
    pulses++;
}
void timer2(void) interrupt 5 using 1 {
    TF2=0;
    if(++tick==200) {
        tick=0;
        if(++frames==100) {
            frames=0;
            rpm=pulses*30;
            pulses=0;
            ready=1;
        }
    }
    if(tick%100==0 && ms_pending<250) ms_pending++;
    apply_bridge();
}

static void wait_us(u16 us) {
    u16 q=0-us;
    TR0=0; TH0=q>>8; TL0=q; TF0=0; TR0=1;
    while(!TF0) {}
    TR0=0;
}
static void lcd(bit value,u8 b) {
    LCD_RW=0; LCD_RS=value; P0=b;
    _nop_(); LCD_E=1; _nop_(); _nop_(); LCD_E=0;
    wait_us(55);
}
static void number(u16 v,u8 width) {
    char a[5];
    u8 i=width;
    do {a[--i]='0'+v%10; v/=10;} while(i);
    for(i=0;i<width;i++) lcd(1,a[i]);
}
static void text(char code *s) {
    while(*s) lcd(1,*s++);
}
static void lcd_init(void) {
    LCD_E=0; LCD_RW=0; wait_us(25000);
    lcd(0,0x38); wait_us(4500); lcd(0,0x38); lcd(0,0x0c); lcd(0,0x06);
    lcd(0,1); wait_us(2500);
}
static void refresh(void) {
    u16 value;
    bit f,r;
    EA=0; value=rpm; ready=0; f=forward; r=running; EA=1;
    lcd(0,0x80); text("V:"); number(value,5); text(" rpm ");
    if(!r) text("STOP");
    else if(f) text("FWD ");
    else text("REV ");
    lcd(0,0xc0); text("zhankongbi:"); number(duty,3); text("% ");
}
static void scan(void) {
    u8 keys=(~P1)&31, pressed;
    if(keys!=previous_keys) {
        equal_samples=0; previous_keys=keys; return;
    }
    if(equal_samples<2) equal_samples++;
    if(equal_samples==2 && keys!=current_keys) {
        pressed=keys&~current_keys; current_keys=keys;
        if(pressed&1 && duty<100) duty++;
        if(pressed&2 && duty>0) duty--;
        if(pressed&4) forward=1;
        if(pressed&8) forward=0;
        if(pressed&16) running=!running;
        key_repeat=35;
        ready=1;
    } else if(keys&3) {
        if(key_repeat) key_repeat--;
        else {
            if((keys&3)==1 && duty<100) duty++;
            if((keys&3)==2 && duty>0) duty--;
            key_repeat=5; ready=1;
        }
    }
}
void main(void) {
    P0=P1=P2=P3=0xff;
    TMOD=0x01;
    BRAKE();
    lcd_init();
    T2CON=0;
    RCAP2H=TH2=0xff; RCAP2L=TL2=0x9c;
    IT1=1; EX1=1; ET2=1; EA=1; TR2=1;
    while(1) {
        if(ms_pending) {
            EA=0; ms_pending--; EA=1;
            scan();
        }
        if(ready) refresh();
    }
}


