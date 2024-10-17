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
volatile uint16_t steps_glob;
int num_targets = 0;
volatile int level = 1;

volatile uint16_t freq;	// Hz
volatile float dutycycle;	// %

uint32_t last_target_area_flash_time;
uint8_t new_object_location;
uint8_t new_object_x = 0;
uint8_t new_object_y = 0;

bool target_met = false;

// uint8_t move_made[];
uint16_t undo_list[6][2];
int undo_capacity = 0;
uint8_t old_player_moves[2];



// A flag for keeping track of whether the player is currently visible.
static bool player_visible;
static bool target_visible;
bool box_pushed_on_target;
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
void reset_animation_display(uint8_t y,  uint8_t x){
	int i;
	reset_cursor_position();
	clear_to_end_of_line();
	printf_P(PSTR(" HEY THERE "));
	uint16_t target_area[9][2] = { {y+1,x-1}, {y+1,x}, {y+1,x+1},
						   {y,x-1}, {y,x}, {y,x+1},
						   {y-1,x-1}, {y-1,x}, {y-1,x+1}
						};
	reset_cursor_position();
	// printf_P(PSTR("RESETING ANIMATION DISPLAY %d %d"), x, y);
	for (i=0;i< 9;i++){
		// printf_P(PSTR("painting %d %d"),target_area[i][0], target_area[i][1]);
		paint_square(target_area[i][0], target_area[i][1]);
		
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
	srand(get_current_time());
	
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
	target_visible = false;

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

void flash_target_square(){
	
	target_visible = !target_visible;
	int i,j;
	for (i=0; i< MATRIX_NUM_ROWS; i++){
		for (j=0; j< MATRIX_NUM_COLUMNS; j++){
			//DONT FORGET TO include BOX AS WELL
			if (i == player_row && j == player_col)
			{
				continue;
			}
			if (board[i][j] == TARGET){
				if(target_visible){
					ledmatrix_update_pixel(i, j, COLOUR_TARGET);
				}
				else{
					ledmatrix_update_pixel(i, j, COLOUR_BLACK);
				}
			}
		}

	}
}
// not done
void get_location_matrix(uint8_t y, uint8_t x){
	//get the location of the player and the target from the matrix
	//board[new_object_y][new_object_x] = (BOX | TARGET);
	int num_area_squares = 9;
	int i;
	uint16_t target_area[9][2] = { {y+1,x-1}, {y+1,x}, {y+1,x+1},
						   {y,x-1}, {y,x}, {y,x+1},
						   {y-1,x-1}, {y-1,x}, {y-1,x+1}
						};

	for (i=0;i< num_area_squares;i++){
		ledmatrix_update_pixel(target_area[i][0], target_area[i][1], COLOUR_LIGHT_ORANGE);
		// paint_square(target_area[i][0], target_area[i][1]);
		
	}

}

void get_location_matrix2(uint8_t y, uint8_t x){
	//get the location of the player and the target from the matrix
	//board[new_object_y][new_object_x] = (BOX | TARGET);
	int num_area_squares = 9;
	int i;
	uint16_t target_area[9][2] = { {y+1,x-1}, {y+1,x}, {y+1,x+1},
						   {y,x-1}, {y,x}, {y,x+1},
						   {y-1,x-1}, {y-1,x}, {y-1,x+1}
						};

	for (i=0;i< num_area_squares;i++){
		paint_square(target_area[i][0], target_area[i][1]);
		
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
//not done
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
bool move_player(int8_t delta_row, int8_t delta_col, bool diagonal_move)
{
	//display_terminal_gameplay();
	
	
	sei();
	player_visible = true; 
	target_visible = true;
	
	flash_player();   
	// | 2. Calculate the new location of the player.                    |
	// |      - You may find creating a function for this useful.        |

	/*Calculating new location and not allowing negative numbers*/
	/*Mapping the moves to the location thus using modulus*/
	/*Can't have moves outta bounds*/
	uint8_t new_player_x = (player_col + (uint8_t)delta_col) % MATRIX_NUM_COLUMNS;
	uint8_t new_player_y = (player_row + (uint8_t)delta_row) % MATRIX_NUM_ROWS;

	new_object_x = (new_player_x + (uint8_t)delta_col) % MATRIX_NUM_COLUMNS;;
	new_object_y = (new_player_y + (uint8_t)delta_row) % MATRIX_NUM_ROWS;

	uint8_t current_object = board[new_player_y][new_player_x] & OBJECT_MASK;
	new_object_location = board[new_object_y][new_object_x]  & OBJECT_MASK;

	




	box_pushed_on_target = false;
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
				generate_music(PUSHING_BOX);
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
			generate_music(PUSHING_BOX);
			
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
			target_met = true;
			printf_P(PSTR("You've put the box in the target"));
			// get_location_matrix(new_object_y, new_object_x);
			board[new_object_y][new_object_x] = (BOX | TARGET);
			board[new_player_y][new_player_x] = ROOM;
			box_pushed_on_target = true;

			paint_square(new_object_y, new_object_x);  // Paint new box position
			paint_square(new_player_y, new_player_x);   
			generate_music(BOX_ON_TARGET);
			update_terminal_moves(board[new_object_y][new_object_x], new_object_y, new_object_x);
			update_terminal_moves(board[new_player_y][new_player_x], new_player_y, new_player_x);
			// set_display_attribute(BG_BLACK);
		}

		
	
	}
	else if (current_object == ROOM){
		generate_music(PLAYER_MOVED);
		
	}

	// | 3. Update the player location (player_row and player_col).      |
	// for flashing player
	old_player_moves[0] = player_row;
	old_player_moves[1] = player_col;
	
	// Sounds must be tones (not clicks) in the range 20Hz to 5kHz.
	
	//stop_tone();
	// sound works just figure how to make it last 200 ms or smth

	/*
	 Will implement the 3 tones for each of the following
	 1. Player being moved
	 2. Invalid move
	 3. Box pushed
	 Constraints
	 1. Pressing the “q”/“Q” key on the terminal should toggle the mute state of the game
	 2. If the Game Pause feature is implemented, no sound should play while the game is paused, however
		  playing sounds must resume where they were left off when unpaused. 
	 
	*/

	player_col = new_player_x;
	player_row = new_player_y;
	// flash terminal player here
	// flash_terminal_player(player_col, player_row, old_p_x, old_p_y);
	move_terminal_cursor(6,4);
	
	
	if (new_object_location != TARGET){
		clear_to_end_of_line();
		printf_P(PSTR("You've made a valid move!\n"));
		
		// player is just moving so 
		// we'll do that for now and then do and test the other 2
		// generate_music(PLAYER_MOVED);
	}
	
	if (diagonal_move){
		steps_glob = steps_glob + 2; //unbounded steps
		number_to_display = (number_to_display + 2) % 100;
		// steps_glob = steps;
	}
	else{
		steps_glob++;
		number_to_display = (number_to_display + 1) % 100;
	}
	
	
	
	move_terminal_cursor(3,4);
	printf_P(PSTR("Level: %d"), level);
	move_terminal_cursor(7,5);
	printf_P(PSTR("STEPS: %d"), steps_glob);
	move_terminal_cursor(0, 0);
	printf_P(PSTR("Joystick coordinates: x: %d y:%d     "), joy_x, joy_y);
	// step should keep incrementing 
	// number_to_display = (number_to_display + 1) % 100; // max steps is 99 on the Seven-segment display
	

	flash_player();  


	
	
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


void undo_move(uint8_t move_made[]){
	
	int i;
	// fix this because of array
	if (undo_capacity > 6){
		// If the undo capacity is 6 and another valid move is made, the oldest remembered
		// move is discarded and the newest move is added,
		// means that it is trying to store a 7th element
		for (i = 0; i < 6; i++){
			if (i == 5){
				undo_list[i] = move_made;
			}
			else{
				undo_list[i] = undo_list[i+1];
			}
		}
	}
	else{
		undo_list[undo_capacity] = move_made;
		undo_capacity++;
	}
}