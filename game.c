/*
 * game.c
 *
 * Authors: Jarrod Bennett, Cody Burnett, Bradley Stone, Yufeng Gao
 * Modified by: <YOUR NAME HERE>
 *
 * Game logic and state handler.
 */ 

#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "ledmatrix.h"
#include "terminalio.h"
#include <avr/io.h>


// ========================== NOTE ABOUT MODULARITY ==========================

// The functions and global variables defined with the static keyword can
// only be accessed by this source file. If you wish to access them in
// another C file, you can remove the static keyword, and define them with
// the extern keyword in the other C file (or a header file included by the
// other C file). While not assessed, it is suggested that you develop the
// project with modularity in mind. Exposing internal variables and functions
// to other .C files reduces modularity.


// ============================ GLOBAL VARIABLES =============================

// The game board, which is dynamically constructed by initialise_game() and
// updated throughout the game. The 0th element of this array represents the
// bottom row, and the 7th element of this array represents the top row.
static uint8_t board[MATRIX_NUM_ROWS][MATRIX_NUM_COLUMNS];

// The location of the player.
static uint8_t player_row;
static uint8_t player_col;

// A flag for keeping track of whether the player is currently visible.
static bool player_visible;
#define NULL_WALL_MESSAGES 3

// Seven segment display - segment values for digits 0 to 9
// uint8_t seven_seg[10] = { 63, 6, 91, 79, 102, 109, 125, 7, 127, 111 };

// Step count variable
volatile uint8_t step_count = 0;

void display_digit(uint8_t number, uint8_t digit) 
{
	PORTD = digit << 2; // because of location of pin
	PORTA = seven_seg[number];	// We assume digit is in range 0 to 9
}

void seven_segment(uint8_t fixed_number) {
	
    static uint8_t digit = 0; // Track which digit to display (0 = right, 1 = left)
	
    uint8_t value;
	
	// Extract the correct digit value to display
	if (digit == 0) {
		value = fixed_number % 10;  // Ones place
	} else {
		value = (fixed_number / 10) % 10;  // Tens place
	}
	

    // Display the digit on the seven-segment display
    display_digit(value, digit);

    // Alternate between right and left digit for next update
    digit = 1 - digit;

    // Wait for timer 1 to reach output compare A value (1 ms delay)
    while ((TIFR1 & (1 << OCF1A)) == 0) {
        ; // Do nothing - wait for the bit to be set
    }
    // Clear the output compare flag by writing a 1 to it
    TIFR1 &= (1 << OCF1A);
}


// ========================== GAME LOGIC FUNCTIONS ===========================

// This function paints a square based on the object(s) currently on it.
static void paint_square(uint8_t row, uint8_t col)
{
	switch (board[row][col] & OBJECT_MASK)
	{
		case ROOM:
			ledmatrix_update_pixel(row, col, COLOUR_BLACK);
			break;
		case WALL:
			ledmatrix_update_pixel(row, col, COLOUR_WALL);
			break;
		case BOX:
			ledmatrix_update_pixel(row, col, COLOUR_BOX);
			break;
		case TARGET:
			ledmatrix_update_pixel(row, col, COLOUR_TARGET);
			break;
		case BOX | TARGET:
			ledmatrix_update_pixel(row, col, COLOUR_DONE);
			break;
		default:
			break;
	}
}

void wall_message(){
	int message_num = rand() % NULL_WALL_MESSAGES;
	
	const char *messages[3] = {
		PSTR("YOU'VE HIT A WALL!"),
		PSTR("THE WALL IS AN ENEMY! BEWARE1"),
		PSTR("AVOID THE WALLS!")
	};
	
	printf_P(messages[message_num]); 
}

void update_moves(char move, char object, uint8_t new_player_x, uint8_t new_player_y){
	//

	//Modulus kinda works with right and up
	// just with not left and down

	//Moving to the right
	if (move == 'D' && object == 'R'){
		ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
		ledmatrix_update_pixel(new_player_y, new_player_x+1, COLOUR_BOX);
		board[new_player_y][new_player_x] = ROOM;
		board[(new_player_y)%MATRIX_NUM_ROWS][(new_player_x+1)% MATRIX_NUM_COLUMNS] = BOX;

	
	}
	else if (move == 'D' && object == 'T'){
		ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
		ledmatrix_update_pixel(new_player_y, new_player_x+1, COLOUR_DONE);
		board[new_player_y][new_player_x] = ROOM;
		//This is a bug, cant assign colour done here, but fine for now
		// it has to be an object for e.g. target_acquired
		//BUG!
		board[(new_player_y)%MATRIX_NUM_ROWS][(new_player_x+1) % MATRIX_NUM_COLUMNS] = COLOUR_DONE;
	}

	
	//Moving Up
	if (move == 'W' && object == 'R'){
		ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
		ledmatrix_update_pixel(new_player_y+1, new_player_x, COLOUR_BOX);
		board[new_player_y][new_player_x] = ROOM;
		board[(new_player_y+1)%MATRIX_NUM_ROWS][(new_player_x)% MATRIX_NUM_COLUMNS] = BOX;
	}
	else if (move == 'W' && object == 'T'){
		ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
		ledmatrix_update_pixel(new_player_y+1, new_player_x, COLOUR_DONE);
		board[new_player_y][new_player_x] = ROOM;
		board[new_player_y][new_player_x+1] = COLOUR_DONE;
	}

	//Moving Down
	if (move == 'S' && object == 'R'){
		ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
		ledmatrix_update_pixel(new_player_y-1, new_player_x, COLOUR_BOX);
		board[new_player_y][new_player_x] = ROOM;
		board[(new_player_y-1)%MATRIX_NUM_ROWS][(new_player_x)% MATRIX_NUM_COLUMNS] = BOX;
	}
	else if (move == 'S' && object == 'T'){
		ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
		ledmatrix_update_pixel(new_player_y-1, new_player_x, COLOUR_DONE);
		board[new_player_y][new_player_x] = ROOM;
		board[new_player_y-1][new_player_x] = COLOUR_DONE;
	}

	//Moving to the left
	if (move == 'A' && object == 'R'){
		ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
		ledmatrix_update_pixel(new_player_y, new_player_x-1, COLOUR_BOX);
		board[new_player_y][new_player_x] = ROOM;
		board[(new_player_y)%MATRIX_NUM_ROWS][(new_player_x-1)% MATRIX_NUM_COLUMNS] = BOX;
	}
	else if (move == 'A' && object == 'T'){
		ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
		ledmatrix_update_pixel(new_player_y, new_player_x-1, COLOUR_DONE);
		board[new_player_y][new_player_x] = ROOM;
		board[new_player_y][new_player_x-1] = COLOUR_DONE;
	}

}
// This function initialises the global variables used to store the game
// state, and renders the initial game display.
void initialise_game(void)
{
	// Remember that I can't use TIME ON AVR
	srand(time(NULL));
	
	// Short definitions of game objects used temporarily for constructing
	// an easier-to-visualise game layout.
	#define _	(ROOM)
	#define W	(WALL)
	#define T	(TARGET)
	#define B	(BOX)
	
	// The starting layout of level 1. In this array, the top row is the
	// 0th row, and the bottom row is the 7th row. This makes it visually
	// identical to how the pixels are oriented on the LED matrix, however
	// the LED matrix treats row 0 as the bottom row and row 7 as the top
	// row.
	static const uint8_t lv1_layout[MATRIX_NUM_ROWS][MATRIX_NUM_COLUMNS] =
	{
		{ _, W, _, W, W, W, _, W, W, W, _, _, W, W, W, W },
		{ _, W, T, W, _, _, W, T, _, B, _, _, _, _, T, W },
		{ _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _ },
		{ W, _, B, _, _, _, _, W, _, _, B, _, _, B, _, W },
		{ W, _, _, _, W, _, B, _, _, _, _, _, _, _, _, _ },
		{ _, _, _, _, _, _, T, _, _, _, _, _, _, _, _, _ },
		{ _, _, _, W, W, W, W, W, W, T, _, _, _, _, _, W },
		{ W, W, _, _, _, _, _, _, W, W, _, _, W, W, W, W }
	};

	// Undefine the short game object names defined above, so that you
	// cannot use use them in your own code. Use of single-letter names/
	// constants is never a good idea.
	#undef _
	#undef W
	#undef T
	#undef B

	// Set the initial player location (for level 1).
	player_row = 5;
	player_col = 2;

	// Make the player icon initially invisible.
	player_visible = false;

	// Copy the starting layout (level 1 map) to the board array, and flip
	// all the rows.
	for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++)
	{
		for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++)
		{
			board[MATRIX_NUM_ROWS - 1 - row][col] =
				lv1_layout[row][col];
		}
	}

	// Draw the game board (map).
	for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++)
	{
		for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++)
		{
			paint_square(row, col);
		}
	}
}

// This function flashes the player icon. If the icon is currently visible, it
// is set to not visible and removed from the display. If the player icon is
// currently not visible, it is set to visible and rendered on the display.
// The static global variable "player_visible" indicates whether the player
// icon is currently visible.
void flash_player(void)
{
	player_visible = !player_visible;
	if (player_visible)
	{
		// The player is visible, paint it with COLOUR_PLAYER.
		
		
		ledmatrix_update_pixel(player_row, player_col, COLOUR_PLAYER);
		
	}
	else
	{
		// The player is not visible, paint the underlying square.
		paint_square(player_row, player_col);
	}
}

// This function handles player movements.
bool move_player(int8_t delta_row, int8_t delta_col)
{
	DDRA = 0xFF;
	DDRD = (1 << 2);
	PORTA = 0x00;
	PORTD = 0x00;
	sei();
	
	//                    Implementation Suggestions
	//                    ==========================
	//
	//    Below are some suggestions for how to implement the first few
	//    features. These are only suggestions, you are absolutely not
	//   required to follow them if you know what you're doing, they're
	//     just here to help you get started. The suggestions for the
	//       earlier features are more detailed than the later ones.
	//
	// +-----------------------------------------------------------------+
	// |            Move Player with Push Buttons/Terminal               |
	// +-----------------------------------------------------------------+
	// | 1. Remove the display of the player icon from the current       |
	// |    location.     
	// |      - You may find the function flash_player() useful.  
	player_visible = true; 
	flash_player();   
	// | 2. Calculate the new location of the player.                    |
	// |      - You may find creating a function for this useful.        |

	/*Calculating new location and not allowing negative numbers*/
	/*Mapping the moves to the location thus using modulus*/
	/*Can't have moves outta bounds*/
	uint8_t new_player_x = (player_col + (uint8_t)delta_col) % MATRIX_NUM_COLUMNS;
	uint8_t new_player_y = (player_row + (uint8_t)delta_row) % MATRIX_NUM_ROWS;

	uint8_t new_object_x = (new_player_x + (uint8_t)delta_col) % MATRIX_NUM_COLUMNS;;
	uint8_t new_object_y = (new_player_y + (uint8_t)delta_row) % MATRIX_NUM_ROWS;

	uint8_t current_object = board[new_player_y][new_player_x] & OBJECT_MASK;
	uint8_t new_object_location = board[new_object_y][new_object_x]  & OBJECT_MASK;

	static uint8_t steps = 0;

	
	clear_terminal();

	// printf(PSTR());
		// +-----------------------------------------------------------------+
	//
	// +-----------------------------------------------------------------+
	// |                      Game Logic - Walls                         |
	// +-----------------------------------------------------------------+
	// | 1. Modify this function to return a flag/boolean for indicating |
	// |    move validity - you do not want to reset icon flash cycle on |
	// |    invalid moves.                                               |
	
	// | 2. Modify this function to check if there is a wall at the      |
	// |    target location.                                             |
	// | 3. If the target location contains a wall, print one of your 3  |
	// |    'hit wall' messages and return a value indicating an invalid |
	// |    move.                                                        |
	// | 4. Otherwise make the move, clear the message area of the       |
	// |    terminal and return a value indicating a valid move.         |
	if (current_object == WALL){
		wall_message();
		return false;
	}
	else if (current_object == BOX || current_object == (BOX | TARGET)){
		// Everything else
		// Check if the box can be moved
		if (current_object == (BOX | TARGET)){
			if (new_object_location == ROOM) {
			// Move the box
				board[new_object_y][new_object_x] = BOX;
				board[new_player_y][new_player_x] = TARGET;

				paint_square(new_object_y, new_object_x);  // Paint new box position
				paint_square(new_player_y, new_player_x); 


				//FIGURE THIS
				//If there was a message displayed in the message area of the terminal, it must be cleared.
				
				clear_terminal();
				printf(PSTR("BOX MOVED FROM TARGET.\n"));
				
			}
			else if (new_object_location == WALL || new_object_location == BOX || new_object_location == (BOX | TARGET)){
				switch (new_object_location)
				{
				case WALL:
					printf_P(PSTR("There's a wall there mate!"));
					return false;
					break;
				case BOX:
					printf_P(PSTR("A box cannot be stacked on top of another box."));
					return false;
					break;
				case (BOX | TARGET):
					printf_P(PSTR("Target already placed"));
					return false;
					break;
				}
			}
		}
		else if (new_object_location == ROOM) {
			// Move the box
			board[new_object_y][new_object_x] = BOX;
			board[new_player_y][new_player_x] = ROOM;

			paint_square(new_object_y, new_object_x);  // Paint new box position
            paint_square(new_player_y, new_player_x);   

			printf("Box moved successfully.\n");
		}
		else if (new_object_location == WALL || new_object_location == BOX || new_object_location == (BOX | TARGET)){
			switch (new_object_location)
			{
			case WALL:
				printf_P(PSTR("There's a wall there mate!"));
				return false;
				break;
			case BOX:
				printf_P(PSTR("A box cannot be stacked on top of another box."));
				return false;
				break;
			case (BOX | TARGET):
					printf_P(PSTR("Target already placed"));
					return false;
					break;
			}
			
		}
		
		else if (new_object_location == TARGET){
			printf_P(PSTR("You've put the box in the target"));
			board[new_object_y][new_object_x] = (BOX | TARGET);
			board[new_player_y][new_player_x] = ROOM;
			paint_square(new_object_y, new_object_x);  // Paint new box position
			paint_square(new_player_y, new_player_x);   
		}
	
	}

	// | 3. Update the player location (player_row and player_col).      |
	player_col = new_player_x;
	player_row = new_player_y;
	printf_P(PSTR("You've made a valid move!\n"));
	steps = (steps + 1) % 100; // max steps is 99 on the Seven-segment display
	printf_P(PSTR("STEPS: %d"), steps);
	number_to_display = (number_to_display + 1) % 100; 
	// seven_segment(steps);
	// | 4. Draw the player icon at the new player location.             |
	// |      - Once again, you may find the function flash_player()     |
	// |        useful.
	
	                                           
	// | 5. Reset the icon flash cycle in the caller function (i.e.,     |
	// |    play_game())                                                 |

	flash_player();      
	return true;
	
	 
	// +-----------------------------------------------------------------+
	//
	// +-----------------------------------------------------------------+
	// |                      Game Logic - Boxes                         |
	// +-----------------------------------------------------------------+
	// | 1. Modify this function to check if there is a box at the       |
	// |    target location.                                             |
	// | 2. If the target location contains a box, see if it can be      |
	// |    pushed. If not, print a message and return a value           |
	// |    indicating an invalid move.                                  |
	// | 3. Otherwise push the box and move the player, then clear the   |
	// |    message area of the terminal and return a valid indicating a |
	// |    valid move.                                                  |
	// +-----------------------------------------------------------------+

	// <YOUR CODE HERE>
}

// This function checks if the game is over (i.e., the level is solved), and
// returns true iff (if and only if) the game is over.
bool is_game_over(void)
{
	// <YOUR CODE HERE>.
	return false;
}
