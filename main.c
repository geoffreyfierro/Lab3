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
    P5->SEL0 |= BIT1;

    //Set ADC14ENC to enable ADC configuration
    //Bits in control register can only be modified when ADC14ENC = 0
    // Bit has to be set to 1 for ADC conversion operation
    ADC14->CTL0 &= ~BIT1;
    // OR .. ADC14->CTL0 &= !ADC14_CTL0_ENC;

    //Set Clock Divider 4
    ADC14->CTL0 |= (1<<30);//Bit 30 to 1
    ADC14->CTL0 &= ~(1<<31);//Bit 31 to 0

    //Set ADC14SHSx to ADC14SC bit
    ADC14->CTL0 &= ~(1<<29);
    ADC14->CTL0 &= ~(1<<28);
    ADC14->CTL0 &= ~(1<<27);

    //Set ADC14SHP to SAMPCON signal
    ADC14->CTL0 |= (1<<26);

    //Set ADC14SSELx to MCLK
    ADC14->CTL0 &= ~(1<<21);
    ADC14->CTL0 |= (1<<20);
    ADC14->CTL0 |= (1<<19);

    //Set ADC14SHT0x to 32
    ADC14->CTL0 &= ~(1<<11);
    ADC14->CTL0 &= ~(1<<10);
    ADC14->CTL0 |= (1<<9);
    ADC14->CTL0 |= (1<<8);

    //Turn on ADC power
    ADC14->CTL0 |= (1<<4);

    //Set ADC14INCHx to A4 for x = 0
    ADC14->MCTL[0] &= ~(0x1F);
    ADC14->MCTL[0] |= (1<<2);

    //Enable conversion
    ADC14->CTL0 |= 0b1;
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

int read_ADC()
{
    ADC14->CTL0 |= BIT1; // enable bit to allow conversion
    /*
    while(ADC14->CTL0[16] == 0b1)
    {
         busy_wait();   //Wait for conversion to complete
    }
    */

    //Get ADC Conversion Result

    ADC14->CTL0 &= ~BIT1; // set bit back to 0 after conversion 

    uint16_t conversion_result = ADC14->MEM[0];

    return conversion_result * (3.3/16384);
}

void convert_ADC_result_to_Vin(int conversion_result)
{

    ADC14->CTL0 |= BIT1;
    int temp = conversion_result * 1000;
    display[3] = temp / 1000;
    temp = temp % 1000;
    display[2] = temp / 100;
    temp = temp % 100;
    display[1] = temp / 10;
    temp = temp % 10;
    display[0] = temp;

    ADC14->CTL0 &= ~BIT1
    
}

void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer
	simple_clock_init();
	GPIO_init();

	ADC_config();

	while(1)
	{
	    // get_ADC_conversion_result(); // start conversion, polling, and read result 
	    //convert_ADC_result_to_Vin(); // convert adc reading to array of digits
	    //display_ADC_result_to_Vin(); // display on 4-digit display
	}
}
//ADC Lab 3
