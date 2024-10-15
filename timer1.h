/*
 * timer1.h
 *
 * Author: Peter Sutton
 *
 * Timer 1 skeleton.
 */

#ifndef TIMER1_H_
#define TIMER1_H_
#include <stdint.h>
/// <summary>
/// Skeletal timer 1 initialisation function.

// The number currently displayed
uint16_t freq_to_clock_period(uint16_t freq);
uint16_t duty_cycle_to_pulse_width(float dutycycle, uint16_t clockperiod);
/// </summary>
void init_timer1(void);
void start_tone();
void stop_tone();
void generate_music(int type_of_music);
void set_up_music(uint16_t freq, float dutycycle);
// void display_digit(uint8_t number, uint8_t digit);

#endif /* TIMER1_H_ */



