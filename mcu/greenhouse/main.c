#include <REG51.H>
#include <intrins.h>

typedef unsigned char u8;
typedef unsigned int u16;
sbit LCD_E=P2^0;
sbit LCD_RW=P2^1;
sbit LCD_RS=P2^2;
sbit BEEP=P2^3;
sbit HEATER=P2^4;
sbit RTC_CE=P2^5;
sbit RTC_CLK=P2^6;
sbit RTC_IO=P2^7;
sbit PUMP=P3^0;
sbit LAMP=P3^1;
sbit FAN=P3^2;
sbit DQ=P3^3;
sbit ADC_CS=P3^4;
sbit ADC_CLK=P3^5;
sbit ADC_DI=P3^6;
sbit ADC_DO=P3^7;
sbit HOT_LED=P0^0;
sbit COLD_LED=P0^4;

static u8 limits[4]={15,35,30,50};
static u8 menu=0, lock=0, manual=0, stable_keys=0, last_keys=0;
static u8 humidity, light, reading_valid, chirp;
static int degrees;
static u8 clock_value[3];
static u8 code headings[4][17]={"Temperature min ","Temperature max ","Humidity minimum","Light minimum   "};

static void delay_us(u16 us) {
    u16 q=0-us;
    TR0=0; TH0=q>>8; TL0=q; TF0=0; TR0=1;
    while(!TF0) {}
    TR0=0;
}

static void lcd_byte(bit writing, u8 value) {
    LCD_RS=writing; LCD_RW=0; P1=value;
    _nop_(); LCD_E=1; _nop_(); _nop_(); LCD_E=0;
    delay_us(55);
}

static void lcd_line(u8 addr, char *buf) {
    u8 i;
    lcd_byte(0,addr);
    for(i=0;i<16;i++) lcd_byte(1,buf[i]);
}

static void lcd_init(void) {
    LCD_E=0; LCD_RW=0;
    delay_us(20000);
    lcd_byte(0,0x38); delay_us(4500);
    lcd_byte(0,0x38); lcd_byte(0,0x0c); lcd_byte(0,0x06);
    lcd_byte(0,0x01); delay_us(2200);
}

static bit ow_reset(void) {
    bit found;
    DQ=0; delay_us(505);
    DQ=1; delay_us(60);
    found=!DQ;
    delay_us(430);
    return found;
}

static void ow_write(u8 v) {
    u8 i;
    for(i=0;i<8;i++) {
        if(v&1) {
            DQ=0; _nop_(); _nop_(); DQ=1; delay_us(65);
        } else {
            DQ=0; delay_us(52); DQ=1; delay_us(8);
        }
        v>>=1;
    }
}

static u8 ow_read(void) {
    u8 i,v=0;
    for(i=0;i<8;i++) {
        v>>=1;
        DQ=0; _nop_(); _nop_(); DQ=1;
        _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_();
        if(DQ) v|=0x80;
        delay_us(58);
    }
    return v;
}

static void start_temperature(void) {
    if(ow_reset()) {
        ow_write(0xcc); ow_write(0x44);
    }
}

static void sample_temperature(void) {
    u8 lo,hi;
    if(ow_reset()) {
        ow_write(0xcc); ow_write(0xbe);
        lo=ow_read(); hi=ow_read();
        degrees=(int)(((u16)hi<<8)|lo);
        reading_valid=(degrees>=-880 && degrees<=2000);
    } else reading_valid=0;
    start_temperature();
}

static void adc_clock(void) {
    _nop_(); ADC_CLK=1; _nop_(); _nop_(); ADC_CLK=0; _nop_();
}

static u8 adc_read(u8 channel) {
    u8 i,v=0;
    ADC_CLK=0; ADC_CS=0;
    ADC_DI=1; adc_clock(); adc_clock();
    ADC_DI=channel; adc_clock();
    for(i=0;i<8;i++) {
        adc_clock();
        v=(v<<1)|ADC_DO;
    }
    ADC_CS=1;
    return (u16)v*100/255;
}

static void rtc_send(u8 cmd) {
    u8 i;
    for(i=0;i<8;i++) {
        RTC_IO=cmd&1;
        RTC_CLK=1; _nop_();
        /* The RTC shifts its first output on the falling edge of bit 7. */
        if(i==7 && RTC_IO) RTC_IO=1;
        RTC_CLK=0; _nop_();
        cmd>>=1;
    }
}

static void rtc_write(u8 address,u8 value) {
    RTC_CLK=0; RTC_CE=1; _nop_();
    rtc_send(address); rtc_send(value);
    RTC_CE=0; RTC_IO=1;
}

static void rtc_read(void) {
    u8 i,j,v;
    RTC_CLK=0; RTC_CE=1; _nop_();
    rtc_send(0xbf); RTC_IO=1;
    for(i=0;i<3;i++) {
        v=0;
        for(j=0;j<8;j++) {
            v=(v>>1)|(RTC_IO?0x80:0);
            RTC_CLK=1; _nop_(); RTC_CLK=0; _nop_();
        }
        clock_value[i]=v;
    }
    RTC_CE=0;
}

static void rtc_init(void) {
    rtc_write(0x8e,0);
    rtc_write(0x80,0); rtc_write(0x82,0); rtc_write(0x84,0);
    rtc_write(0x86,1); rtc_write(0x88,1); rtc_write(0x8a,1);
    rtc_write(0x8c,0x26); rtc_write(0x8e,0x80);
}

static void digits(char *p, u16 value, u8 width) {
    while(width) {
        p[--width]='0'+value%10;
        value/=10;
    }
}

static void refresh(void) {
    char top[16], bottom[16];
    u8 i,v;
    u16 tenth;
    if(menu) {
        for(i=0;i<16;i++) {top[i]=headings[menu-1][i];bottom[i]=' ';}
        digits(bottom,limits[menu-1],3);
        bottom[3]=(menu<3)?0xdf:'%';
        bottom[4]=(menu<3)?'C':' ';
        bottom[8]='-'; bottom[10]='+'; bottom[13]='S';
    } else {
        for(i=0;i<3;i++) {
            v=clock_value[2-i] & (i==0?0x3f:0x7f);
            top[i*3]='0'+(v>>4); top[i*3+1]='0'+(v&15);
            top[i*3+2]=(i==2?' ':':');
        }
        if(reading_valid) {
            top[9]=degrees<0?'-':'+';
            tenth=(u16)(degrees<0?-degrees:degrees)*10/16;
            if(tenth>=1000) {
                digits(top+10,tenth/10,3);
                top[13]=' '; top[14]=0xdf; top[15]='C';
            } else {
                digits(top+10,tenth/10,2);
                top[12]='.'; top[13]='0'+tenth%10; top[14]=0xdf; top[15]='C';
            }
        } else {
            for(i=9;i<16;i++) top[i]=' ';
            top[10]='T'; top[11]=':'; top[12]='-'; top[13]='-';
        }
        bottom[0]='H'; bottom[1]='u'; bottom[2]='m'; bottom[3]=':';
        digits(bottom+4,humidity,3);
        bottom[7]='%'; bottom[8]=' ';
        bottom[9]='S'; bottom[10]='u'; bottom[11]='n'; bottom[12]=':';
        digits(bottom+13,light,3);
    }
    lcd_line(0x80,top);
    lcd_line(0xc0,bottom);
}

static void scan_keys(void) {
    u8 keys=(~P0)&0xee, pressed, pos;
    if(keys==last_keys && keys!=stable_keys) {
        pressed=keys&~stable_keys;
        stable_keys=keys;
        if(!pressed) return;
        chirp=9;
        if(pressed&8) {
            menu=(menu+1)%5;
        } else if(menu) {
            pos=menu-1;
            if((pressed&4) && limits[pos]<(menu<3?99:100)
                 && (menu!=1 || limits[0]<limits[1])) limits[pos]++;
            if((pressed&2) && limits[pos]
                 && (menu!=2 || limits[1]>limits[0])) limits[pos]--;
        } else {
            pressed&=0xe0;
            manual^=pressed;
            lock|=pressed;
        }
        refresh();
    }
    last_keys=keys;
}

static void control(void) {
    bit cold=reading_valid && degrees<(int)limits[0]*16;
    bit hot=reading_valid && degrees>(int)limits[1]*16;
    bit dry=humidity<limits[2];
    bit dark=light<limits[3];
    if(lock&0x80) cold=hot=0;
    HEATER=!cold;
    FAN=(lock&0x80)?!(manual&0x80):!hot;
    PUMP=(lock&0x40)?!(manual&0x40):!dry;
    LAMP=(lock&0x20)?!(manual&0x20):!dark;
    COLD_LED=!cold; HOT_LED=!hot;
    BEEP=!(cold || hot || (!(lock&0x40)&&dry) || (!(lock&0x20)&&dark) || chirp);
}
void main(void) {
    u8 count=0;
    P0=P1=P2=P3=0xff;
    TMOD=0x01;
    ADC_CLK=0;
    RTC_CE=RTC_CLK=0;
    lcd_init();
    rtc_init();
    start_temperature();
    for(;;) {
        scan_keys();
        if(count==0 || count%10==0) {
            humidity=adc_read(0); light=adc_read(1);
            rtc_read(); refresh();
        }
        control();
        if(chirp) chirp--;
        delay_us(10000);
        if(++count==80) {
            count=0;
            sample_temperature();
        }
    }
}

