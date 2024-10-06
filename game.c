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
bool move_player(int8_t delta_row, int8_t delta_col, char move)
{
	
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
	uint8_t current_object = board[new_player_y][new_player_x] & OBJECT_MASK;

	uint8_t next_object_right = board[new_player_y][new_player_x+1] & OBJECT_MASK;
	uint8_t next_object_left = board[new_player_y][new_player_x-1] & OBJECT_MASK;
	uint8_t next_object_up = board[new_player_y+1][new_player_x] & OBJECT_MASK;
	uint8_t next_object_down = board[new_player_y-1][new_player_x] & OBJECT_MASK;
	
	clear_terminal();
	
	if (current_object == WALL){
		wall_message();
		return false;
	}
	else if (current_object == BOX){
		// TARGETS AREN'T WORKING ATM
		if (toupper(move) == 'D'){
			if (next_object_right == ROOM){
				printf_P(PSTR("YOU CAN GO and move the box to the right"));
				ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
				ledmatrix_update_pixel(new_player_y, new_player_x+1, COLOUR_BOX);
				//do the rest once this works
			}
			else if (next_object_right == BOX){
				printf_P(PSTR("A box cannot be stacked on top of another box."));
				return false;
			}

			else if (next_object_right == WALL){
				printf_P(PSTR("A box cannot be pushed onto the wall."));
				return false;
			}
			else if (next_object_right == TARGET){
				printf_P(PSTR("You've put the box in the target"));
				ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
				ledmatrix_update_pixel(new_player_y, new_player_x+1, COLOUR_TARGET);
			}
		}

		else if (toupper(move) == 'W'){
			if (next_object_up == ROOM){
				printf_P(PSTR("YOU CAN GO and move the box up"));
				ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
				ledmatrix_update_pixel(new_player_y+1, new_player_x, COLOUR_BOX);
				//do the rest once this works
			}
			else if (next_object_up == BOX){
				printf_P(PSTR("A box cannot be stacked on top of another box."));
				return false;
			}

			else if (next_object_up == WALL){
				printf_P(PSTR("A box cannot be pushed onto the wall."));
				return false;
			}
			else if (next_object_up == TARGET){
				printf_P(PSTR("You've put the box in the target"));
				ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
				ledmatrix_update_pixel(new_player_y+1, new_player_x, COLOUR_TARGET);
			}
		}

		else if (toupper(move) == 'S'){
			if (next_object_down == ROOM){
				printf_P(PSTR("YOU CAN GO and move the box down"));
				ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
				ledmatrix_update_pixel(new_player_y-1, new_player_x, COLOUR_BOX);
				//do the rest once this works
			}
			else if (next_object_down == BOX){
				printf_P(PSTR("A box cannot be stacked on top of another box."));
				return false;
			}

			else if (next_object_down== WALL){
				printf_P(PSTR("A box cannot be pushed onto the wall."));
				return false;
			}
			else if (next_object_down == TARGET){
				printf_P(PSTR("You've put the box in the target down"));
				ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
				ledmatrix_update_pixel(new_player_y-1, new_player_x, COLOUR_TARGET);
			}
		}

		else if (toupper(move) == 'A'){
			if (next_object_left == ROOM){
				printf_P(PSTR("YOU CAN GO and move the box to the left"));
				ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
				ledmatrix_update_pixel(new_player_y, new_player_x-1, COLOUR_BOX);
				//do the rest once this works
			}
			else if (next_object_left == BOX){
				printf_P(PSTR("A box cannot be stacked on top of another box."));
				return false;
			}
			else if (next_object_left == WALL){
				printf_P(PSTR("A box cannot be pushed onto the wall."));
				return false;
			}
			else if (next_object_left == TARGET){
				printf_P(PSTR("You've put the box in the target"));
				ledmatrix_update_pixel(new_player_y, new_player_x, COLOUR_BLACK);
				ledmatrix_update_pixel(new_player_y, new_player_x-1, COLOUR_TARGET);
			}
		}
		
	
	}

	// | 3. Update the player location (player_row and player_col).      |
	player_col = new_player_x;
	player_row = new_player_y;
	// | 4. Draw the player icon at the new player location.             |
	// |      - Once again, you may find the function flash_player()     |
	// |        useful.
	
	                                           
	// | 5. Reset the icon flash cycle in the caller function (i.e.,     |
	// |    play_game())                                                 |
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
