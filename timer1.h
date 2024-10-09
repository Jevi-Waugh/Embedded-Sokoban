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
// extern volatile uint8_t number_to_display; // Default to 42 for testing
/// </summary>
void init_timer1(void);
void display_digit(uint8_t number, uint8_t digit);

#endif /* TIMER1_H_ */



