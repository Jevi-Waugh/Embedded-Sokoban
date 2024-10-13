/*
 * timer1.c
 *
 * Author: Peter Sutton
 */

#include "timer1.h"
#include <avr/io.h>
#include <avr/interrupt.h>

uint16_t freq_to_clock_period(uint16_t freq) {
	return (1000000UL / freq);	// UL makes the constant an unsigned long (32 bits)
								// and ensures we do 32 bit arithmetic, not 16
}

// Return the width of a pulse (in clock cycles) given a duty cycle (%) and
// the period of the clock (measured in clock cycles)
uint16_t duty_cycle_to_pulse_width(float dutycycle, uint16_t clockperiod) {
	return (dutycycle * clockperiod) / 100;
}


void init_timer1(void)
{
	uint16_t freq = 500;	// Hz
	float dutycycle = 50;	// %
	
	uint16_t clockperiod = freq_to_clock_period(freq);
	uint16_t pulsewidth = duty_cycle_to_pulse_width(dutycycle, clockperiod);
	// Setup timer 1.
	TCNT1 = 0;
	// /* Set up timer/counter  so that it reaches an output compare
	// ** match every 1 millisecond (1000 times per second) and then
	// ** resets to 0.
	// ** We divide the clock by 8 and count 1000 cycles (0 to 999)
	// */
	// OCR1A = 999;
	// TCCR1A = (0 << COM1A1) | (1 << COM1A0)  // Toggle OC1A on compare match
	// 	| (0 << WGM11) | (0 << WGM10); // Least two significant WGM bits
	// TCCR1B = (0 << WGM13) | (1 << WGM12) // Two most significant WGM bits
	// 	| (0 << CS12) | (1 << CS11) | (0 <<CS10); // Divide clock by 8

	// // /* Enable interrupt on timer on output compare match 
	// // */
	// // TIMSK1 = (1<<OCIE1A);

	// // /* Ensure interrupt flag is cleared */
	// // TIFR1 = (1<<OCF1A);
	// // DDRA = 0xFF;
	// // 1 output, 0 input make connection to cc and make OC1B PD4 the end of the buzzer
	// DDRD = (1 << 2) | (1 << 4);

	// Make pin OC1B be an output (port D, pin 4)
	DDRD = (1<<4);
	
	// Set the maximum count value for timer/counter 1 to be one less than the clockperiod
	OCR1A = clockperiod - 1;
	
	// Set the count compare value based on the pulse width. The value will be 1 less
	// than the pulse width - unless the pulse width is 0.
	if(pulsewidth == 0) {
		OCR1B = 0;
	} else {
		OCR1B = pulsewidth - 1;
	}
	
	// Set up timer/counter 1 for Fast PWM, counting from 0 to the value in OCR1A
	// before reseting to 0. Count at 1MHz (CLK/8).
	// Configure output OC1B to be clear on compare match and set on timer/counter
	// overflow (non-inverting mode).
	TCCR1A = (1 << COM1B1) | (0 <<COM1B0) | (1 <<WGM11) | (1 << WGM10);
	TCCR1B = (1 << WGM13) | (1 << WGM12);
}

void start_tone(){
	TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);
}

void stop_tone(){
	// it makes those bits not, so the inverse and then
	//  bit mask AND to still have WGM bits but no CS bits for no clock source
	// so that it turns off.
	TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10)); // ~(0b00000111) => 0b11111000
}

