/* Name: main.c
 * Author: Harry Johnson
 * License: CC BY/SA unported.
 * SPI interfaced True Hardware Random Number Generator: Keep shifting in bytes, and it will keep shifting random bytes out. 
 * This is an SPI slave, and technically doesn't need to recieve data, but the MOSI line is included for future use. (There is spare memory space..) 
 * Setup in the main routine, everything else is handled in ISRs. One ISR updates the random value, and another ISR handles SPI communication.
 *
 * PINOUT:
 * PB0: MOSI
 * PB1: MISO
 * PB2: SCK
 * PB3: CS/SS (Slave Select)
 * PB4: Input from RNG hardware.
 * PB5: /RESET
 */

#include <avr/io.h> //Device-specific IO pins.
#include <avr/interrupt.h> //Interrupts
#include <stdint.h> //standard int types.

#define mixInBit(bit)	randomNumber = (randomNumber >> 1) | (((bit ^ (randomNumber >> 0) ^ (randomNumber >> 3)) & 1) << 24) //Feedback Shift Register to mix in a new bit to the number.

volatile unsigned long randomNumber = 0b1010101010101010101010101; //32 bit random number, always being refreshed.
volatile unsigned char lastbit = 255; //what the last input digit was, or 255 if we want to force a start-over. (after outputting a bit)

int main(void)
{
    WDTCR |= (1<<WDCE) | (WDE); 
    WDTCR &= ~(1<<WDE); //disable the watchdog timers.

    ADCSRA = 0; //disable the ADC, just in case.
    PRR |= (1<<PRADC) | (1<<PRTIM0); //power down the ADC and timer0.
    ACSR |= ~(1<<ACIE); 
    ACSR |= (1<<ACD); //disable the analog comparator.
    
    cli(); //disable interrupts for the meantime.
    DDRB = (1<<1); //PB1 is MISO pin, everything else is inputs.
    PORTB |= (1<<3); //enable CS pin internal pullup.
    PCMSK = (1<<PCINT3); //pin change interrupt 3 enabled, everything else disabled. (CS pin only)
    GIMSK |= (1<<PCIE); //pin change interrupts enabled in general.
    
    TIMSK = (1<<OCIE1A); //enable output compare match interrupt.
    OCR1A = 0x40; //trigger every 8Mhz/128 = 62500 Hz, oversampling noise, which is around 20Khz or so. 
    GTCCR = 0; //no PWM or anything of the sort.
    TCCR1 = (1<<CS10);// | (1<<CS10); //reset counter 1, and set clock source to System Clock (8Mhz).
    
    USICR = (1<<USIWM0) | (1<<USICS1); //Enable SPI, and select the Clock pin to be the clock source.
    sei(); //re-enable interrupts.
    for(;;){
    }
    return 0;   /* never reached */
}

//USI (SPI) overflow vector: Every 8 bits, this triggers.
//if the value sent from the master is from 1-2, then make that the number of bytes of random number to send. 
//if it isn't, and there is a byte to send, send it, and decrement the byte send counter.
ISR(USI_OVF_vect) { 
    USIDR = randomNumber & 0xFF; //return the lowest byte of the current number. Result will be decimal integer, 0-255.
    USISR |= (1<<USIOIF); //clear overflow register.
}

ISR(PCINT0_vect) { //pin change vector: only CS pin is enabled, so this must be a transition on CS.
    if((PINB & (1<<3))==1) { //CS is high, disable SPI.
        DDRB &= ~(1<<1); //MISO pin disable.
        USICR &= ~(1<<USIOIE); //disable the SPI overflow vector.
    } else { //else CS is low. Time to get into SPI mode!
        DDRB |= (1<<1); //MISO pin re-enable
        USISR &= 0xF0; //reset the clock.
        USISR |= (1<<USIOIF); //clear overflow interrupt flag, just in case.
        USICR |= (1<<USIOIE); //enable the overflow interrupt.  
}
    
}

ISR(TIM1_COMPA_vect) {  //triggers on a 62500KHz clock. ((8MHz/128): 1 for prescaler, 128 for counter. random input on PB4.
    TCNT1=0; //reset the counter
    if((lastbit != (PINB&(1<<4))) && (lastbit != 255)) { //if there are 2 bits, and they aren't equal. (Von Neumann)
        mixInBit(lastbit); // update the number.
        lastbit = 255; //and start the sequence of capturing another 2 bits.
    } else {
        lastbit = (PINB&(1<<4)); //capture the first bit.
    }
}
