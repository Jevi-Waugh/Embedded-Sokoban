/*
 * timer2.h
 *
 * Author: Peter Sutton
 *
 * Timer 2 skeleton
 */

#ifndef TIMER2_H_
#define TIMER2_H_
#include <stdint.h>

extern volatile uint8_t number_to_display; // Default to 42 for testing
extern volatile uint16_t joy_x;
extern volatile uint16_t joy_y;
/// <summary>
/// Skeletal timer 2 initialisation function.
/// </summary>
void init_timer2(void);

#endif /* TIMER2_H_ */
