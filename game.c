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
#include <avr/io.h>
#include "ledmatrix.h"
#include "terminalio.h"
#include "timer1.h"
#include "timer2.h"


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
volatile uint8_t steps_glob;
int num_targets = 0;
volatile int level = 1;

volatile uint16_t freq;	// Hz
volatile float dutycycle;	// %



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
		PSTR("THE WALL IS AN ENEMY! BEWARE"),
		PSTR("AVOID THE WALLS!")
	};

	move_terminal_cursor(6,4);
	clear_to_end_of_line();
	printf_P(messages[message_num]); 
}

// This function initialises the global variables used to store the game
// state, and renders the initial game display.
void initialise_game(int level)
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

	// worry about optimising setting boards later
	static const uint8_t lv2_layout[MATRIX_NUM_ROWS][MATRIX_NUM_COLUMNS] =
	{
		{ _, _, W, W, W, W, _, _, W, W, _, _, _, _, _, W },
		{ _, _, W, _, _, W, _, W, W, _, _, _, _, _, B, _ },
		{ _, _, W, _, B, W, W, W, _, _, T, W, _, T, W, W },
		{ _, _, W, _, _, _, _, T, _, _, B, W, W, W, _, _ },
		{ W, W, W, W, _, W, _, _, _, _, _, W, _, W, W, _ },
		{ W, T, B, _, _, _, _, B, _, _, _, W, W, _, W, W },
		{ W, _, _, _, T, _, _, _, _, _, _, B, T, _, _, _ },
		{ W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W }
		
	};
	
	// Undefine the short game object names defined above, so that you
	// cannot use use them in your own code. Use of single-letter names/
	// constants is never a good idea.
	#undef _
	#undef W
	#undef T
	#undef B

	// Set the initial player location (for level 1).
	if (level == 1){
		player_row = 5;
		player_col = 2;
	}
	else if (level == 2){
		player_row = MATRIX_NUM_ROWS-2;
		player_col = MATRIX_NUM_COLUMNS-1;
	}
	

	// Make the player icon initially invisible.
	player_visible = false;

	// Copy the starting layout (level 1 map) to the board array, and flip
	// all the rows.
	for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++)
	{
		for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++)
		{
			if (level == 1){
				board[MATRIX_NUM_ROWS - 1 - row][col] =
				lv1_layout[row][col];
			}
			else if (level == 2){
				board[MATRIX_NUM_ROWS - 1 - row][col] =
				lv2_layout[row][col];
			}
			
				
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
	num_targets = 0;
	steps_glob = 0;
	for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++)
	{
		for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++)
		{
			if (board[row][col] & TARGET){
				// optimise this with another loop instead
				num_targets++;
			}
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


void display_terminal_gameplay(){
	// board
	int i,j;
	for(i=0; i< MATRIX_NUM_ROWS; i++){
		for(j=0; j< MATRIX_NUM_COLUMNS; j++){

			switch (board[i][j])
			{
			case ROOM:
				move_terminal_cursor(8 + (MATRIX_NUM_ROWS - i),j+7);
				set_display_attribute(BG_BLACK);
				printf_P(PSTR("  "));
				break;
			case WALL:
				move_terminal_cursor(8 + (MATRIX_NUM_ROWS - i),j+7);
				set_display_attribute(BG_YELLOW);
				printf_P(PSTR("  "));
				break;
			case BOX:
				move_terminal_cursor(8 + (MATRIX_NUM_ROWS - i),j+7);
				set_display_attribute(BG_MAGENTA);
				printf_P(PSTR("  "));
				break;
			case TARGET:
				move_terminal_cursor(8 + (MATRIX_NUM_ROWS - i),j+7);
				set_display_attribute(BG_RED);
				printf_P(PSTR("  "));
				break;
			default:
				break;
			
			}	
			set_display_attribute(BG_BLACK);
		}
	}
	set_display_attribute(BG_BLACK);
}
void reset_cursor_position(){
	move_terminal_cursor(0,0);
}
//flashes player on terminal
void flash_terminal_player(uint8_t player_x, uint8_t player_y, uint8_t old_player_x, uint8_t old_player_y){
	
	// reset_cursor_position();
	move_terminal_cursor(8 + (MATRIX_NUM_ROWS - player_y), player_x+7);
	show_cursor();
	set_display_attribute(BG_CYAN);
	printf_P(PSTR("  "));
	// be careful of 1 space and 2 spaces
	// set_display_attribute(BG_BLACK);
	// reset_cursor_position();
	// move_terminal_cursor(8 + (MATRIX_NUM_ROWS - old_player_y) , old_player_x+7);
	// printf_P(PSTR("  "));
	
	set_display_attribute(BG_BLACK);
	move_terminal_cursor(8 + (MATRIX_NUM_ROWS - old_player_y), old_player_x+7);

}

void update_terminal_moves(uint8_t object, uint8_t row, uint8_t col){
	switch (object)
	{
	case ROOM /* constant-expression */:
		move_terminal_cursor(8 + (MATRIX_NUM_ROWS - row),col+7);
		set_display_attribute(BG_BLACK);
		printf_P(PSTR(" "));
		break;
	case WALL /* constant-expression */:
		move_terminal_cursor(8 + (MATRIX_NUM_ROWS - row),col+7);
		set_display_attribute(BG_YELLOW);
		printf_P(PSTR(" "));
		break;
	case BOX /* constant-expression */:
		move_terminal_cursor(8 + (MATRIX_NUM_ROWS - row),col+7);
		set_display_attribute(BG_MAGENTA);
		printf_P(PSTR(" "));
		break;
	case TARGET /* constant-expression */:
		move_terminal_cursor(8 + (MATRIX_NUM_ROWS - row),col+7);
		set_display_attribute(BG_RED);
		printf_P(PSTR(" "));
		break;
	case (BOX | TARGET) /* constant-expression */:
		move_terminal_cursor(8 + (MATRIX_NUM_ROWS - row),col+7);
		set_display_attribute(BG_GREEN);
		printf_P(PSTR(" "));
		break;
	
	default:
		break;
	}
	set_display_attribute(BG_BLACK);
	
}
// This function handles player movements.
bool move_player(int8_t delta_row, int8_t delta_col)
{
	//display_terminal_gameplay();
	
	
	sei();
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

	uint8_t old_p_x;
	uint8_t old_p_y;
	static uint8_t steps = 0;


	
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
				update_terminal_moves(board[new_player_y][new_player_x], new_player_y, new_player_x);
				update_terminal_moves(board[new_object_y][new_object_x], new_object_y, new_object_x);

				// clear_terminal();
				set_display_attribute(BG_BLACK);
				move_terminal_cursor(6,4);
				clear_to_end_of_line();
				printf(PSTR("BOX MOVED FROM TARGET.\n"));
				
			}
			else if (new_object_location == WALL || new_object_location == BOX || new_object_location == (BOX | TARGET)){
				switch (new_object_location)
				{
				case WALL:
					move_terminal_cursor(6,4);
					clear_to_end_of_line();
					printf_P(PSTR("There's a wall there mate!"));
					return false;
					break;
				case BOX:
					move_terminal_cursor(6,4);
					clear_to_end_of_line();
					printf_P(PSTR("A box cannot be stacked on top of another box."));
					return false;
					break;
				case (BOX | TARGET):
					move_terminal_cursor(6,4);
					clear_to_end_of_line();
					printf_P(PSTR("Target already placed"));
					return false;
					break;
				}
			}
		}
		else if (new_object_location == ROOM) {
			// Move the box
			board[new_object_y][new_object_x] = BOX; // NEW LOCATION
			board[new_player_y][new_player_x] = ROOM; //PREVIOUS LOCATION

			paint_square(new_object_y, new_object_x);  // Paint new box position
            paint_square(new_player_y, new_player_x);   
			update_terminal_moves(board[new_object_y][new_object_x], new_object_y, new_object_x);
			update_terminal_moves(board[new_player_y][new_player_x], new_player_y, new_player_x);
			move_terminal_cursor(6,4);
			clear_to_end_of_line();
			printf("Box moved successfully.\n");
		}
		else if (new_object_location == WALL || new_object_location == BOX || new_object_location == (BOX | TARGET)){
			switch (new_object_location)
			{
			case WALL:
				move_terminal_cursor(6,4);
				clear_to_end_of_line();
				printf_P(PSTR("There's a wall there mate!"));
				return false;
				break;
			case BOX:
				move_terminal_cursor(6,4);
				clear_to_end_of_line();
				printf_P(PSTR("A box cannot be stacked on top of another box."));
				return false;
				break;
			case (BOX | TARGET):
				move_terminal_cursor(6,4);
				clear_to_end_of_line();
				printf_P(PSTR("Target already placed"));
				return false;
				break;
			}
			
		}
		
		else if (new_object_location == TARGET){
			move_terminal_cursor(6,4);
			clear_to_end_of_line();
			printf_P(PSTR("You've put the box in the target"));
			
			board[new_object_y][new_object_x] = (BOX | TARGET);
			board[new_player_y][new_player_x] = ROOM;
			paint_square(new_object_y, new_object_x);  // Paint new box position
			paint_square(new_player_y, new_player_x);   
			update_terminal_moves(board[new_object_y][new_object_x], new_object_y, new_object_x);
			update_terminal_moves(board[new_player_y][new_player_x], new_player_y, new_player_x);
			// set_display_attribute(BG_BLACK);
		}
	
	}

	// | 3. Update the player location (player_row and player_col).      |
	// for flashing player
	old_p_x = player_col;
	old_p_y = player_row;
	start_tone();
	//stop_tone();
	// sound works just figure how to make it last 200 ms or smth

	player_col = new_player_x;
	player_row = new_player_y;
	// flash terminal player here
	// flash_terminal_player(player_col, player_row, old_p_x, old_p_y);
	move_terminal_cursor(6,4);
	
	
	if (new_object_location != TARGET){
		clear_to_end_of_line();
		printf_P(PSTR("You've made a valid move!\n"));
	}
	
	steps_glob++; //unbounded steps
	// steps_glob = steps;
	
	
	move_terminal_cursor(3,4);
	printf_P(PSTR("Level: %d"), level);
	move_terminal_cursor(7,5);
	printf_P(PSTR("STEPS: %d"), steps_glob);
	// step should keep incrementing 
	number_to_display = (number_to_display + 1) % 100; // max steps is 99 on the Seven-segment display
	// seven_segment(steps);
	// | 4. Draw the player icon at the new player location.             |
	// |      - Once again, you may find the function flash_player()     |
	// |        useful.
	
	                                           
	// | 5. Reset the icon flash cycle in the caller function (i.e.,     |
	// |    play_game())                                                 |

	flash_player();     
	// set_display_attribute(TERM_RESET);
	return true;

}

// This function checks if the game is over (i.e., the level is solved), and
// returns true iff (if and only if) the game is over.
bool is_game_over(void)
{
	// <YOUR CODE HERE>.
	// if the number of objects that are targets equal the number of (box | target) 
	//then level is finished.
	int i,j;
	int count_targets = 0;
	for(i = 0;  i < MATRIX_NUM_ROWS; i++){
		for(j = 0;  j < MATRIX_NUM_COLUMNS; j++){
			if ((board[i][j] & TARGET) && (board[i][j] & BOX)) {
				count_targets++;
			}
		}
	}

	if (count_targets == num_targets){
		// Level is finished and return true
		return true;
	}

	
	return false;
}
