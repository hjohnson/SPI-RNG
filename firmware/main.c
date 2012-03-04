/* Name: main.c
 * Author: Harry Johnson
 * License: CC BY/SA unported.
 * Master prompts with a command of how many bytes to send. Slave (this device) returns that number of random bytes when clock is toggled through. Pretty simple stuff. 
 * Setup in the main routine, everything else is handled in ISRs. One ISR updates the random value, and another ISR handles SPI communication.
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#define mixInBit(bit)	randomNumber = (randomNumber >> 1) | (((bit ^ (randomNumber >> 0) ^ (randomNumber >> 3)) & 1) << 24)

volatile uint8_t BytesToSend; //number of bytes left to send as slave.
volatile uint32_t randomNumber =0b1010101010101010101010101; //16 bit random number, always being refreshed.
volatile uint8_t lastbit = 255; //what the last input digit was, or -1 if we want to force a start-over. (after outputting a bit)

int main(void)
{
    WDTCR |= (1<<WDCE) | (WDE);
    WDTCR &= ~(1<<WDE); //disable the watchdog timers.
    ADCSRA = 0; //disable the ADC, just in case.
    PRR |= (1<<PRADC); //power down the ADC.
    ACSR |= (1<<ACD); //disable the analog comparator.
    cli(); //disable interrupts for the meantime.
    DDRB = (1<<1); //PB1 is MISO pin, everything else is inputs.
    PORTB |= (1<<3); //enable CS pin internal pullup.
    PCMSK = (1<<PCINT3); //pin change interrupt 3 enabled, everything else disabled. (CS pin only)
    GIMSK = (1<<PCIE); //pin change interrupts enabled in general.
    TIMSK = (1<<TOIE1); //enable rollover interrupt.
    GTCCR = 0; //no PWM or anything of the sort.
    TCCR1 = (1<<CTC1) | (1<<CS11) | (1<<CS10); //reset counter 1, and set clock source to System Clock/4.
    USICR = (1<<USICS1) | (1<<USIOIE); //SPI via USI: 3-wire mode, clock triggered on positive edge, overflow enabled.
    sei(); //re-enable interrupts.
    for(;;){
    }
    return 0;   /* never reached */
}

//USI (SPI) overflow vector: Every 8 bits, this triggers.
//if the value sent from the master is from 1-2, then make that the number of bytes of random number to send. 
//if it isn't, and there is a byte to send, send it, and decrement the byte send counter.
ISR(USI_OVF_vect) { 
    uint8_t value = USIDR;
    if((value>0) && (value<=2)) {
        BytesToSend = value; 
    } else {
        if(BytesToSend >= 0) {
            USIDR=((randomNumber>>(8*BytesToSend-1)) & 0xFF); //load up the next value.
            BytesToSend--;
        }
    }
    USISR |= (1<<USIOIF); //clear overflow register.
}

ISR(PCINT0_vect) { //pin change vector: only CS pin is enabled, so this must be a transition on CS.
    if((PORTB & (1<<3))==1) { //CS is high, disable SPI.
        USICR &= ~(1<<USIWM0); //disable SPI.
    } else { //else CS is low. Time to get into SPI mode!
        USISR |= (1<<USIOIF); //clear overflow interrupt flag, just in case.
        USISR &= ~((1<<USICNT0) | (1<<USICNT1) | (1<<USICNT2) | (1<<USICNT3)); //clear the clock.
        USICR |= (1<<USIWM0); //re-enable SPI.
    }
    
}

ISR(TIMER1_OVF_vect) {  //triggers on a 7.8KHz clock. ((8MHz/4)/256): 4 for prescaler, 256 for counter.
    if((lastbit != (PINB&(1<<4))>>4) && (lastbit != 255)) { //if there are 2 bits, and they aren't equal
       mixInBit(lastbit);
        lastbit = 255; //and start the sequence of capturing another 2 bits.
    } else {
        lastbit = (PINB&(1<<4))>>4; //capture the first bit.
    }
    TIFR |= (1<<TOV1); //reset the timer vector.
}