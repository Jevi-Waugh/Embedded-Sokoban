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

extern volatile uint8_t steps_glob;
extern volatile level;


/// <summary>
/// Initialises the game.
/// </summary>
// void display_digit(uint8_t number, uint8_t digit) ;
// void seven_segment(uint8_t fixed_number);
void initialise_game(int level);
void wall_message();
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
bool move_player(int8_t delta_row, int8_t delta_col);

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
