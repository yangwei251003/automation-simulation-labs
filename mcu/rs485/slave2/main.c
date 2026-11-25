
#include <reg51.h>

#define MY_ADDR     0x02        
#define MY_ID       '2'         
#define CMD_ON      0x01
#define CMD_OFF     0x00
#define ACK_OK      0x0F
#define ACK_ERR     0xF0
#define FRAME_MAX   8

sbit DE_RE = P3^2;              
sbit LED   = P2^0;              


void Delay_bit(void)
{
    unsigned char i;
    for(i = 0; i < 30; i++);
}

void Delay_turnaround(void)
{
    unsigned char i, j;
    for(i = 0; i < 5; i++)
        for(j = 0; j < 100; j++);
}


void UART_Init(void)
{
    TMOD = 0x21;
    TH1  = 0xFD;  TL1 = 0xFD;
    PCON = 0x00;
    SCON = 0xD0;                
    TR1  = 1;
    TR0  = 0;  TF0 = 0;
    TI   = 0;  RI  = 0;
    DE_RE = 0;
}

bit Recv_Byte(unsigned char *dat)
{
    TH0 = 0;  TL0 = 0;  TF0 = 0;  TR0 = 1;
    while(!RI)
    {
        if(TF0)
        {
            TR0 = 0;  TF0 = 0;
            return 0;
        }
    }
    TR0 = 0;  TF0 = 0;
    *dat = SBUF;
    RI = 0;
    return 1;
}

void Send_Ack(unsigned char ack)
{
    Delay_turnaround();
    DE_RE = 1;
    TB8 = 0;
    TI = 0;  SBUF = ack;  while(!TI);  TI = 0;
    Delay_bit();
    DE_RE = 0;
}

void Send_Frame(unsigned char *buf, unsigned char len)
{
    unsigned char i, bcc;
    Delay_turnaround();
    DE_RE = 1;
    bcc = len;
    TB8 = 0;
    TI = 0;  SBUF = len;  while(!TI);  TI = 0;
    for(i = 0; i < len; i++)
    {
        bcc ^= buf[i];
        TB8 = 0;
        TI = 0;  SBUF = buf[i];  while(!TI);  TI = 0;
    }
    TB8 = 0;
    TI = 0;  SBUF = bcc;  while(!TI);  TI = 0;
    Delay_bit();
    DE_RE = 0;
}

unsigned char Recv_Frame(unsigned char *buf)
{
    unsigned char len, i, bcc, dat;

    if(!Recv_Byte(&dat)) return 2;
    if(RB8) return 2;                   
    len = dat;
    if(len == 0 || len > FRAME_MAX) return 1;

    bcc = len;
    for(i = 0; i < len; i++)
    {
        if(!Recv_Byte(&dat)) return 2;
        if(RB8) return 2;
        buf[i] = dat;
        bcc ^= dat;
    }
    if(!Recv_Byte(&dat)) return 2;
    if(RB8) return 2;
    bcc ^= dat;
    return (bcc == 0) ? 0 : 1;
}


void main(void)
{
    unsigned char rx[FRAME_MAX];
    unsigned char tx[3];
    unsigned char st, dat, tries;

    LED   = 1;                  
    DE_RE = 0;
    UART_Init();

    while(1)
    {
        
        SM2 = 1;
        RI  = 0;
        while(1)
        {
            if(Recv_Byte(&dat))
            {
                if(dat == MY_ADDR) break;    
            }
        }

        
        SM2 = 0;                            
        Send_Ack(MY_ADDR);

        
        st = Recv_Frame(rx);
        if(st == 1)                         
        {
            Send_Ack(ACK_ERR);              
            continue;                       
        }
        if(st == 2) continue;               

        
        if(rx[0] == CMD_ON)  LED = 0;       
        else                 LED = 1;       

        
        Send_Ack(ACK_OK);

        
        if(rx[0] == CMD_ON)
        {
            tx[0] = '#';
            tx[1] = MY_ID;
            tx[2] = '#';
            for(tries = 0; tries < 3; tries++)
            {
                Send_Frame(tx, 3);
                if(Recv_Byte(&dat))         
                {
                    if(dat == ACK_OK) break;    
                    else              break;    
                }
            }
        }
    }
}