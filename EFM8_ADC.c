// ADC.c:  Shows how to use the 14-bit ADC.  This program
// measures the voltage from some pins of the EFM8LB1 using the ADC.
//
// (c) 2008-2018, Jesus Calvino-Fraga
//

#include <stdio.h>
#include <stdlib.h>
#include <EFM8LB1.h>
#include "lcd.h"
#include "global.h"

// ~C51~  

unsigned char overflow_count;

#define PIN_SIG1 QFP32_MUX_P1_4
#define PIN_SIG2 QFP32_MUX_P1_5
#define PIN_SIG3 QFP32_MUX_P1_6

char _c51_external_startup (void)
{
	// Disable Watchdog with key sequence
	SFRPAGE = 0x00;
	WDTCN = 0xDE; //First key
	WDTCN = 0xAD; //Second key
  
	VDM0CN=0x80;       // enable VDD monitor
	RSTSRC=0x02|0x04;  // Enable reset on missing clock detector and VDD

	#if (SYSCLK == 48000000L)	
		SFRPAGE = 0x10;
		PFE0CN  = 0x10; // SYSCLK < 50 MHz.
		SFRPAGE = 0x00;
	#elif (SYSCLK == 72000000L)
		SFRPAGE = 0x10;
		PFE0CN  = 0x20; // SYSCLK < 75 MHz.
		SFRPAGE = 0x00;
	#endif
	
	#if (SYSCLK == 12250000L)
		CLKSEL = 0x10;
		CLKSEL = 0x10;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 24500000L)
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 48000000L)	
		// Before setting clock to 48 MHz, must transition to 24.5 MHz first
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x07;
		CLKSEL = 0x07;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 72000000L)
		// Before setting clock to 72 MHz, must transition to 24.5 MHz first
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x03;
		CLKSEL = 0x03;
		while ((CLKSEL & 0x80) == 0);
	#else
		#error SYSCLK must be either 12250000L, 24500000L, 48000000L, or 72000000L
	#endif
	
	P0MDOUT |= 0x10; // Enable UART0 TX as push-pull output
	XBR0     = 0x01; // Enable UART0 on P0.4(TX) and P0.5(RX)                     
	XBR1     = 0X00;
	XBR2     = 0x40; // Enable crossbar and weak pull-ups

	// Configure Uart 0
	#if (((SYSCLK/BAUDRATE)/(2L*12L))>0xFFL)
		#error Timer 0 reload value is incorrect because (SYSCLK/BAUDRATE)/(2L*12L) > 0xFF
	#endif
	SCON0 = 0x10;
	TH1 = 0x100-((SYSCLK/BAUDRATE)/(2L*12L));
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit auto-reload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready
  	
	return 0;
}

void InitADC (void)
{
	SFRPAGE = 0x00;
	ADC0CN1 = 0b_10_000_000; //14-bit,  Right justified no shifting applied, perform and Accumulate 1 conversion.
	ADC0CF0 = 0b_11111_0_00; // SYSCLK/32
	ADC0CF1 = 0b_0_0_011110; // Same as default for now
	ADC0CN0 = 0b_0_0_0_0_0_00_0; // Same as default for now
	ADC0CF2 = 0b_0_01_11111 ; // GND pin, Vref=VDD
	ADC0CN2 = 0b_0_000_0000;  // Same as default for now. ADC0 conversion initiated on write of 1 to ADBUSY.
	ADEN=1; // Enable ADC
}

#define VDD 3.3035 // The measured value of VDD in volts

void InitPinADC (unsigned char portno, unsigned char pinno)
{
	unsigned char mask;
	
	mask=1<<pinno;

	SFRPAGE = 0x20;
	switch (portno)
	{
		case 0:
			P0MDIN &= (~mask); // Set pin as analog input
			P0SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		case 1:
			P1MDIN &= (~mask); // Set pin as analog input
			P1SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		case 2:
			P2MDIN &= (~mask); // Set pin as analog input
			P2SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		default:
		break;
	}
	SFRPAGE = 0x00;
}

unsigned int ADC_at_Pin(unsigned char pin)
{
	ADC0MX = pin;   // Select input from pin
	ADBUSY=1;       // Dummy conversion first to select new pin
	while (ADBUSY); // Wait for dummy conversion to finish
	ADBUSY = 1;     // Convert voltage at the pin
	while (ADBUSY); // Wait for conversion to complete
	return (ADC0);
}

float Volts_at_Pin(unsigned char pin)
{
	 return ((ADC_at_Pin(pin)*VDD)/0b_0011_1111_1111_1111);
}

unsigned int Get_ADC (void)
{
	ADBUSY = 1;
	while (ADBUSY); // Wait for conversion to complete
	return (ADC0);
}

void main (void)
{
	float half_period;
	float peak_sig1;
	float peak_sig2;
	float sig_diff;
	char display_text[17];

    waitms(500); // Give PuTTy a chance to start before sending
	printf("\x1b[2J"); // Clear screen using ANSI escape sequence.
	printf("\x1b[;f"); // Reset cursor position
	printf ("ADC test program\n"
	        "File: %s\n"
	        "Compiled: %s, %s\n\n",
	        __FILE__, __DATE__, __TIME__);
	
	InitPinADC(1, 4); // Configure P1.4 as analog input
	InitPinADC(1, 5); // Configure P1.5 as analog input
	InitPinADC(1, 6); // Configure P1.6 as analog input
	InitPinADC(1, 7); // Configure P1.7 as analog input
    InitADC();

	while(1)
	{
		// Reset the counter
		TL0=0; 
		TH0=0;
		TF0=0;
		overflow_count=0;
		
		while(P0_1!=0); // Wait for the signal to be zero
		while(P0_1!=1); // Wait for the signal to be one
		TR0=1; // Start the timer
		while(P0_1!=0) // Wait for the signal to be zero
		{
			if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
			}
		}
		while(P0_1!=1) // Wait for the signal to be one
		{
			if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
			}
		}
		TR0=0; // Stop timer 0, the 24-bit number [overflow_count-TH0-TL0] has the period!
		half_period=(overflow_count*65536.0+TH0*256.0+TL0)*(12.0/SYSCLK)*1000;

		// // Reset the timer
		// TL0=0; 
		// TH0=0;
		// while (ADC_at_Pin(PIN_SIG1)!=0); // Wait for next zero
		// while (ADC_at_Pin(PIN_SIG1)==0); // Wait for positive
		// TR0=1;
		// waitms(half_period*500); // Wait for 1/4 period
		// ADC_at_Pin(PIN_SIG1);
		// peak_sig1 = Volts_at_Pin(PIN_SIG1); // record peak voltage for reference

		// while (ADC_at_Pin(PIN_SIG2)!=0); // Wait for zero of second signal
		// while (ADC_at_Pin(PIN_SIG2)==0); // Wait for positive
		// waitms(half_period*500); // Wait for 1/4 period
		// TR0=0;
		// sig_diff=(TH0*0x100+TL0);
		// ADC_at_Pin(PIN_SIG2);
		// peak_sig2 = Volts_at_Pin(PIN_SIG2); // record peak voltage for second signal

		peak_sig1=0.0;
		peak_sig2=0.0;
		sig_diff=0.0;

		//peak_sig1 *= 0.7071; // Convert to RMS
		//peak_sig2 *= 0.7071;
		//sig_diff = (360*sig_diff)/(2*half_period); // calculate phase shift in degrees

		// Display values in PuTTY
		printf("\x1b[0K"); // ANSI: Clear from cursor to end of line.
	    printf("\rf=%.4fHz, V1=%.4fV(rms), V2=%.4fV(rms), phase=%.4fdeg", half_period, peak_sig1, peak_sig2, sig_diff);

		// Display values on LCD
		sprintf(display_text, "V1=%.3f, V2=%.3f", peak_sig1, peak_sig2);
		LCDprint(display_text, 1, 1);
		sprintf(display_text, "phase=%.3f", sig_diff);
		LCDprint(display_text, 2, 1);

		waitms(500);
	 }  
}	
