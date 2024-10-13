/*
 * project.c
 *
 * Authors: Peter Sutton, Luke Kamols, Jarrod Bennett, Cody Burnett,
 *          Bradley Stone, Yufeng Gao
 * Modified by: <YOUR NAME HERE>
 *
 * Main project event loop and entry point.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define F_CPU 8000000UL
#include <util/delay.h>

#include "game.h"
#include "startscrn.h"
#include "ledmatrix.h"
#include "buttons.h"
#include "serialio.h"
#include "terminalio.h"
#include "timer0.h"
#include "timer1.h"
#include "timer2.h"

#define MILLISECONDS 1000
int32_t level_time = 0;
int level = 1;
// Function prototypes - these are defined below (after main()) in the order
// given here.
void initialise_hardware(void);
void start_screen(void);
void new_game(void);
void play_game(void);
void handle_game_over(void);

/////////////////////////////// main //////////////////////////////////
int main(void)
{
	// Setup hardware and callbacks. This will turn on interrupts.
	initialise_hardware();

	// Show the start screen. Returns when the player starts the game.
	start_screen();

	// Loop forever and continuously play the game.
	while (1)
	{
		new_game();
		play_game();
		handle_game_over();
	}
}

void initialise_hardware(void)
{
	init_ledmatrix();
	init_buttons();
	init_serial_stdio(19200, false);
	init_timer0();
	init_timer1();
	init_timer2();

	// Turn on global interrupts.
	sei();
}

void start_screen(void)
{
	// Hide terminal cursor and set display mode to default.
	// reset ssd to 0. its done again in new game when the player restarts the game as well
	number_to_display = 0;
	hide_cursor();
	normal_display_mode();

	// Clear terminal screen and output the title ASCII art.
	clear_terminal();
	display_terminal_title(3, 5);
	move_terminal_cursor(11, 5);
	// Change this to your name and student number. Remember to remove the
	// chevrons - "<" and ">"!
	printf_P(PSTR("CSSE2010/7201 Project by Jevi Waugh - 48829678"));
	// Setup the start screen on the LED matrix.
	setup_start_screen();

	// Clear button presses registered as the result of powering on the
	// I/O board. This is just to work around a minor limitation of the
	// hardware, and is only done here to ensure that the start screen is
	// not skipped when you power cycle the I/O board.
	clear_button_presses();

	// Wait until a button is pushed, or 's'/'S' is entered.
	while (1)
	{
		// Check for button presses. If any button is pressed, exit
		// the start screen by breaking out of this infinite loop.
		if (button_pushed() != NO_BUTTON_PUSHED)
		{
			break;
		}

		// No button was pressed, check if we have terminal inputs.
		if (serial_input_available())
		{
			// Terminal input is available, get the character.
			int serial_input = fgetc(stdin);

			// If the input is 's'/'S', exit the start screen by
			// breaking out of this loop.
			if (serial_input == 's' || serial_input == 'S')
			{
				break;
			}
			else if ((serial_input) == '2'){
				level = 2;
				break;
			}

			
		}

		// No button presses and no 's'/'S' typed into the terminal,
		// we will loop back and do the checks again. We also update
		// the start screen animation on the LED matrix here.
		update_start_screen();
	}
}

void new_game()
{
	// reset ssd to 0
	number_to_display = 0;

	// Clear the serial terminal.
	hide_cursor();
	clear_terminal();

	// Initialise the game and display.
	initialise_game(level);

	// Clear all button presses and serial inputs, so that potentially
	// buffered inputs aren't going to make it to the new game.
	clear_button_presses();
	clear_serial_input_buffer();
}

void play_game(void)
{

	uint32_t last_flash_time = get_current_time();
	uint32_t last_print_time = 0;
	uint32_t start_time = get_current_time();  // Only record start time now
	bool game_paused = false;
    

	display_terminal_gameplay();
	steps_glob = 0;
	
	// move_terminal_cursor(4,4);
    // printf_P(PSTR("Level: %d "), level);
	
	// We play the game until it's over.
	while (!is_game_over())
	{
		// We need to check if any buttons have been pushed, this will
		// be NO_BUTTON_PUSHED if no button has been pushed. If button
		// 0 has been pushed, we get BUTTON0_PUSHED, and likewise, if
		// button 1 has been pushed, we get BUTTON1_PUSHED, and so on.
		uint32_t curr_time = get_current_time();
		level_time = (curr_time - start_time) / MILLISECONDS;
		
		// oops was doing the opposite
		// level_time = ((last_flash_time / MILLISECONDS) % 60) + 1;
		// // adding a one because we cant have o seconds displayed, but not sure if i should or not.
		
		if (level_time != last_print_time) {
			move_terminal_cursor(5,4);
            printf_P(PSTR("Time elapsed: %d "), level_time);
			
            last_print_time = level_time;
        }
		// elapsed_time = (get_current_time() - last_flash_time); // 200ms
		
		ButtonState btn = button_pushed();
		int serial_input;
		if (serial_input_available())
		{
			// Terminal input is available, get the character.
			serial_input = fgetc(stdin);
		}
		else{
			serial_input = NULL;

		}
		if (btn == BUTTON0_PUSHED || toupper(serial_input) == 'D')
		{
			// Move the player, see move_player(...) in game.c.
			// Also remember to reset the flash cycle here.
			// move_player(y, x)
			move_player(0, 1);
			flash_player();
			
		}

		/*USE SWITCH STATEMENT HERE*/
		else if (btn == BUTTON1_PUSHED || toupper(serial_input) == 'S'){
			/*move the player down*/
			move_player(-1, 0);
			flash_player();
		}
		else if (btn == BUTTON2_PUSHED || toupper(serial_input) == 'W'){
			/*move the player UP*/
			move_player(1,0);
			flash_player();
			}
		else if (btn == BUTTON3_PUSHED || toupper(serial_input) == 'A'){
			/*move the player LEFT*/
			move_player(0,-1);
			flash_player();
			}
		
		else if (toupper(serial_input) == 'P'){
			game_paused = true;
			// printf_P(PSTR("Time elapsed: %d "), level_time);
			reset_cursor_position();
			clear_to_end_of_line();
			printf_P(PSTR("GAME PAUSED!"));
			uint32_t game_pause_time = get_current_time();
			// uint32_t last_game_pause_time = get_current_time();
			// LAST FLASH TIME Thingi
			while(game_paused){
				
				// Game is currently paused
				// see if the user types anything else 
				if (serial_input_available())
				{
					// Terminal input is available, get the character.
					serial_input = fgetc(stdin);
				}
				else{
					serial_input = NULL;
					}
		
				if (toupper(serial_input) == 'P'){
					reset_cursor_position();
					clear_to_end_of_line();
					printf_P(PSTR("GAME RESUMED!"));
					start_time += get_current_time() - game_pause_time;
					last_flash_time += get_current_time() - game_pause_time;
					// LAST FLASH TIME Thingi
					game_paused = false;
				}
					
			}
		}

		uint32_t current_time = get_current_time();
		if (current_time >= last_flash_time + 200)
		{
			// 200ms (0.2 seconds) has passed since the last time
			// we flashed the player icon, flash it now.
			flash_player();

			// Update the most recent icon flash time.
			last_flash_time = current_time;
		}

		// if (delta steps and move
		// if (delta_steps > 0 and move_player is true)
		//      10 ms
			// update steps
			// update terminal stuff

	}
	// We get here if the game is over.
	// printf_P(PSTR("LEVEL COMPLETED"));
}

uint8_t min(uint8_t steps_score , int zero){
	// testCondition ? expression1 : expression 2;
	return (steps_score < zero) ? steps_score : zero;
}

uint8_t max(uint8_t time_score, int zero){
	return (time_score > zero) ? time_score : zero;
}
void handle_game_over(void)
{
	// Score = max(200 - S, 0) * 20 + max(1200 - T, 0)
	uint8_t score = max(200 - steps_glob, 0) * 20 + max(1200 - level_time, 0);

	move_terminal_cursor(17, 10);
	printf_P(PSTR("LEVEL %d COMPLETED"), level);
	move_terminal_cursor(18, 10);
	printf_P(PSTR("STEPS TAKEN: %d"), steps_glob);
	move_terminal_cursor(19, 10);
	// amount of time in seconds (rounded down to the nearest second)
	printf_P(PSTR("TIME TAKEN: %d"), level_time);
	move_terminal_cursor(20, 10);
	printf_P(PSTR("SCORE: %d"), score);

	move_terminal_cursor(21, 10);
	printf_P(PSTR("Press 'r'/'R' to restart, or 'e'/'E' to exit"));

	// Do nothing until a valid input is made.
	while (1)
	{
		// Get serial input. If no serial input is ready, serial_input
		// would be -1 (not a valid character).
		int serial_input = -1;
		if (serial_input_available())
		{
			serial_input = fgetc(stdin);
		}

		// Check serial input.
		switch (toupper(serial_input))
		{
		case 'R':
			// Restarting Game
			return;
			break;
		
		case 'E':
			// Game exited
			initialise_hardware();
			start_screen();
			return;
			break;
		case 'N':
			// New game
			level = 2;
			return;
			break;
		
		default:
			break;
		}
	
	}
}
