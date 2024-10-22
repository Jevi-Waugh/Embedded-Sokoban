/*
 * timer1.c
 *
 * Author: Peter Sutton
 */

#include "timer1.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "game.h"
uint8_t music_duration = 0;
bool game_muted;
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

	// Setup timer 1.
	TCNT1 = 0;
	set_up_music(200,2);
	// // /* Enable interrupt on timer on output compare match 
	TIMSK1 = (1<<OCIE1A);

	// // /* Ensure interrupt flag is cleared */
	TIFR1 = (1<<OCF1A);

	// Make pin OC1B be an output (port D, pin 4)
	DDRD = (1<<4);
	

	// /* Set up timer/counter  so that it reaches an output compare
	// ** match every 1 millisecond (1000 times per second) and then
	// ** resets to 0.
	// ** We divide the clock by 8 and count 1000 cycles (0 to 999)
	OCR1A = 999;
	// 8,000,000/8 = number * 0.001 = 1000 - 1 = 999

	// Set up timer/counter 1 for Fast PWM, counting from 0 to the value in OCR1A
	// before reseting to 0. Count at 1MHz (CLK/8).
	// Configure output OC1B to be clear on compare match and set on timer/counter
	// overflow (non-inverting mode).
	TCCR1A = (1 << COM1B1) | (0 <<COM1B0) | (1 <<WGM11) | (1 << WGM10);
	TCCR1B = (1 << WGM13) | (1 << WGM12); // dont start the buzzer just yet otherwise as soon as power is plugged into the AVR it will play.
	// Sounds must be tones (not clicks) in the range 20Hz to 5kHz.
	 
	
}

void set_up_music(uint16_t freq, float dutycycle){
	// freq	-> Hz
	// dutycycle -> %
	uint16_t clockperiod = freq_to_clock_period(freq);
	uint16_t pulsewidth = duty_cycle_to_pulse_width(dutycycle, clockperiod);

	// Set the maximum count value for timer/counter 1 to be one less than the clockperiod
	OCR1A = clockperiod - 1;
	
	// Set the count compare value based on the pulse width. The value will be 1 less
	// than the pulse width - unless the pulse width is 0.
	if(pulsewidth == 0) {
		OCR1B = 0;
	} else {
		OCR1B = pulsewidth - 1;
	}
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

void generate_music(int type_of_music){
	
	music_duration = 100; // in ms
	// 1. Player being moved
	// 2. Invalid move
	// 3. Box pushed
	
	if (type_of_music == 1){
		// Player being moved and enter param
		set_up_music(400, 10);
	}
	else if(type_of_music == 2){
		// BOX_ON_TARGET
		set_up_music(400, 10);
	}
	else if(type_of_music == 3){
		// Box pushed
		set_up_music(200, 350);
		set_up_music(400, 20);
		set_up_music(300, 3);
		set_up_music(500, 5);
	}
	// start tone here so that way its easier for music debugging
	if (game_muted == false){
		//If the Game Pause feature is implemented, no sound should play while the game is paused, however
		//playing sounds must resume where they were left off when unpaused.
		//havent done that
		start_tone();
	}
	
	
}

ISR(TIMER1_COMPA_vect){
	// printf(("inTERRUPT......"));
	// Timer/counter 1 compare match A interrupt service routine
	// I think compare match happens every millisecond
	if (music_duration > 0){
		// if set then it means that the musc has to play
		
		music_duration--;
	}
	else{
		// if not then the music has to stop
		stop_tone();
		// TIMSK1 &= ~(1 << OCIE1A); // Disable interrupt
	}
}

