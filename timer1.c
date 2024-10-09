/*
 * timer1.c
 *
 * Author: Peter Sutton
 */

#include "timer1.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// Segment values for digits 0 to 9
uint8_t seven_seg[10] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111};

// The number currently displayed
volatile uint8_t number_to_display = 42; // Default to 42 for testing

// Track which digit to display (0 = right, 1 = left)
volatile uint8_t current_digit = 0;

void init_timer1(void)
{
	// Setup timer 1.
	TCNT1 = 0;
	/* Set up timer/counter 1 so that it reaches an output compare
	** match every 1 millisecond (1000 times per second) and then
	** resets to 0.
	** We divide the clock by 8 and count 1000 cycles (0 to 999)
	*/
	OCR1A = 999;
	TCCR1A = (0 << COM1A1) | (1 << COM1A0)  // Toggle OC1A on compare match
		| (0 << WGM11) | (0 << WGM10); // Least two significant WGM bits
	TCCR1B = (0 << WGM13) | (1 << WGM12) // Two most significant WGM bits
		| (0 << CS12) | (1 << CS11) | (0 <<CS10); // Divide clock by 8


	
}

ISR(TIMER1_COMPA_vect) {
    // Timer 1 interrupt service routine to update seven-segment display
    uint8_t value;

    // Determine the value to display on the current digit
    if (current_digit == 0) {
        value = number_to_display % 10; // Ones place
    } else {
        value = (number_to_display / 10) % 10; // Tens place
    }

    // Display the current digit
    PORTD = (current_digit == 0) ? (0 << PD2) : (1 << PD2); // Set PD2 to select right or left digit
    PORTA = seven_seg[value]; // Set segment values

    // Switch to the other digit for the next interrupt
    current_digit = 1 - current_digit;
}

void display_digit(uint8_t number, uint8_t digit) {
    // Directly control which digit is selected and set the segment value
    PORTD = (digit == 0) ? (0 << PD2) : (1 << PD2); // Select digit
    PORTA = seven_seg[number]; // Set segment values
}