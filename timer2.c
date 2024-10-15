/*
 * timer2.c
 *
 * Author: Peter Sutton
 */

#include "timer2.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// Segment values for digits 0 to 9
uint8_t seven_seg[10] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111};

// The number currently displayed
volatile uint8_t number_to_display; // Default to 42 for testing

// Track which digit to display (0 = right, 1 = left)
volatile uint8_t current_digit = 0;
volatile uint8_t x_or_y = 0;

volatile uint16_t joy_x = 0;
volatile uint16_t joy_y = 0;

void init_timer2(void)
{
	// Setup timer 2.
	TCNT2 = 0;

	/* Set up timer/counter 2 so that it reaches an output compare
	** match every 1 millisecond (1000 times per second) and then
	** resets to 0.
	** We divide the clock by 8 and count 1000 cycles (0 to 999)
	*/
	OCR2A = 249;
	TCCR2A = (0 << COM2A1) | (1 << COM2A0)  // Toggle OC2A on compare match
		| (0 << WGM21) | (0 << WGM20); // Least two significant WGM bits
	TCCR2B = (1 << WGM22) // Two most significant WGM bits
		| (0 << CS22) | (1 << CS21) | (1 <<CS20); // Divide clock by 8

	/* Enable interrupt on timer on output compare match 
	*/
	TIMSK2 = (1<<OCIE2A);

	/* Ensure interrupt flag is cleared */
	TIFR2 = (1<<OCF2A);
	// SET DISPLAY TO ALL Ones
	// DDRA = 0xFF;
	// 1 output, 0 input
	DDRD |= (1 << 2);
	// output 
	PORTC = 0x00;
	
	DDRC = 0xFF;
	
}

ISR(TIMER2_COMPA_vect) {
    // Timer 1 interrupt service routine to update seven-segment display
    uint8_t value;

    // Determine the value to display on the current digit
    if (current_digit == 0) {
        value = number_to_display % 10; // Ones place
    } else {
        value = (number_to_display / 10) % 10; // Tens place
    }

    // Display the current digit
    PORTD = current_digit << 2; // Set PD2 to select right or left digit
    PORTC = seven_seg[value]; // Set segment values

    // Switch to the other digit for the next interrupt
    current_digit = 1 - current_digit;

	uint16_t value_2;
	
	
	/* Set up the serial port for stdin communication at 19200 baud, no echo */
	// init_serial_stdio(19200,0);
	
	// /* Turn on global interrupts */
	// sei();
	
	// Set up ADC - AVCC reference, right adjust
	// Input selection doesn't matter yet - we'll swap this around in the while
	// // loop below.
	ADMUX = (1<<REFS0);
	// // Turn on the ADC (but don't start a conversion yet). Choose a clock
	// // divider of 64. (The ADC clock must be somewhere
	// // between 50kHz and 200kHz. We will divide our 8MHz clock by 64
	// // to give us 125kHz.)
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1);
	
	// /* Print a welcome message
	// */
	// printf("ADC Test\n");
	
	
	// Set the ADC mux to choose ADC0 if x_or_y is 0, ADC1 if x_or_y is 1
	if(x_or_y == 0) {
		ADMUX &= ~1;
	} else {
		ADMUX |= 1;
	}
	// Start the ADC conversion
	ADCSRA |= (1<<ADSC);
	
	
	value_2 = ADC; // read the value
	if(x_or_y == 0) {
		// printf("X: %4d ", value_2);
		joy_x = value_2;
	} else {
		// printf("Y: %4d\n", value_2);
		joy_y = value_2;
	}
	// // Next time through the loop, do the other direction
	x_or_y ^= 1;
	
}
