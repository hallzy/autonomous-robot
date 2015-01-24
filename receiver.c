#include <stdio.h>
#include <at89lp51rd2.h>
#include <stdbool.h>
// ~C51~
#define CLK 22118400L
#define BAUD 115200L
#define BRG_VAL (0x100-(CLK/(32L*BAUD)))
#define VERBOSE 1

#define FREQ 10000L   // was 10000L before
#define TIMER0_RELOAD_VALUE (65536L-(CLK/(12L*FREQ)))




// Variables
float rightvoltage;
float leftvoltage;
float desiredvoltage = 2100;  // in mV
int delay =0;
unsigned char command;
unsigned char spin = 0;
float BUFFER = 0;
int s = 0;


volatile unsigned char pwmCountRightForward = 0;
volatile unsigned char pwmCountRightReverse = 0;
volatile unsigned char pwmCountLeftForward = 0;
volatile unsigned char pwmCountLeftReverse = 0;
volatile unsigned char pwmRightForward = 0;
volatile unsigned char pwmRightReverse = 0 ;
volatile unsigned char pwmLeftForward = 0;
volatile unsigned char pwmLeftReverse = 0;

void wait_bit_time()
{
_asm 
;For a 22.1184MHz crystal one machine cycle 
;takes 12/22.1184MHz=0.5425347us
mov R2, #2 ;;was 14 ; was 3; was 1 for immediate response
L9: mov R1, #124
L8: mov R0, #184
L7: djnz R0, L7 ; 2 machine cycles-> 2*0.5425347us*184=200us
djnz R1, L8 ; 200us*250=0.05s
djnz R2, L9 
ret
_endasm;
}


void wait_one_and_half_bit_time()
{
_asm 
;For a 22.1184MHz crystal one machine cycle 
;takes 12/22.1184MHz=0.5425347us
mov R2, #4 ;; was 30 ; was 7 was 3 for immediate response
L91: mov R1, #124
L81: mov R0, #184
L71: djnz R0, L71 ; 2 machine cycles-> 2*0.5425347us*184=200us
djnz R1, L81 ; 200us*250=0.05s
djnz R2, L91 ; 0.05s*20=0.25s
ret
_endasm;
}

// Functions
int follow();
void readvoltage();
void ParallelPark(int);

// Interrupt
void pwmcounter (void) interrupt 1
{

	if(spin == 0)
	{
	if(++pwmCountRightForward>99) pwmCountRightForward=0;
	P0_3=(pwmRightForward>pwmCountRightForward)?1:0; //P1_0
	
	if(++pwmCountRightReverse>99) pwmCountRightReverse=0;
	P0_4=(pwmRightReverse>pwmCountRightReverse)?1:0; // P1_1
	
	if(++pwmCountLeftReverse>99) pwmCountLeftReverse=0;
	P0_2=(pwmLeftReverse>pwmCountLeftReverse)?1:0; // P1_3
	
	if(++pwmCountLeftForward>99) pwmCountLeftForward=0;
	P0_1=(pwmLeftForward>pwmCountLeftForward)?1:0; // P1_2
	}
	else
	{
		if(++pwmCountRightForward>99) pwmCountRightForward=0;
	P0_3=(pwmRightForward>pwmCountRightForward)?0:1; //P1_0
	
	if(++pwmCountRightReverse>99) pwmCountRightReverse=0;
	P0_4=(pwmRightReverse>pwmCountRightReverse)?0:1; // P1_1
	
	if(++pwmCountLeftReverse>99) pwmCountLeftReverse=0;
	P0_2=(pwmLeftReverse>pwmCountLeftReverse)?0:1; // P1_3
	
	if(++pwmCountLeftForward>99) pwmCountLeftForward=0;
	P0_1=(pwmLeftForward>pwmCountLeftForward)?0:1; // P1_2
	}
}

unsigned char _c51_external_startup(void)
{
// Configure ports as a bidirectional with internal pull-ups.
P0M0=0; P0M1=0;
P1M0=0; P1M1=0;
P2M0=0; P2M1=0;
P3M0=0; P3M1=0;
AUXR=0B_0001_0001; // 1152 bytes of internal XDATA, P4.4 is a general purpose I/O
P4M0=0; P4M1=0;
// The AT89LP51RB2 has a baud rate generator:
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

void wait()
{
	_asm	
		;For a 22.1184MHz crystal one machine cycle 
		;takes 12/22.1184MHz=0.5425347us
	    mov R2, #20 ;20
	L3:	mov R1, #248 ;248 
	L2:	mov R0, #183
	L1:	djnz R0, L1 ; 2 machine cycles-> 2*0.5425347us*184=200us
	    djnz R1, L2 ; 200us*250=0.05s
	    djnz R2, L3 ; 0.05s*20=1s
	    ret
    _endasm;
}


void wait180()
{
	_asm	
		;For a 22.1184MHz crystal one machine cycle 
		;takes 12/22.1184MHz=0.5425347us
	    mov R2, #18
	x6:	mov R1, #248
	x5:	mov R0, #184
	x4:	djnz R0, x4 ; 2 machine cycles-> 2*0.5425347us*184=200us
	    djnz R1, x5 ; 200us*250=0.05s
	    djnz R2, x6 ; 0.05s*20=0.85s
	    ret
    _endasm;
}
void waitParallel()
{
	_asm	
		;For a 22.1184MHz crystal one machine cycle 
		;takes 12/22.1184MHz=0.5425347us
	    mov R2, #5
	L19:	mov R1, #248
	L18:	mov R0, #184
	L17:	djnz R0, L17 ; 2 machine cycles-> 2*0.5425347us*184=200us
	    djnz R1, L18 ; 200us*250=0.05s
	    djnz R2, L19 ; 0.05s*20=0.85s
	    ret
    _endasm;
}

void wait1ms() // its 11ms atm
{
	_asm	
		;For a 22.1184MHz crystal one machine cycle 
		;takes 12/22.1184MHz=0.5425347us
	x61:	mov R1, #35
	x51:	mov R0, #184
	x41:	djnz R0, x41 ; 2 machine cycles-> 2*0.5425347us*184=200us
	    djnz R1, x51 ; 200us*250=0.05s
	    ret
    _endasm;
}

void SPIWrite( unsigned char value)
{
SPSTA&=(~SPIF); // Clear the SPIF flag in SPSTA
SPDAT=value;
while((SPSTA & SPIF)!=SPIF); //Wait for transmission to end
}
// Read 10 bits from the MCP3004 ADC converter
unsigned int GetADC(unsigned char channel)
{
unsigned int adc;
// initialize the SPI port to read the MCP3004 ADC attached to it.
SPCON&=(~SPEN); // Disable SPI
SPCON=MSTR|CPOL|CPHA|SPR1|SPR0|SSDIS;
SPCON|=SPEN; // Enable SPI
P1_4=0; // Activate the MCP3004 ADC.
SPIWrite(channel|0x18); // Send start bit, single/diff* bit, D2, D1, and D0 bits.
for(adc=0; adc<10; adc++); // Wait for S/H to setup
SPIWrite(0x55); // Read bits 9 down to 4
adc=((SPDAT&0x3f)*0x100);
SPIWrite(0x55); // Read bits 3 down to 0
P1_4=1; // Deactivate the MCP3004 ADC.
adc+=(SPDAT&0xf0); // SPDR contains the low part of the result.
adc>>=4;
return adc;
}
unsigned char rx_byte ( int min )
{
	unsigned char j, val;
	unsigned int v;
	//Skip the start bit
	val=0;
	wait_one_and_half_bit_time();   // changed this from 1.5 bit time
	for(j=0; j<3; j++)
	{
	v=(GetADC(0)*1000);
	val|=(v>min)?(0x01<<j):0x00;
	P2_0 = !P2_0; // added this right here
	printf(" \n v: %d", v);
	printf(" \n val: %d", val);
	
	wait_bit_time();
	}
	//Wait for stop bits
	wait_one_and_half_bit_time();
	return val;
}



float voltage (unsigned char channel)
{
return ( (GetADC(channel)*4.77)/1023.0 ); // VCC=4.77V (measured)
}
// Signal LP51B MCP3004
//---------------------------
// MISO - P1.5 - pin 10
// SCK - P1.6 - pin 11
// MOSI - P1.7 - pin 9
// CE* - P1.4 - pin 8
// 4.8V - VCC - pins 13, 14
// 0V - GND - pins 7, 12
// CH0 - - pin 1
// CH1 - - pin 2
// CH2 - - pin 3
// CH3 - - pin 4
void main (void)
{
P2_0 = 0;
	printf("\n");
 while(1)
   {
  	BUFFER = (desiredvoltage * .05);//Calibrated for max sensitivity
    delay++;
    if(delay % 50 == 0)
	readvoltage();
	follow();
   }
}


void readvoltage()
{

	if(spin == 0)
	{
		rightvoltage = (voltage(0)*1000);
		leftvoltage = (voltage(1)*1000);
	}
	
	else if(spin ==1)
	{
		leftvoltage = (voltage(0)*1000);
		rightvoltage = (voltage(1)*1000);
	
	}




if(VERBOSE == 1)
	{
	printf("\r Left voltage is: %4.2f", rightvoltage);
	printf(" Right voltage is: %4.2f", leftvoltage);
	}

}

void Spin180()
{
	//Clockwise
	pwmLeftForward = 100;
	pwmLeftReverse = 0;
	pwmRightForward = 0;
	pwmRightReverse = 100;
	wait180();
	pwmRightForward = 0;
	pwmLeftForward = 0;
	pwmRightReverse = 0;
	pwmLeftReverse = 0;
		
	if (spin == 1){
		desiredvoltage = 2000;
		spin = 0;
	}
	else{
		spin = 1;
		desiredvoltage = 2500;
		}
	
}

int follow()
{
	
	readvoltage();
	if(rightvoltage < (desiredvoltage-BUFFER ) && leftvoltage < (desiredvoltage-BUFFER ) )
	{
		pwmRightForward = 75;
		pwmLeftForward = 75;
		pwmRightReverse = 0;
		pwmLeftReverse = 0;	
	}

	if( rightvoltage > (desiredvoltage+BUFFER ) && leftvoltage > (desiredvoltage+BUFFER ) )
	{
		pwmRightForward = 0;
		pwmLeftForward = 0;
		pwmRightReverse = 75;
		pwmLeftReverse = 75;
	}
	
	if( rightvoltage < (desiredvoltage-BUFFER ) && leftvoltage > (desiredvoltage+BUFFER ) )
	{
		pwmRightForward = 70;
		pwmLeftForward = 0;
		pwmRightReverse = 0;
		pwmLeftReverse = 70;	
	}
	
	
	if( rightvoltage > (desiredvoltage+BUFFER ) && leftvoltage < (desiredvoltage-BUFFER ) )
	{
		pwmRightForward = 0;
		pwmLeftForward = 70;
		pwmRightReverse = 70;
		pwmLeftReverse = 0;
	}
	
	while( rightvoltage > (desiredvoltage-BUFFER ) && rightvoltage < (desiredvoltage +BUFFER ) && leftvoltage > (desiredvoltage-BUFFER ) && leftvoltage < (desiredvoltage+BUFFER ))
	 {
	 
	 //	command = 0;
	 	pwmRightForward = 0;
		pwmLeftForward = 0;
		pwmRightReverse = 0;
		pwmLeftReverse = 0;
	
		
		readvoltage();
	//	wait1ms(); // I killed this, shaved off a second
		while(s < 50)
		{	
		readvoltage();
		if(rightvoltage < 400 && leftvoltage < 400)
		{
			while(rightvoltage < 400 && leftvoltage < 400)
					readvoltage();
					
			command = rx_byte(400);
			printf("\n %d \n",command);
	
			if(command == 1)
			{
				desiredvoltage += 400;
				if(desiredvoltage > 2500)
					desiredvoltage = 2500;
						
				break;		
			}
			if(command == 3 || command == 7)
			{
				desiredvoltage -= 400;
				if(desiredvoltage < 400)
					desiredvoltage = 300;
					
				break;
			}
			if(command == 5)
			{
				Spin180();
				break;
			}
			if(command == 6)
			{
		   		command= 0;
				ParallelPark(0);
				wait();
				while(command != 6)
				{
					command = rx_byte(400);
					wait_bit_time();
				//wait();
				}
				break;
			}
			
		}
		s++;
		}
		s = 0;
		
			
		
		
	 }
	 
	 
	
		return 0;
	
}

void ParallelPark(int x)
{
	//Park on Left Side
	if(x == 0)
	{
		// 45 degree Clockwise Turn
		pwmLeftForward = 100;
		pwmLeftReverse = 0;
		pwmRightForward = 0;
		pwmRightReverse = 100;
		waitParallel();
			pwmRightForward = 0;
		pwmLeftForward = 0;
		pwmRightReverse = 0;
		pwmLeftReverse = 0;
	
		//Backwards
		pwmLeftForward = 0;
		pwmLeftReverse = 100;
		pwmRightForward = 0;
		pwmRightReverse = 100;
		wait180();
			pwmRightForward = 0;
		pwmLeftForward = 0;
		pwmRightReverse = 0;
		pwmLeftReverse = 0;
	
		//45 degree Counterclockwise turn
		pwmLeftForward = 0;
		pwmLeftReverse = 75;
		pwmRightForward = 75;
		pwmRightReverse = 0;
		waitParallel();
		pwmRightForward = 0;
		pwmLeftForward = 0;
		pwmRightReverse = 0;
		pwmLeftReverse = 0;
	}
	else if(x==1) // Park on Right Side
	{
		// 45 degree counter-Clockwise Turn
		pwmLeftForward = 0;
		pwmLeftReverse = 90;
		pwmRightForward = 90;
		pwmRightReverse = 0;
		waitParallel();
		pwmRightForward = 0;
		pwmLeftForward = 0;
		pwmRightReverse = 0;
		pwmLeftReverse = 0;
	
		//Backwards
		pwmLeftForward = 0;
		pwmLeftReverse = 100;
		pwmRightForward = 0;
		pwmRightReverse = 100;
		wait180();
		pwmRightForward = 0;
		pwmLeftForward = 0;
		pwmRightReverse = 0;
		pwmLeftReverse = 0;
	
		//45 degree Counterclockwise turn
		pwmLeftForward = 85;
		pwmLeftReverse = 0;
		pwmRightForward = 0;
		pwmRightReverse = 85;
		waitParallel();
		pwmRightForward = 0;
		pwmLeftForward = 0;
		pwmRightReverse = 0;
		pwmLeftReverse = 0;
	}
	else
		printf("error");
	

}