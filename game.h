/*
 * game.h
 *
 * Authors: Jarrod Bennett, Cody Burnett, Bradley Stone, Yufeng Gao
 * Modified by: <YOUR NAME HERE>
 *
 * Function prototypes for game functions available externally. You may wish
 * to add extra function prototypes here to make other functions available to
 * other files.
 */

#ifndef GAME_H_
#define GAME_H_

#define PLAYER_MOVED 1
#define BOX_ON_TARGET 2
#define PUSHING_BOX 3

#include <stdint.h>
#include <stdbool.h>

// Object definitions.
#define ROOM       	(0U << 0)
#define WALL       	(1U << 0)
#define BOX        	(1U << 1)
#define TARGET     	(1U << 2)
#define OBJECT_MASK	(ROOM | WALL | BOX | TARGET)

// Colour definitions.
#define COLOUR_PLAYER	(COLOUR_DARK_GREEN)
#define COLOUR_WALL  	(COLOUR_YELLOW)
#define COLOUR_BOX   	(COLOUR_ORANGE)
#define COLOUR_TARGET	(COLOUR_RED)
#define COLOUR_DONE  	(COLOUR_GREEN)

extern volatile uint16_t steps_glob;
extern volatile int level;

extern volatile uint16_t freq;	// Hz
extern volatile float dutycycle;	// %
extern uint32_t last_target_area_flash_time;
extern uint8_t new_object_location;
extern uint8_t new_object_x;
extern uint8_t new_object_y;
extern bool target_met;
extern int undo_capacity;
extern uint8_t old_player_moves[2];

/// <summary>
/// Initialises the game.
/// </summary>
// void display_digit(uint8_t number, uint8_t digit) ;
// void seven_segment(uint8_t fixed_number);
void initialise_game(int level);
void reset_animation_display(uint8_t new_object_x,  uint8_t new_object_y);
void wall_message();
void flash_target_square();
void get_location_matrix(uint8_t y, uint8_t x);
void get_location_matrix2(uint8_t y, uint8_t x);
void undo_move(uint8_t move_made[]);
/// <summary>
/// Moves the player based on row and column deltas.
/// </summary>
/// <param name="delta_row">The row delta.</param>
/// <param name="delta_col">The column delta.</param>
void check_surroundings(uint8_t new_object_location);
void display_terminal_gameplay();
void reset_cursor_position();
void flash_terminal_player(uint8_t player_x, uint8_t player_y, uint8_t old_player_x, uint8_t old_player_y);
void update_terminal_moves(uint8_t object, uint8_t row, uint8_t col);
bool move_player(int8_t delta_row, int8_t delta_col, bool diagonal_move);

/// <summary>
/// Detects whether the game is over (i.e., current level solved).
/// </summary>
/// <returns>Whether the game is over.</returns>
bool is_game_over(void);

/// <summary>
/// Flashes the player icon.
/// </summary>
void flash_player(void);

#endif /* GAME_H_ */
