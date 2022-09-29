#include "msp.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define ADC14_CTL0_ENC ((uint32_t)0x00000002) //ADC14 enable conversion


/**
 * main.c
 */
 
 int count = 0;
 int k = 0;
 int conversion_result = 0;
 int display[4] = {0b11011111, 0b11101111, 0b11110111, 0b11111011};
 int sseg_table[10]={0b11000000,0b11111001,0b10100100,0b10110000,
    0B10011001,0b10010010,0b010000010,0b11111000,
    0b10000000,0b10010000};

void simple_clock_init(void) // sets up clock and associated wait-state settings
{
    //Set power level for the desired clock frequency
    while (PCM->CTL1 & 0x00000100) {;} //wait for PCM to become not busy
    uint32_t key_bits = 0x695A0000; //0x695A is the key code
    uint32_t AM_LDO_VCORE1_bits = 0x00000100; //AMR Active Mode Request - 01b = AM_LDO_VCORE1
    PCM->CTL0 = key_bits | AM_LDO_VCORE1_bits; //unlock PCM register and set power mode
    while (PCM->CTL1 & 0x00000100); //wait for PCM to become not busy
    PCM->CTL0 &= 0x0000FFFF; //lock PCM register again
    //Flash read wait state number change
    FLCTL->BANK0_RDCTL &= ~(BIT(12) | BIT(13) | BIT(14) | BIT(15)); //reset bits
    FLCTL->BANK0_RDCTL |= BIT(12); //bit 12~15: wait state selection. 0001b=1 wait states
    FLCTL->BANK1_RDCTL &= ~(BIT(12) | BIT(13) | BIT(14) | BIT(15)); //reset bits
    FLCTL->BANK1_RDCTL |= BIT(12); //bit 12~15: wait state selection. 0001b = 1 wait states
    //Clock source: DCO, nominal DCO frequency: 48MHz
    CS->KEY = 0x695A; //unlock CS registers
    CS->CTL0 |= BIT(18) | BIT(16); //bit 16~18 DCORSEL frequency range select 101b = 48Mhz
    CS->CTL0 |= BIT(23); //bit 23 - DCOEN, enables DCO oscillator
    //clock module that uses DCO: MCLK
    CS->CTL1 &= ~(BIT(16) | BIT(17) | BIT(18)); //source divider = 1
    CS->CTL1 &= ~(BIT0 | BIT1 | BIT2 ); //reset all bits
    CS->CTL1 |= 0x3; //bits 0 to 2 - SELM, selects MCLK source. 011b = DCOCLK (by default)
    CS->CLKEN |= BIT1; //bit 1 - MCLK_EN, enable MCLK (by default)
    CS->KEY = 0x0; //lock CS registers
    while (!(CS->STAT & BIT(25))){} //while MCLK isn't steady
}

void ADC_config()
{
    P5->SEL1 |= BIT1;
    P5->SEL0 |= BIT1; // I/O Function selection, P5.1, A4 channel
			

    //Set ADC14ENC to enable ADC configuration
    //Bits in control register can only be modified when ADC14ENC = 0
    // Bit has to be set to 1 for ADC conversion operation
    ADC14->CTL0 &= BIT1;
    // OR .. ADC14->CTL0 &= !ADC14_CTL0_ENC;
   
    //Set Predivider 
    ADC14->CTL0 |= (1<<31); 
    ADC14->CTL0 &= ~(1<<30);
  
    //Set Clock Divider 4
    ADC14->CTL0 &= ~(1<<24);//Bit 24 to 0
    ADC14->CTL0 |= (1<<23);//Bit 23 to 1
    ADC14->CTL0 |= (1<<22); //Bit 22 to 1

    //Set ADC14SHSx to ADC14SC bit, select which source triggers ADC to start conversion
	// all 0s so software (9ADC14SC) triggers 
    ADC14->CTL0 &= ~(1<<29);
    ADC14->CTL0 &= ~(1<<28);
    ADC14->CTL0 &= ~(1<<27);

    //Set ADC14SHP to SAMPCON signal, pulse sample mode
    ADC14->CTL0 |= (1<<26);

    //Set ADC14SSELx to MCLK, 011b = MCLK
    ADC14->CTL0 &= ~(1<<21);
    ADC14->CTL0 |= (1<<20);
    ADC14->CTL0 |= (1<<19);
	
    //Sets ADC14CONSEQx to single-channel single-conversion
    ADC14->CTL0 &= ~(1<<18);
    ADC14->CTL0 &= ~(1<<17);

    //Set ADC14SHT0x to 32, number of cycles in sample period
    ADC14->CTL0 &= ~(1<<11);
    ADC14->CTL0 &= ~(1<<10);
    ADC14->CTL0 |= (1<<9);
    ADC14->CTL0 |= (1<<8);

    //Set ADC14INCHx to A4 for x = 0
    ADC14->MCTL[0] &= ~(0x1F);
    ADC14->MCTL[0] |= (1<<2);
	
    ADC14->CTL1 |= 0x0000;
    
    //Sets ADC14DF to unsigned binary format	
	
    ADC14->CTL1 &= ~(1<<3);

    // Sets ADC14VRSEL reference voltages to 3.3v and 0V
	
    ADC14->CTL1 &= ~(1<<11);
    ADC14->CTL1 &= ~(1<<10);
    ADC14->CTL1 &= ~(1<<9);
    ADC14->CTL1 &= ~(1<<8);

    // Sets ADC14RES to 14-bit ADC
    ADC14->CTL1 |= 0x03;

    // Sets ADC14LO0 to 0x0000
    ADC14->LO0 |= 0x0000;

    // Sets ADC14HI0
    ADC14->HI0 |= 0x3FFFF; 

    //Turn on ADC power, bit 4 = 1
    ADC14->CTL0 |= (1<<4);

    //Set ADC14ENC to 1
    ADC14->CTL0 |= (1<<2);

    //Start conversion
    ADC14->CTL0 |= (1<<0);
     
}

void GPIO_init()
{
    //Set P8.2-5 to output for digits
    P8->DIR |= BIT2 | BIT3 | BIT4 | BIT5;
    //Set P4.0-6 to output for segments
    P4->DIR |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6;
}

void busy_wait()
{
    count = 0;
    
    while (count < 1000)
    {
        count++;
    }
}

int get_ADC_conversion_result()
{
    //ADC14->CTL0 |= BIT1; // enable bit to allow conversion
    //ADC14->CTL0 |= BIT0; // start conversion bit, resets to zero after conversion
    
    while(ADC14->CTL0 & 0x00010000){;} // busy bit indicating active sample or conversion operation in session
    
    ADC14->CTL0 &= ~BIT1; // set bit back to 0 after conversion 

    uint16_t conversion_result = ADC14->MEM[0];

    return conversion_result * (3.3/16384);
}

void convert_ADC_result_to_Vin(int conversion_result)
{

    int temp = conversion_result * 1000;
    display[3] = temp / 1000;
    temp = temp % 1000;
    display[2] = temp / 100;
    temp = temp % 100;
    display[1] = temp / 10;
    temp = temp % 10;
    display[0] = temp;
	
}
void display_ADC_result_to_Vin() // simlar to keypad code in lab 1, task a
{

	P4->OUT = display[k];
	P8->OUT = sseg_table[k];
	
	if (k>=3)
	   k=0;
	else
	    k=k+1;
}
	
void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer
	simple_clock_init();
	GPIO_init();

	ADC_config();

	while(1)
	{
	    // start conversion, polling, and read result
	    conversion_result = get_ADC_conversion_result();
	    convert_ADC_result_to_Vin(conversion_result); // convert adc reading to array of digits
	    display_ADC_result_to_Vin(); // display on 4-digit display
	}
}
//ADC Lab 3
