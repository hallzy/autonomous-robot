#include <stdio.h>
#include <at89lp51rd2.h>

// ~C51~ 
 
#define CLK 22118400L
#define BAUD 115200L
#define BRG_VAL (0x100-(CLK/(32L*BAUD)))

//We want timer 0 to interrupt every 100 microseconds ((1/10000Hz)=100 us)
#define FREQ (15050L * 2L) // 15050 is good,  15150 is the original
#define TIMER0_RELOAD_VALUE (65536L-(CLK/(12L*FREQ)))

//These variables are used in the ISR



//Functions
unsigned char _c51_external_startup(void)
{
	// Configure ports as a bidirectional with internal pull-ups.
	P0M0=0;	P0M1=0;
	P1M0=0;	P1M1=0;
	P2M0=0;	P2M1=0;
	P3M0=0;	P3M1=0;
	AUXR=0B_0001_0001; // 1152 bytes of internal XDATA, P4.4 is a general purpose I/O
	P4M0=0;	P4M1=0;
    
    // Initialize the serial port and baud rate generator
    PCON|=0x80;
	SCON = 0x52;
    BDRCON=0;
    BRL=BRG_VAL;
    BDRCON=BRR|TBCK|RBCK|SPD;
	
	// Initialize timer 0 for ISR 'pwmcounter()' below
	TR0=0; // Stop timer 0
	TMOD=0x01; // 16-bit timer
	// Use the autoreload feature available in the AT89LP51RB2
	// WARNING: There was an error in at89lp51rd2.h that prevents the
	// autoreload feature to work.  Please download a newer at89lp51rd2.h
	// file and copy it to the crosside\call51\include folder.
	TH0=RH0=TIMER0_RELOAD_VALUE/0x100;
	TL0=RL0=TIMER0_RELOAD_VALUE%0x100;
	TR0=1; // Start timer 0 (bit 4 in TCON)
	ET0=1; // Enable timer 0 interrupt
	EA=1;  // Enable global interrupts
    
    return 0;
}

// Interrupt 1 is for timer 0.  This function is executed every time
// timer 0 overflows: 100 us.
void pwmcounter (void) interrupt 1
{
//	printf("Do we get here INT");
	P1_1 = P1_0;
	P1_0 = !P1_0;
	
}

void wait_bit_time()
{
_asm 
;For a 22.1184MHz crystal one machine cycle 
;takes 12/22.1184MHz=0.5425347us
mov R2, #3 ;was 20 ;was 5 ; 2for immediate response
L9: mov R1, #124
L8: mov R0, #184
L7: djnz R0, L7 ; 2 machine cycles-> 2*0.5425347us*184=200us
djnz R1, L8 ; 200us*250=0.05s
djnz R2, L9 
ret
_endasm;
}

void buttonprep()
{

P1_7 = 0;
P0_3 = 0;
P0_1 = 0;
P0_2 = 0;

}

void tx_byte ( unsigned char val )
{
unsigned char j;
//Send the start bit
TR0=0;
wait_bit_time();
TR0=1;
wait_bit_time();

for (j=0; j<3; j++)
{
TR0=val&(0x01<<j)?1:0;
wait_bit_time();
}
TR0=1;
//Send the stop bits
wait_bit_time(); 
wait_bit_time();
}

void main (void)
{
buttonprep();

while(1)
{
		if(P1_7 == 1)
		{	
			tx_byte(1);
		}
		else if(P0_3 == 1)
		{
			tx_byte(3);
		}
		else if(P0_1 == 1)
		{
			tx_byte(5);
		}
		else if(P0_2 == 1)
		{
			tx_byte(6);	
		}
	


}	
}