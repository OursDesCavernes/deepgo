/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * This is GNU Go, a Go program. Contact gnugo@gnu.org, or see       *
 * http://www.gnu.org/software/gnugo/ for more information.          *
 *                                                                   *
 * Copyright 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,   *
 * 2008, 2009, 2010 and 2011 by the Free Software Foundation.        *
 *                                                                   *
 * This program is free software; you can redistribute it and/or     *
 * modify it under the terms of the GNU General Public License as    *
 * published by the Free Software Foundation - version 3 or          *
 * (at your option) any later version.                               *
 *                                                                   *
 * This program is distributed in the hope that it will be useful,   *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     *
 * GNU General Public License in file COPYING for more details.      *
 *                                                                   *
 * You should have received a copy of the GNU General Public         *
 * License along with this program; if not, write to the Free        *
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,       *
 * Boston, MA 02111, USA.                                            *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "gnugo.h"

#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "interface.h"
#include "liberty.h"
#include "gtp.h"
#include "gg_utils.h"

/* Internal state that's not part of the engine. */
static int report_uncertainty = 0;
static int gtp_orientation = 0;

static void gtp_print_code(int c);
static void gtp_print_vertices2(int n, int *moves);
static void rotate_on_input(int ai, int aj, int *bi, int *bj);
static void rotate_on_output(int ai, int aj, int *bi, int *bj);


#define DECLARE(func) static int func(char *s)

DECLARE(gtp_accurate_approxlib);
DECLARE(gtp_accuratelib);
DECLARE(gtp_advance_random_seed);
DECLARE(gtp_all_legal);
DECLARE(gtp_all_move_values);
DECLARE(gtp_attack);
DECLARE(gtp_attack_either);
DECLARE(gtp_captures);
DECLARE(gtp_clear_board);
DECLARE(gtp_clear_cache);
DECLARE(gtp_countlib);
DECLARE(gtp_cputime);
DECLARE(gtp_decrease_depths);
DECLARE(gtp_defend);
DECLARE(gtp_dragon_data);
DECLARE(gtp_dragon_stones);
DECLARE(gtp_dump_stack);
DECLARE(gtp_echo);
DECLARE(gtp_echo_err);
DECLARE(gtp_estimate_score);
DECLARE(gtp_final_score);
DECLARE(gtp_findlib);
DECLARE(gtp_finish_sgftrace);
DECLARE(gtp_fixed_handicap);
DECLARE(gtp_genmove);
DECLARE(gtp_genmove_black);
DECLARE(gtp_genmove_white);
DECLARE(gtp_get_handicap);
DECLARE(gtp_get_komi);
DECLARE(gtp_get_life_node_counter);
DECLARE(gtp_get_random_seed);
DECLARE(gtp_get_reading_node_counter);
DECLARE(gtp_get_trymove_counter);
DECLARE(gtp_gg_genmove);
DECLARE(gtp_gg_undo);
DECLARE(gtp_increase_depths);
DECLARE(gtp_invariant_hash);
DECLARE(gtp_invariant_hash_for_moves);
DECLARE(gtp_is_legal);
DECLARE(gtp_kgs_genmove_cleanup);
DECLARE(gtp_known_command);
DECLARE(gtp_ladder_attack);
DECLARE(gtp_last_move);
DECLARE(gtp_list_commands);
DECLARE(gtp_list_stones);
DECLARE(gtp_loadsgf);
DECLARE(gtp_move_probabilities);
DECLARE(gtp_move_uncertainty);
DECLARE(gtp_move_history);
DECLARE(gtp_name);
DECLARE(gtp_play);
DECLARE(gtp_playblack);
DECLARE(gtp_playwhite);
DECLARE(gtp_popgo);
DECLARE(gtp_printsgf);
DECLARE(gtp_program_version);
DECLARE(gtp_protocol_version);
DECLARE(gtp_query_boardsize);
DECLARE(gtp_query_orientation);
DECLARE(gtp_quit);
DECLARE(gtp_reg_genmove);
DECLARE(gtp_report_uncertainty);
DECLARE(gtp_reset_life_node_counter);
DECLARE(gtp_reset_reading_node_counter);
DECLARE(gtp_reset_trymove_counter);
DECLARE(gtp_restricted_genmove);
DECLARE(gtp_same_dragon);
DECLARE(gtp_set_boardsize);
DECLARE(gtp_set_komi);
DECLARE(gtp_set_level);
DECLARE(gtp_set_orientation);
DECLARE(gtp_set_random_seed);
DECLARE(gtp_showboard);
DECLARE(gtp_start_sgftrace);
DECLARE(gtp_time_left);
DECLARE(gtp_time_settings);
DECLARE(gtp_top_moves);
DECLARE(gtp_top_moves_black);
DECLARE(gtp_top_moves_white);
DECLARE(gtp_tryko);
DECLARE(gtp_trymove);
DECLARE(gtp_tune_move_ordering);
DECLARE(gtp_undo);
DECLARE(gtp_what_color);

/* List of known commands. */
static struct gtp_command commands[] = {
  {"accurate_approxlib",      gtp_accurate_approxlib},
  {"accuratelib",             gtp_accuratelib},
  {"advance_random_seed",     gtp_advance_random_seed},
  {"all_legal",        	      gtp_all_legal},
  {"all_move_values",         gtp_all_move_values},
  {"attack",           	      gtp_attack},
  {"attack_either",           gtp_attack_either},
  {"black",            	      gtp_playblack},
  {"boardsize",        	      gtp_set_boardsize},
  {"captures",        	      gtp_captures},
  {"clear_board",      	      gtp_clear_board},
  {"clear_cache",	      gtp_clear_cache},
  {"color",            	      gtp_what_color},
  {"countlib",         	      gtp_countlib},
  {"cputime",		      gtp_cputime},
  {"decrease_depths",  	      gtp_decrease_depths},
  {"defend",           	      gtp_defend},
  {"dump_stack",       	      gtp_dump_stack},
  {"echo" ,                   gtp_echo},
  {"echo_err" ,               gtp_echo_err},
  {"estimate_score",          gtp_estimate_score},
  {"final_score",             gtp_final_score},
  {"findlib",          	      gtp_findlib},
  {"finish_sgftrace",  	      gtp_finish_sgftrace},
  {"fixed_handicap",   	      gtp_fixed_handicap},
  {"genmove",                 gtp_genmove},
  {"genmove_black",           gtp_genmove_black},
  {"genmove_white",           gtp_genmove_white},
  {"get_handicap",   	      gtp_get_handicap},
  {"get_komi",        	      gtp_get_komi},
  {"get_life_node_counter",   gtp_get_life_node_counter},
  {"get_random_seed",  	      gtp_get_random_seed},
  {"get_reading_node_counter", gtp_get_reading_node_counter},
  {"get_trymove_counter",     gtp_get_trymove_counter},
  {"gg-undo",                 gtp_gg_undo},
  {"gg_genmove",              gtp_gg_genmove},
  {"help",                    gtp_list_commands},
  {"increase_depths",  	      gtp_increase_depths},
  {"invariant_hash_for_moves",gtp_invariant_hash_for_moves},
  {"invariant_hash",   	      gtp_invariant_hash},
  {"is_legal",         	      gtp_is_legal},
  {"kgs-genmove_cleanup",     gtp_kgs_genmove_cleanup},
  {"known_command",    	      gtp_known_command},
  {"komi",        	      gtp_set_komi},
  {"ladder_attack",    	      gtp_ladder_attack},
  {"last_move",    	      gtp_last_move},
  {"level",        	      gtp_set_level},
  {"list_commands",    	      gtp_list_commands},
  {"list_stones",    	      gtp_list_stones},
  {"loadsgf",          	      gtp_loadsgf},
  {"move_probabilities",      gtp_move_probabilities},
  {"move_uncertainty",	      gtp_move_uncertainty},
  {"move_history",	      gtp_move_history},
  {"name",                    gtp_name},
  {"new_score",               gtp_estimate_score},
  {"orientation",     	      gtp_set_orientation},
  {"play",            	      gtp_play},
  {"popgo",            	      gtp_popgo},
  {"printsgf",         	      gtp_printsgf},
  {"protocol_version",        gtp_protocol_version},
  {"query_boardsize",         gtp_query_boardsize},
  {"query_orientation",       gtp_query_orientation},
  {"quit",             	      gtp_quit},
  {"reg_genmove",             gtp_reg_genmove},
  {"report_uncertainty",      gtp_report_uncertainty},
  {"reset_life_node_counter", gtp_reset_life_node_counter},
  {"reset_reading_node_counter", gtp_reset_reading_node_counter},
  {"reset_trymove_counter",   gtp_reset_trymove_counter},
  {"restricted_genmove",      gtp_restricted_genmove},
  {"set_random_seed",  	      gtp_set_random_seed},
  {"showboard",        	      gtp_showboard},
  {"start_sgftrace",  	      gtp_start_sgftrace},
  {"time_left",               gtp_time_left},
  {"time_settings",           gtp_time_settings},
  {"top_moves",               gtp_top_moves},
  {"top_moves_black",         gtp_top_moves_black},
  {"top_moves_white",         gtp_top_moves_white},
  {"tryko",          	      gtp_tryko},
  {"trymove",          	      gtp_trymove},
  {"tune_move_ordering",      gtp_tune_move_ordering},
  {"undo",                    gtp_undo},
  {"version",                 gtp_program_version},
  {"white",            	      gtp_playwhite},
  {NULL,                      NULL}
};


/* Start playing using the Go Text Protocol. */
void
play_gtp(FILE *gtp_input, FILE *gtp_output, FILE *gtp_dump_commands,
	 int gtp_initial_orientation)
{
  /* Make sure `gtp_output' is unbuffered. (Line buffering is also
   * okay but not necessary. Block buffering breaks GTP mode.)
   *
   * FIXME: Maybe should go to `gtp.c'?
   */
  setbuf(gtp_output, NULL);

  /* Inform the GTP utility functions about the board size. */
  gtp_internal_set_boardsize(board_size);
  gtp_orientation = gtp_initial_orientation;
  gtp_set_vertex_transform_hooks(rotate_on_input, rotate_on_output);

  /* Initialize time handling. */
  init_timers();
  
  /* Prepare pattern matcher and reading code. */
  reset_engine();
  clearstats();
  gtp_main_loop(commands, gtp_input, gtp_output, gtp_dump_commands);
  if (showstatistics)
    showstats();
}


/****************************
 * Administrative commands. *
 ****************************/

/* Function:  Quit
 * Arguments: none
 * Fails:     never
 * Returns:   nothing
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_quit(char *s)
{
  UNUSED(s);
  gtp_success("");
  return GTP_QUIT;
}


/* Function:  Report protocol version.
 * Arguments: none
 * Fails:     never
 * Returns:   protocol version number
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_protocol_version(char *s)
{
  UNUSED(s);
  return gtp_success("%d", gtp_version);
}


/****************************
 * Program identity.        *
 ****************************/

/* Function:  Report the name of the program.
 * Arguments: none
 * Fails:     never
 * Returns:   program name
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_name(char *s)
{
  UNUSED(s);
  return gtp_success("GNU Go");
}




/* Function:  Report the version number of the program.
 * Arguments: none
 * Fails:     never
 * Returns:   version number
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_program_version(char *s)
{
  UNUSED(s);
  return gtp_success(VERSION);
}


/***************************
 * Setting the board size. *
 ***************************/

/* Function:  Set the board size to NxN and clear the board.
 * Arguments: integer
 * Fails:     board size outside engine's limits
 * Returns:   nothing
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_set_boardsize(char *s)
{
  int boardsize;

  if (sscanf(s, "%d", &boardsize) < 1)
    return gtp_failure("boardsize not an integer");
  
  if (!check_boardsize(boardsize, NULL)) {
    if (gtp_version == 1)
      return gtp_failure("unacceptable boardsize");
    else
      return gtp_failure("unacceptable size");
  }

  /* If this is called with a non-empty board, we assume that a new
   * game will be started, for which we want a new random seed.
   */
  if (stones_on_board(BLACK | WHITE) > 0)
    update_random_seed();

  board_size = boardsize;
  clear_board();
  gtp_internal_set_boardsize(boardsize);
  reset_engine();
  return gtp_success("");
}

/* Function:  Find the current boardsize
 * Arguments: none
 * Fails:     never
 * Returns:   board_size
 */
static int
gtp_query_boardsize(char *s)
{
  UNUSED(s);

  return gtp_success("%d", board_size);
}

/***********************
 * Clearing the board. *
 ***********************/

/* Function:  Clear the board.
 * Arguments: none
 * Fails:     never
 * Returns:   nothing
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_clear_board(char *s)
{
  UNUSED(s);

  /* If this is called with a non-empty board, we assume that a new
   * game will be started, for which we want a new random seed.
   */
  if (stones_on_board(BLACK | WHITE) > 0)
    update_random_seed();

  clear_board();
  init_timers();
  
  return gtp_success("");
}

/****************************
 * Setting the orientation. *
 ****************************/

/* Function:  Set the orienation to N and clear the board
 * Arguments: integer
 * Fails:     illegal orientation
 * Returns:   nothing
 */
static int
gtp_set_orientation(char *s)
{
  int orientation;
  if (sscanf(s, "%d", &orientation) < 1)
    return gtp_failure("orientation not an integer");
  
  if (orientation < 0 || orientation > 7)
    return gtp_failure("unacceptable orientation");

  clear_board();
  gtp_orientation = orientation;
  gtp_set_vertex_transform_hooks(rotate_on_input, rotate_on_output);
  return gtp_success("");
}

/* Function:  Find the current orientation
 * Arguments: none
 * Fails:     never
 * Returns:   orientation
 */
static int
gtp_query_orientation(char *s)
{
  UNUSED(s);

  return gtp_success("%d", gtp_orientation);
}

/***************************
 * Setting komi.           *
 ***************************/

/* Function:  Set the komi.
 * Arguments: float
 * Fails:     incorrect argument
 * Returns:   nothing
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_set_komi(char *s)
{
  if (sscanf(s, "%f", &komi) < 1)
    return gtp_failure("komi not a float");
  
  return gtp_success("");
}


/***************************
 * Getting komi            *
 ***************************/

/* Function:  Get the komi
 * Arguments: none
 * Fails:     never
 * Returns:   Komi 
 */
static int
gtp_get_komi(char *s)
{
  UNUSED(s);
  return gtp_success("%4.1f", komi);
}


/******************
 * Playing moves. *
 ******************/

/* Function:  Play a black stone at the given vertex.
 * Arguments: vertex
 * Fails:     invalid vertex, illegal move
 * Returns:   nothing
 *
 * Status:    Obsolete GTP version 1 command.
 */
static int
gtp_playblack(char *s)
{
  int i, j;
  char *c;

  for (c = s; *c; c++)
    *c = tolower((int)*c);

  if (strncmp(s, "pass", 4) == 0) {
    i = -1;
    j = -1;
  }
  else if (!gtp_decode_coord(s, &i, &j))
    return gtp_failure("invalid coordinate");

  if (!is_allowed_move(POS(i, j), BLACK))
    return gtp_failure("illegal move");

  gnugo_play_move(POS(i, j), BLACK);
  return gtp_success("");
}


/* Function:  Play a white stone at the given vertex.
 * Arguments: vertex
 * Fails:     invalid vertex, illegal move
 * Returns:   nothing
 *
 * Status:    Obsolete GTP version 1 command.
 */
static int
gtp_playwhite(char *s)
{
  int i, j;
  char *c;

  for (c = s; *c; c++)
    *c = tolower((int)*c);

  if (strncmp(s, "pass", 4) == 0) {
    i = -1;
    j = -1;
  }
  else if (!gtp_decode_coord(s, &i, &j))
    return gtp_failure("invalid coordinate");
  
  if (!is_allowed_move(POS(i, j), WHITE))
    return gtp_failure("illegal move");

  gnugo_play_move(POS(i, j), WHITE);
  return gtp_success("");
}


/* Function:  Play a stone of the given color at the given vertex.
 * Arguments: color, vertex
 * Fails:     invalid vertex, illegal move
 * Returns:   nothing
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_play(char *s)
{
  int i, j;
  int color;

  if (!gtp_decode_move(s, &color, &i, &j))
    return gtp_failure("invalid color or coordinate");

  if (!is_allowed_move(POS(i, j), color))
    return gtp_failure("illegal move");

  gnugo_play_move(POS(i, j), color);
  return gtp_success("");
}


/* Function:  Set up fixed placement handicap stones.
 * Arguments: number of handicap stones
 * Fails:     invalid number of stones for the current boardsize
 * Returns:   list of vertices with handicap stones
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_fixed_handicap(char *s)
{
  int m, n;
  int first = 1;
  int this_handicap;

  if (gtp_version == 1)
    clear_board();
  else if (stones_on_board(BLACK | WHITE) > 0)
    return gtp_failure("board not empty");

  if (sscanf(s, "%d", &this_handicap) < 1)
    return gtp_failure("handicap not an integer");
  
  if (this_handicap < 2 && (gtp_version > 1 || this_handicap != 0))
    return gtp_failure("invalid handicap");

  if (place_fixed_handicap(this_handicap) != this_handicap) {
    clear_board();
    return gtp_failure("invalid handicap");
  }

  handicap = this_handicap;

  gtp_start_response(GTP_SUCCESS);

  for (m = 0; m < board_size; m++)
    for (n = 0; n < board_size; n++)
      if (BOARD(m, n) != EMPTY) {
	if (!first)
	  gtp_printf(" ");
	else
	  first = 0;
	gtp_mprintf("%m", m, n);
      }
  
  return gtp_finish_response();
}


/* Function:  Get the handicap
 * Arguments: none
 * Fails:     never
 * Returns:   handicap
 */
static int
gtp_get_handicap(char *s)
{
  UNUSED(s);
  return gtp_success("%d", handicap);
}


/* Function:  Load an sgf file, possibly up to a move number or the first
 *            occurence of a move.           
 * Arguments: filename + move number, vertex, or nothing
 * Fails:     missing filename or failure to open or parse file
 * Returns:   color to play
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_loadsgf(char *s)
{
  char filename[GTP_BUFSIZE];
  char untilstring[GTP_BUFSIZE];
  SGFTree sgftree;
  Gameinfo gameinfo;
  int nread;
  int color_to_move;
  
  nread = sscanf(s, "%s %s", filename, untilstring);
  if (nread < 1)
    return gtp_failure("missing filename");

  sgftree_clear(&sgftree);
  if (!sgftree_readfile(&sgftree, filename))
    return gtp_failure("cannot open or parse '%s'", filename);

  if (nread == 1)
    color_to_move = gameinfo_play_sgftree_rot(&gameinfo, &sgftree, NULL,
					      gtp_orientation);
  else
    color_to_move = gameinfo_play_sgftree_rot(&gameinfo, &sgftree, untilstring,
                                              gtp_orientation);

  if (color_to_move == EMPTY)
    return gtp_failure("cannot load '%s'", filename);
  
  gtp_internal_set_boardsize(board_size);
  reset_engine();
  init_timers();

  sgfFreeNode(sgftree.root);

  gtp_start_response(GTP_SUCCESS);
  gtp_mprintf("%C", color_to_move);
  return gtp_finish_response();
}


/*****************
 * Board status. *
 *****************/

/* Function:  Return the color at a vertex.
 * Arguments: vertex
 * Fails:     invalid vertex
 * Returns:   "black", "white", or "empty"
 */
static int
gtp_what_color(char *s)
{
  int i, j;
  if (!gtp_decode_coord(s, &i, &j))
    return gtp_failure("invalid coordinate");
  
  return gtp_success(color_to_string(BOARD(i, j)));
}


/* Function:  List vertices with either black or white stones.
 * Arguments: color
 * Fails:     invalid color
 * Returns:   list of vertices
 */
static int
gtp_list_stones(char *s)
{
  int i, j;
  int color = EMPTY;
  int vertexi[MAX_BOARD * MAX_BOARD];
  int vertexj[MAX_BOARD * MAX_BOARD];
  int vertices = 0;
  
  if (!gtp_decode_color(s, &color))
    return gtp_failure("invalid color");

  for (i = 0; i < board_size; i++)
    for (j = 0; j < board_size; j++)
      if (BOARD(i, j) == color) {
	vertexi[vertices] = i;
	vertexj[vertices++] = j;
      }

  gtp_start_response(GTP_SUCCESS);
  gtp_print_vertices(vertices, vertexi, vertexj);
  return gtp_finish_response();
}


/* Function:  Count number of liberties for the string at a vertex.
 * Arguments: vertex
 * Fails:     invalid vertex, empty vertex
 * Returns:   Number of liberties.
 */
static int
gtp_countlib(char *s)
{
  int i, j;
  if (!gtp_decode_coord(s, &i, &j))
    return gtp_failure("invalid coordinate");

  if (BOARD(i, j) == EMPTY)
    return gtp_failure("vertex must not be empty");

  return gtp_success("%d", countlib(POS(i, j)));
}


/* Function:  Return the positions of the liberties for the string at a vertex.
 * Arguments: vertex
 * Fails:     invalid vertex, empty vertex
 * Returns:   Sorted space separated list of vertices.
 */
static int
gtp_findlib(char *s)
{
  int i, j;
  int libs[MAXLIBS];
  int liberties;
  
  if (!gtp_decode_coord(s, &i, &j))
    return gtp_failure("invalid coordinate");

  if (BOARD(i, j) == EMPTY)
    return gtp_failure("vertex must not be empty");

  liberties = findlib(POS(i, j), MAXLIBS, libs);
  gtp_start_response(GTP_SUCCESS);
  gtp_print_vertices2(liberties, libs);
  return gtp_finish_response();
}


/* Function:  Determine which liberties a stone of given color
 *            will get if played at given vertex.
 * Arguments: move (color + vertex)
 * Fails:     invalid color, invalid vertex, occupied vertex
 * Returns:   Sorted space separated list of liberties
 */
static int
gtp_accuratelib(char *s)
{
  int i, j;
  int color;
  int libs[MAXLIBS];
  int liberties;

  if (!gtp_decode_move(s, &color, &i, &j))
    return gtp_failure("invalid color or coordinate");

  if (BOARD(i, j) != EMPTY)
    return gtp_failure("vertex must be empty");

  liberties = accuratelib(POS(i, j), color, MAXLIBS, libs);

  gtp_start_response(GTP_SUCCESS);
  gtp_print_vertices2(liberties, libs);
  return gtp_finish_response();
}


/* Function:  Determine which liberties a stone of given color
 *            will get if played at given vertex.
 * Arguments: move (color + vertex)
 * Fails:     invalid color, invalid vertex, occupied vertex
 * Returns:   Sorted space separated list of liberties
 *
 * Supposedly identical in behavior to the above function and
 * can be retired when this is confirmed.
 */
static int
gtp_accurate_approxlib(char *s)
{
  int i, j;
  int color;
  int libs[MAXLIBS];
  int liberties;

  if (!gtp_decode_move(s, &color, &i, &j))
    return gtp_failure("invalid color or coordinate");

  if (BOARD(i, j) != EMPTY)
    return gtp_failure("vertex must be empty");

  liberties = accuratelib(POS(i, j), color, MAXLIBS, libs);

  gtp_start_response(GTP_SUCCESS);
  gtp_print_vertices2(liberties, libs);
  return gtp_finish_response();
}


/* Function:  Tell whether a move is legal.
 * Arguments: move
 * Fails:     invalid move
 * Returns:   1 if the move is legal, 0 if it is not.
 */
static int
gtp_is_legal(char *s)
{
  int i, j;
  int color;
  
  if (!gtp_decode_move(s, &color, &i, &j))
    return gtp_failure("invalid color or coordinate");

  return gtp_success("%d", is_allowed_move(POS(i, j), color));
}


/* Function:  List all legal moves for either color.
 * Arguments: color
 * Fails:     invalid color
 * Returns:   Sorted space separated list of vertices.
 */
static int
gtp_all_legal(char *s)
{
  int i, j;
  int color;
  int movei[MAX_BOARD * MAX_BOARD];
  int movej[MAX_BOARD * MAX_BOARD];
  int moves = 0;
  
  if (!gtp_decode_color(s, &color))
    return gtp_failure("invalid color");

  for (i = 0; i < board_size; i++)
    for (j = 0; j < board_size; j++)
      if (BOARD(i, j) == EMPTY && is_allowed_move(POS(i, j), color)) {
	movei[moves] = i;
	movej[moves++] = j;
      }

  gtp_start_response(GTP_SUCCESS);
  gtp_print_vertices(moves, movei, movej);
  return gtp_finish_response();
}


/* Function:  List the number of captures taken by either color.
 * Arguments: color
 * Fails:     invalid color
 * Returns:   Number of captures.
 */
static int
gtp_captures(char *s)
{
  int color;
  
  if (!gtp_decode_color(s, &color))
    return gtp_failure("invalid color");

  if (color == BLACK)
    return gtp_success("%d", white_captured);
  else
    return gtp_success("%d", black_captured);
}


/* Function:  Return the last move.
 * Arguments: none
 * Fails:     no previous move known
 * Returns:   Color and vertex of last move.
 */
static int
gtp_last_move(char *s)
{
  int pos;
  int color;
  UNUSED(s);
  
  if (move_history_pointer <= 0)
    return gtp_failure("no previous move known");
  
  pos = move_history_pos[move_history_pointer - 1];
  color = move_history_color[move_history_pointer - 1];
  
  gtp_start_response(GTP_SUCCESS);
  gtp_mprintf("%C %m", color, I(pos), J(pos));
  return gtp_finish_response();
}

/* Function:  Print the move history in reverse order
 * Arguments: none
 * Fails:     never
 * Returns:   List of moves played in reverse order in format: 
 *            color move (one move per line)
 */
static int
gtp_move_history(char *s)
{
  int k, pos, color;
  UNUSED(s);
  
  gtp_start_response(GTP_SUCCESS);
  if (move_history_pointer > 0)
    for (k = move_history_pointer-1; k >= 0; k--) {
      color = move_history_color[k];
      pos = move_history_pos[k];
      gtp_mprintf("%C %m\n", color, I(pos), J(pos));
    }
  else
    gtp_printf("\n");
  gtp_printf("\n");
  return GTP_OK;
}


/* Function:  Return the rotation/reflection invariant board hash.
 * Arguments: none
 * Fails:     never
 * Returns:   Invariant hash for the board as a hexadecimal number.
 */
static int
gtp_invariant_hash(char *s)
{
  Hash_data hash;
  UNUSED(s);
  hashdata_calc_orientation_invariant(&hash, board, board_ko_pos);
  return gtp_success("%s", hashdata_to_string(&hash));
}


/* Function:  Return the rotation/reflection invariant board hash
 *            obtained by playing all the possible moves for the
 *            given color.
 * Arguments: color
 * Fails:     invalid color
 * Returns:   List of moves + invariant hash as a hexadecimal number,
 *            one pair of move + hash per line.
 */
static int
gtp_invariant_hash_for_moves(char *s)
{
  Hash_data hash;
  int color;
  int pos;
  int move_found = 0;
  
  if (!gtp_decode_color(s, &color))
    return gtp_failure("invalid color");

  gtp_start_response(GTP_SUCCESS);

  for (pos = BOARDMIN; pos < BOARDMAX; pos++) {
    if (board[pos] == EMPTY
	&& trymove(pos, color, "gtp_invariant_hash_for_moves", NO_MOVE)) {
      hashdata_calc_orientation_invariant(&hash, board, board_ko_pos);
      gtp_mprintf("%m %s\n", I(pos), J(pos), hashdata_to_string(&hash));
      popgo();
      move_found = 1;
    }
  }

  if (!move_found)
    gtp_printf("\n");
  
  gtp_printf("\n");
  return GTP_OK;
}



/**********************
 * Retractable moves. *
 **********************/

/* Function:  Play a stone of the given color at the given vertex.
 * Arguments: move (color + vertex)
 * Fails:     invalid color, invalid vertex, illegal move
 * Returns:   nothing
 */
static int
gtp_trymove(char *s)
{
  int i, j;
  int color;
  if (!gtp_decode_move(s, &color, &i, &j))
    return gtp_failure("invalid color or coordinate");

  if (!trymove(POS(i, j), color, "gtp_trymove", NO_MOVE))
    return gtp_failure("illegal move");

  return gtp_success("");
}

/* Function:  Play a stone of the given color at the given vertex, 
 *            allowing illegal ko capture.
 * Arguments: move (color + vertex)
 * Fails:     invalid color, invalid vertex, illegal move
 * Returns:   nothing
 */
static int
gtp_tryko(char *s)
{
  int i, j;
  int color;
  if (!gtp_decode_move(s, &color, &i, &j) || POS(i, j) == PASS_MOVE)
    return gtp_failure("invalid color or coordinate");

  if (!tryko(POS(i, j), color, "gtp_tryko"))
    return gtp_failure("illegal move");

  return gtp_success("");
}


/* Function:  Undo a trymove or tryko.
 * Arguments: none
 * Fails:     stack empty
 * Returns:   nothing
 */
static int
gtp_popgo(char *s)
{
  UNUSED(s);

  if (stackp == 0)
    return gtp_failure("Stack empty.");

  popgo();
  return gtp_success("");
}

/*********************
 * Caching	     *
 *********************/

/* Function:  clear the caches.
 * Arguments: none.
 * Fails:     never.
 * Returns:   nothing.
 */

static int
gtp_clear_cache(char *s)
{
  UNUSED(s);
  reading_cache_clear();
  return gtp_success("");
}

/*********************
 * Tactical reading. *
 *********************/

/* Function:  Try to attack a string.
 * Arguments: vertex
 * Fails:     invalid vertex, empty vertex
 * Returns:   attack code followed by attack point if attack code nonzero.
 */
static int
gtp_attack(char *s)
{
  int i, j;
  int apos;
  int attack_code;
  
  if (!gtp_decode_coord(s, &i, &j))
    return gtp_failure("invalid coordinate");

  if (BOARD(i, j) == EMPTY)
    return gtp_failure("vertex must not be empty");

  attack_code = attack(POS(i, j), &apos);
  gtp_start_response(GTP_SUCCESS);
  gtp_print_code(attack_code);
  if (attack_code > 0) {
    gtp_printf(" ");
    gtp_print_vertex(I(apos), J(apos));
  }
  return gtp_finish_response();
}  


/* Function:  Try to attack either of two strings
 * Arguments: two vertices
 * Fails:     invalid vertex, empty vertex
 * Returns:   attack code against the strings.  Guarantees there
 *            exists a move which will attack one of the two
 *            with attack_code, but does not return the move.
 */
static int
gtp_attack_either(char *s)
{
  int ai, aj;
  int bi, bj;
  int n;
  int acode;

  n = gtp_decode_coord(s, &ai, &aj);
  if (n == 0)
    return gtp_failure("invalid coordinate");

  if (BOARD(ai, aj) == EMPTY)
    return gtp_failure("string vertex must be empty");

  n = gtp_decode_coord(s + n, &bi, &bj);
  if (n == 0)
    return gtp_failure("invalid coordinate");

  if (BOARD(bi, bj) == EMPTY)
    return gtp_failure("string vertex must not be empty");

  acode = attack_either(POS(ai, aj), POS(bi, bj));

  gtp_start_response(GTP_SUCCESS);
  gtp_print_code(acode);
  return gtp_finish_response();
}


/* Function:  Try to defend a string.
 * Arguments: vertex
 * Fails:     invalid vertex, empty vertex
 * Returns:   defense code followed by defense point if defense code nonzero.
 */
static int
gtp_defend(char *s)
{
  int i, j;
  int dpos;
  int defend_code;
  
  if (!gtp_decode_coord(s, &i, &j))
    return gtp_failure("invalid coordinate");

  if (BOARD(i, j) == EMPTY)
    return gtp_failure("vertex must not be empty");

  defend_code = find_defense(POS(i, j), &dpos);
  gtp_start_response(GTP_SUCCESS);
  gtp_print_code(defend_code);
  if (defend_code > 0) {
    gtp_printf(" ");
    gtp_print_vertex(I(dpos), J(dpos));
  }
  return gtp_finish_response();
}  


/* Function:  Try to attack a string strictly in a ladder.
 * Arguments: vertex
 * Fails:     invalid vertex, empty vertex
 * Returns:   attack code followed by attack point if attack code nonzero.
 */
static int
gtp_ladder_attack(char *s)
{
  int i, j;
  int apos;
  int attack_code;
  
  if (!gtp_decode_coord(s, &i, &j))
    return gtp_failure("invalid coordinate");

  if (BOARD(i, j) == EMPTY)
    return gtp_failure("vertex must not be empty");

  if (countlib(POS(i, j)) != 2)
    return gtp_failure("string must have exactly 2 liberties");

  attack_code = simple_ladder(POS(i, j), &apos);
  gtp_start_response(GTP_SUCCESS);
  gtp_print_code(attack_code);
  if (attack_code > 0) {
    gtp_printf(" ");
    gtp_print_vertex(I(apos), J(apos));
  }
  return gtp_finish_response();
}  


/* Function:  Increase depth values by one.
 * Arguments: none
 * Fails:     never
 * Returns:   nothing
 */
static int
gtp_increase_depths(char *s)
{
  UNUSED(s);
  increase_depth_values();
  return gtp_success("");
}  


/* Function:  Decrease depth values by one.
 * Arguments: none
 * Fails:     never
 * Returns:   nothing
 */
static int
gtp_decrease_depths(char *s)
{
  UNUSED(s);
  decrease_depth_values();
  return gtp_success("");
}  


/********************
 * generating moves *
 ********************/

/* Function:  Generate and play the supposedly best black move.
 * Arguments: none
 * Fails:     never
 * Returns:   a move coordinate or "PASS"
 *
 * Status:    Obsolete GTP version 1 command.
 */
static int
gtp_genmove_black(char *s)
{
  int move;
  UNUSED(s);

  if (stackp > 0)
    return gtp_failure("genmove cannot be called when stackp > 0");

  move = genmove(BLACK, NULL, NULL);

  gnugo_play_move(move, BLACK);

  gtp_start_response(GTP_SUCCESS);
  gtp_print_vertex(I(move), J(move));
  return gtp_finish_response();
}

/* Function:  Generate and play the supposedly best white move.
 * Arguments: none
 * Fails:     never
 * Returns:   a move coordinate or "PASS"
 *
 * Status:    Obsolete GTP version 1 command.
 */
static int
gtp_genmove_white(char *s)
{
  int move;
  UNUSED(s);

  if (stackp > 0)
    return gtp_failure("genmove cannot be called when stackp > 0");

  move = genmove(WHITE, NULL, NULL);

  gnugo_play_move(move, WHITE);

  gtp_start_response(GTP_SUCCESS);
  gtp_print_vertex(I(move), J(move));
  return gtp_finish_response();
}

/* Function:  Generate and play the supposedly best move for either color.
 * Arguments: color to move
 * Fails:     invalid color
 * Returns:   a move coordinate or "PASS" (or "resign" if resignation_allowed)
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_genmove(char *s)
{
  int move;
  int resign;
  int color;
  int n;

  n = gtp_decode_color(s, &color);
  if (!n)
    return gtp_failure("invalid color");

  if (stackp > 0)
    return gtp_failure("genmove cannot be called when stackp > 0");

  adjust_level_offset(color);
  move = genmove(color, NULL, &resign);

  if (resign)
    return gtp_success("resign");

  gnugo_play_move(move, color);

  gtp_start_response(GTP_SUCCESS);
  gtp_print_vertex(I(move), J(move));
  return gtp_finish_response();
}


/* Function:  Generate the supposedly best move for either color.
 * Arguments: color to move
 * Fails:     invalid color
 * Returns:   a move coordinate (or "PASS")
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_reg_genmove(char *s)
{
  int move;
  int color;
  int n;
  unsigned int saved_random_seed = get_random_seed();

  n = gtp_decode_color(s, &color);
  if (!n)
    return gtp_failure("invalid color");

  if (stackp > 0)
    return gtp_failure("genmove cannot be called when stackp > 0");

  /* This is intended for regression purposes and should therefore be
   * deterministic. The best way to ensure this is to reset the random
   * number generator before calling genmove(). It is always seeded by
   * 0.
   */
  set_random_seed(0);
  
  move = genmove(color, NULL, NULL);

  set_random_seed(saved_random_seed);
  gtp_start_response(GTP_SUCCESS);
  gtp_print_vertex(I(move), J(move));
  return gtp_finish_response();
}

/* Function:  Generate the supposedly best move for either color.
 * Arguments: color to move, optionally a random seed
 * Fails:     invalid color
 * Returns:   a move coordinate (or "PASS")
 *
 * This differs from reg_genmove in the optional random seed.
 */
static int
gtp_gg_genmove(char *s)
{
  int move;
  int color;
  int n;
  unsigned int saved_random_seed = get_random_seed();
  unsigned int seed;

  n = gtp_decode_color(s, &color);
  if (!n)
    return gtp_failure("invalid color");

  if (stackp > 0)
    return gtp_failure("genmove cannot be called when stackp > 0");

  /* This is intended for regression purposes and should therefore be
   * deterministic. The best way to ensure this is to reset the random
   * number generator before calling genmove(). By default it is
   * seeded with 0, but if an optional unsigned integer is given in
   * the command after the color, this is used as seed instead.
   */
  seed = 0;
  sscanf(s+n, "%u", &seed);
  set_random_seed(seed);
  
  move = genmove(color, NULL, NULL);
  set_random_seed(saved_random_seed);
  gtp_start_response(GTP_SUCCESS);
  gtp_print_vertex(I(move), J(move));
  return gtp_finish_response();
}


/* Function:  Generate the supposedly best move for either color from a
 *            choice of allowed vertices.
 * Arguments: color to move, allowed vertices
 * Fails:     invalid color, invalid vertex, no vertex listed
 * Returns:   a move coordinate (or "PASS")
 */
static int
gtp_restricted_genmove(char *s)
{
  int move;
  int i, j;
  int color;
  int n;
  unsigned int saved_random_seed = get_random_seed();
  int allowed_moves[BOARDMAX];
  int number_allowed_moves = 0;
  memset(allowed_moves, 0, sizeof(allowed_moves));

  n = gtp_decode_color(s, &color);
  if (!n)
    return gtp_failure("invalid color");

  s += n;
  while (1) {
    n = gtp_decode_coord(s, &i, &j);
    if (n > 0) {
      allowed_moves[POS(i, j)] = 1;
      number_allowed_moves++;
      s += n;
    }
    else if (sscanf(s, "%*s") != EOF)
      return gtp_failure("invalid coordinate");
    else
      break;
  }

  if (number_allowed_moves == 0)
    return gtp_failure("no allowed vertex");

  if (stackp > 0)
    return gtp_failure("genmove cannot be called when stackp > 0");

  /* This is intended for regression purposes and should therefore be
   * deterministic. The best way to ensure this is to reset the random
   * number generator before calling genmove(). It is always seeded by
   * 0.
   */
  set_random_seed(0);
  
  move = genmove(color, NULL, NULL);
  set_random_seed(saved_random_seed);
  gtp_start_response(GTP_SUCCESS);
  gtp_print_vertex(I(move), J(move));
  return gtp_finish_response();
}


/* Function:  Generate and play the supposedly best move for either color,
 *            not passing until all dead opponent stones have been removed.
 * Arguments: color to move
 * Fails:     invalid color
 * Returns:   a move coordinate (or "PASS")
 *
 * Status:    KGS specific command.
 *
 * A similar command, but possibly somewhat different, will likely be added
 * to GTP version 3 at a later time.
 */
static int
gtp_kgs_genmove_cleanup(char *s)
{
  int move;
  int color;
  int n;
  int save_capture_all_dead = capture_all_dead;

  n = gtp_decode_color(s, &color);
  if (!n)
    return gtp_failure("invalid color");

  if (stackp > 0)
    return gtp_failure("kgs-genmove_cleanup cannot be called when stackp > 0");

  /* Turn on the capture_all_dead option to force removal of dead
   * opponent stones.
   */
  capture_all_dead = 1;
  
  adjust_level_offset(color);
  move = genmove(color, NULL, NULL);

  capture_all_dead = save_capture_all_dead;
  
  gnugo_play_move(move, color);

  gtp_start_response(GTP_SUCCESS);
  gtp_print_vertex(I(move), J(move));
  return gtp_finish_response();
}


/* Function : Generate a list of all moves with values larger than zero in
 *            the previous genmove command.
 *            If no previous genmove command has been issued, the result
 *            of this command will be meaningless.
 * Arguments: none
 * Fails:   : never
 * Returns  : list of moves with values
 */

static int
gtp_all_move_values(char *s)
{
  UNUSED(s);
  gtp_start_response(GTP_SUCCESS);
//TODO
//  print_all_move_values(gtp_output_file);
  gtp_printf("\n");
  return GTP_OK;
}

/* Function : Generate a sorted list of the best moves in the previous genmove
 *            command.
 *            If no previous genmove command has been issued, the result
 *            of this command will be meaningless.
 * Arguments: none
 * Fails:   : never
 * Returns  : list of moves with weights
 */

/* FIXME: Don't we want the moves one per row? */
static int
gtp_top_moves(char *s)
{
  int k;
  UNUSED(s);
  gtp_start_response(GTP_SUCCESS);
  for (k = 0; k < 10; k++)
    if (best_move_values[k] > 0.0) {
      gtp_print_vertex(I(best_moves[k]), J(best_moves[k]));
      gtp_printf(" %.2f ", best_move_values[k]);
    }
  gtp_printf("\n\n");
  return GTP_OK;
}

/* Function : Generate a list of the best moves for white with weights
 * Arguments: none
 * Fails:   : never
 * Returns  : list of moves with weights
 */

static int
gtp_top_moves_white(char *s)
{
  int k;
  UNUSED(s);
  genmove(WHITE, NULL, NULL);
  gtp_start_response(GTP_SUCCESS);
  for (k = 0; k < 10; k++)
    if (best_move_values[k] > 0.0) {
      gtp_print_vertex(I(best_moves[k]), J(best_moves[k]));
      gtp_printf(" %.2f ", best_move_values[k]);
    }
  return gtp_finish_response();
}

/* Function : Generate a list of the best moves for black with weights
 * Arguments: none
 * Fails:   : never
 * Returns  : list of moves with weights
 */

static int
gtp_top_moves_black(char *s)
{
  int k;
  UNUSED(s);
  genmove(BLACK, NULL, NULL);
  gtp_start_response(GTP_SUCCESS);
  for (k = 0; k < 10; k++)
    if (best_move_values[k] > 0.0) {
      gtp_print_vertex(I(best_moves[k]), J(best_moves[k]));
      gtp_printf(" %.2f ", best_move_values[k]);
    }
  return gtp_finish_response();
}



/* Function:  Set the playing level.
 * Arguments: int
 * Fails:     incorrect argument
 * Returns:   nothing
 */
static int
gtp_set_level(char *s)
{
  int new_level;
  if (sscanf(s, "%d", &new_level) < 1)
    return gtp_failure("level not an integer");
  
  set_level(new_level);
  return gtp_success("");
}

/* Function:  Undo one move
 * Arguments: none
 * Fails:     If move history is too short.
 * Returns:   nothing
 *
 * Status:    GTP version 2 standard command.
 */

static int
gtp_undo(char *s)
{
  UNUSED(s);

  if (stackp > 0 || !undo_move(1))
    return gtp_failure("cannot undo");

  reset_engine();
  
  return gtp_success("");
}


/* Function:  Undo a number of moves
 * Arguments: optional int
 * Fails:     If move history is too short.
 * Returns:   nothing
 */

static int
gtp_gg_undo(char *s)
{
  int number_moves = 1;

  sscanf(s, "%d", &number_moves);

  if (number_moves < 0)
    return gtp_failure("can't undo a negative number of moves");

  if (stackp > 0 || !undo_move(number_moves))
    return gtp_failure("cannot undo");

  reset_engine();
  
  return gtp_success("");
}


/*****************
 * time handling *
 *****************/

/* Function:  Set time allowance
 * Arguments: int main_time, int byo_yomi_time, int byo_yomi_stones
 * Fails:     syntax error
 * Returns:   nothing
 *
 * Status:    GTP version 2 standard command.
 */

static int
gtp_time_settings(char *s)
{
  int main_time, byoyomi_time, byoyomi_stones;
  
  if (sscanf(s, "%d %d %d", &main_time, &byoyomi_time, &byoyomi_stones) < 3)
    return gtp_failure("not three integers");

  clock_settings(main_time, byoyomi_time, byoyomi_stones);
  return gtp_success("");
}


/* Function:  Report remaining time
 * Arguments: color color, int time, int stones
 * Fails:     syntax error
 * Returns:   nothing
 *
 * Status:    GTP version 2 standard command.
 */

static int
gtp_time_left(char *s)
{
  int color;
  int time;
  int stones;
  int n;

  n = gtp_decode_color(s, &color);
  if (!n)
    return gtp_failure("invalid color");
  
  if (sscanf(s+n, "%d %d", &time, &stones) < 2)
    return gtp_failure("time and stones not two integers");

  update_time_left(color, time, stones);
  
  return gtp_success("");
}


/***********
 * scoring *
 ***********/

static float final_score;
static const char *status_names[6] = {"alive", "dead", "seki",
				      "white_territory", "black_territory",
				      "dame"};

/* Helper function. */
static void
finish_and_score_game(int seed)
{
  int move;
  int i, j;
  int next;
  int pass = 0;
  int moves = 0;
  int saved_board[MAX_BOARD][MAX_BOARD];
  struct board_state saved_pos;
  static int current_board[MAX_BOARD][MAX_BOARD];
  static int current_seed = -1;
  int cached_board = 1;

  if (current_seed != seed) {
    current_seed = seed;
    cached_board = 0;
  }

  for (i = 0; i < board_size; i++)
    for (j = 0; j < board_size; j++)
      if (BOARD(i, j) != current_board[i][j]) {
	current_board[i][j] = BOARD(i, j);
	cached_board = 0;
      }

  /* If this is exactly the same position as the one we analyzed the
   * last time, the contents of final_score and final_status are up to date.
   */
  if (cached_board)
    return;

  doing_scoring = 1;
  store_board(&saved_pos);

  /* Let black start if we have no move history. Otherwise continue
   * alternation.
   */
  if (get_last_player() == EMPTY)
    next = BLACK;
  else
    next = OTHER_COLOR(get_last_player());

  do {
    move = genmove(next, NULL, NULL);
    gnugo_play_move(move, next);
    if (move != PASS_MOVE) {
      pass = 0;
      moves++;
    }
    else
      pass++;

    next = OTHER_COLOR(next);
  } while (pass < 2 && moves < board_size * board_size);

  restore_board(&saved_pos);
  doing_scoring = 0;

  /* Update the status for vertices which were changed while finishing
   * the game, up to filling dame.
   */
/*  for (i = 0; i < board_size; i++)
    for (j = 0; j < board_size; j++) {
      if (BOARD(i, j) == saved_board[i][j])
	continue;

      if (BOARD(i, j) == EMPTY) {
	  final_status[i][j] = DAME;
	else if (final_status[i][j] == DEAD) {
	  if (saved_board[i][j] == BLACK)
	    final_status[i][j] = WHITE_TERRITORY;
	  else
	    final_status[i][j] = BLACK_TERRITORY;
	}
      }
      else if (BOARD(i, j) == BLACK) {
	if (final_status[i][j] == WHITE_TERRITORY)
	  final_status[i][j] = DEAD;
	else if (final_status[i][j] == DAME)
	  final_status[i][j] = ALIVE_IN_SEKI;
	else if (final_status[i][j] == BLACK_TERRITORY)
	  final_status[i][j] = ALIVE;
	else
	  final_status[i][j] = DEAD;
      }
      else if (BOARD(i, j) == WHITE) {
	if (final_status[i][j] == BLACK_TERRITORY)
	  final_status[i][j] = DEAD;
	else if (final_status[i][j] == DAME)
	  final_status[i][j] = ALIVE_IN_SEKI;
	else if (final_status[i][j] == WHITE_TERRITORY)
	  final_status[i][j] = ALIVE;
	else
	  final_status[i][j] = DEAD;
      }
    }*/
}


/* Function:  Compute the score of a finished game.
 * Arguments: Optional random seed
 * Fails:     never
 * Returns:   Score in SGF format (RE property).
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_final_score(char *s)
{
  unsigned int saved_random_seed = get_random_seed();
  int seed;
  /* This is intended for regression purposes and should therefore be
   * deterministic. The best way to ensure this is to reset the random
   * number generator before calling genmove(). By default it is
   * seeded with 0, but if an optional unsigned integer is given in
   * the command after the color, this is used as seed instead.
   */
  seed = 0;
  sscanf(s, "%d", &seed);
  set_random_seed(seed);

  finish_and_score_game(seed);

  set_random_seed(saved_random_seed);

  gtp_start_response(GTP_SUCCESS);
  if (final_score > 0.0)
    gtp_printf("W+%3.1f", final_score);
  else if (final_score < 0.0)
    gtp_printf("B+%3.1f", -final_score);
  else
    gtp_printf("0");
  return gtp_finish_response();
}


/* Function:  Estimate the score
 * Arguments: None
 * Fails:     never
 * Returns:   upper and lower bounds for the score
 */

static int
gtp_estimate_score(char *s)
{
  float score;
  float upper_bound, lower_bound;
  UNUSED(s);

  score = gnugo_estimate_score(&upper_bound, &lower_bound);
  gtp_start_response(GTP_SUCCESS);
  /* Traditionally W wins jigo */
  if (score >= 0.0) 
    gtp_printf("W+%3.1f (upper bound: %3.1f, lower: %3.1f)", 
	       score, upper_bound, lower_bound);
  else if (score < 0.0)
    gtp_printf("B+%3.1f (upper bound: %3.1f, lower: %3.1f)", 
	       -score, upper_bound, lower_bound);
  return gtp_finish_response();
}  


/**************
 * statistics *
 **************/

/* Function:  Reset the count of life nodes.
 * Arguments: none
 * Fails:     never
 * Returns:   nothing
 *
 * Note: This function is obsolete and only remains for backwards
 * compatibility.
 */
static int
gtp_reset_life_node_counter(char *s)
{
  UNUSED(s);
  return gtp_success("");
}


/* Function:  Retrieve the count of life nodes.
 * Arguments: none
 * Fails:     never
 * Returns:   number of life nodes
 *
 * Note: This function is obsolete and only remains for backwards
 * compatibility.
 */
static int
gtp_get_life_node_counter(char *s)
{
  UNUSED(s);
  return gtp_success("0");
}


/* Function:  Reset the count of reading nodes.
 * Arguments: none
 * Fails:     never
 * Returns:   nothing
 */
static int
gtp_reset_reading_node_counter(char *s)
{
  UNUSED(s);
  reset_reading_node_counter();
  return gtp_success("");
}


/* Function:  Retrieve the count of reading nodes.
 * Arguments: none
 * Fails:     never
 * Returns:   number of reading nodes
 */
static int
gtp_get_reading_node_counter(char *s)
{
  int nodes = get_reading_node_counter();
  UNUSED(s);
  return gtp_success("%d", nodes);
}


/* Function:  Reset the count of trymoves/trykos.
 * Arguments: none
 * Fails:     never
 * Returns:   nothing
 */
static int
gtp_reset_trymove_counter(char *s)
{
  UNUSED(s);
  reset_trymove_counter();
  return gtp_success("");
}


/* Function:  Retrieve the count of trymoves/trykos.
 * Arguments: none
 * Fails:     never
 * Returns:   number of trymoves/trykos
 */
static int
gtp_get_trymove_counter(char *s)
{
  int nodes = get_trymove_counter();
  UNUSED(s);
  return gtp_success("%d", nodes);
}


/*********
 * debug *
 *********/


/* Function:  Returns elapsed CPU time in seconds.
 * Arguments: none
 * Fails:     never
 * Returns:   Total elapsed (user + system) CPU time in seconds.
 */
static int
gtp_cputime(char *s)
{
  UNUSED(s);
  return gtp_success("%.3f", gg_cputime());
}



/* Function:  Write the position to stdout.
 * Arguments: none
 * Fails:     never
 * Returns:   nothing
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_showboard(char *s)
{
  UNUSED(s);
  
  gtp_start_response(GTP_SUCCESS);
  gtp_printf("\n");
  simple_showboard(gtp_output_file);
  return gtp_finish_response();
}


/* Function:  Dump stack to stderr.
 * Arguments: none
 * Fails:     never
 * Returns:   nothing
 */
static int
gtp_dump_stack(char *s)
{
  UNUSED(s);
  dump_stack();
  return gtp_success("");
}

/* Determine whether a string starts with a specific substring. */
static int
has_prefix(const char *s, const char *prefix)
{
  return strncmp(s, prefix, strlen(prefix)) == 0;
}



/* Function:  List probabilities of each move being played (when non-zero).
 *            If no previous genmove command has been issued, the result
 *            of this command will be meaningless.
 * Arguments: none
 * Fails:     never
 * Returns:   Move, probabilty pairs, one per row.
 */
static int
gtp_move_probabilities(char *s)
{
  float probabilities[BOARDMAX];
  int pos;
  int any_moves_printed = 0;

  UNUSED(s);

  memset(probabilities,0,BOARDMAX);

  gtp_start_response(GTP_SUCCESS);
  for (pos = BOARDMIN; pos < BOARDMAX; pos++) {
    if (ON_BOARD(pos) && probabilities[pos] != 0.0) {
      gtp_mprintf("%m ", I(pos), J(pos));
      gtp_printf("%.4f\n", probabilities[pos]);
      any_moves_printed = 1;
    }
  }

  if (!any_moves_printed)
    gtp_printf("\n");
  gtp_printf("\n");

  return GTP_OK;
}


/* Function:  Return the number of bits of uncertainty in the move.
 *            If no previous genmove command has been issued, the result
 *            of this command will be meaningless.
 * Arguments: none
 * Fails:     never
 * Returns:   bits of uncertainty
 */
static int
gtp_move_uncertainty(char *s)
{
  float probabilities[BOARDMAX];
  int pos;
  double uncertainty = 0.0;

  UNUSED(s);

  memset(probabilities,0,BOARDMAX);

  gtp_start_response(GTP_SUCCESS);
  for (pos = BOARDMIN; pos < BOARDMAX; pos++) {
    if (ON_BOARD(pos) && probabilities[pos] > 0.0) {
      /* Shannon's formula */
      uncertainty += -1 * ((double)probabilities[pos]) *
	log((double)probabilities[pos]) / log(2.0);
    }
  }

  gtp_printf("%.4f\n\n", uncertainty);

  return GTP_OK;
}



static SGFTree gtp_sgftree;

/* Function:  Start storing moves executed during reading in an sgf
 *            tree in memory. 
 * Arguments: none
 * Fails:     never
 * Returns:   nothing
 *
 * Warning: You had better know what you're doing if you try to use this
 *          command.
 */
static int
gtp_start_sgftrace(char *s)
{
  UNUSED(s);
  sgffile_begindump(&gtp_sgftree);
  count_variations = 1;
  return gtp_success("");
}


/* Function:  Finish storing moves in an sgf tree and write it to file. 
 * Arguments: filename
 * Fails:     never
 * Returns:   nothing
 *
 * Warning: You had better know what you're doing if you try to use this
 *          command.
 */
static int
gtp_finish_sgftrace(char *s)
{
  char filename[GTP_BUFSIZE];
  int nread;
  
  nread = sscanf(s, "%s", filename);
  if (nread < 1)
    return gtp_failure("missing filename");

  sgffile_enddump(filename);
  count_variations = 0;
  return gtp_success("");
}


/* Function:  Dump the current position as a static sgf file to filename,
 *            or as output if filename is missing or "-" 
 * Arguments: optional filename
 * Fails:     never
 * Returns:   nothing if filename, otherwise the sgf
 */
static int
gtp_printsgf(char *s)
{
  char filename[GTP_BUFSIZE];
  int nread;
  int next;
  
  if (get_last_player() == EMPTY)
    next = BLACK;
  else
    next = OTHER_COLOR(get_last_player());

  nread = sscanf(s, "%s", filename);

  if (nread < 1)
    gg_snprintf(filename, GTP_BUFSIZE, "%s", "-");

  if (strcmp(filename, "-") == 0) {
    gtp_start_response(GTP_SUCCESS);
    sgffile_printsgf(next, filename);
    gtp_printf("\n");
    return GTP_OK;
  }
  else {
    sgffile_printsgf(next, filename);
    return gtp_success("");
  }
}


/* Function:  Tune the parameters for the move ordering in the tactical
 *            reading.
 * Arguments: MOVE_ORDERING_PARAMETERS integers
 * Fails:     incorrect arguments
 * Returns:   nothing
 */
static int
gtp_tune_move_ordering(char *s)
{
  int params[MOVE_ORDERING_PARAMETERS];
  int k;
  int p;
  int n;

  for (k = 0; k < MOVE_ORDERING_PARAMETERS; k++) {
    if (sscanf(s, "%d%n", &p, &n) == 0)
      return gtp_failure("incorrect arguments, expected %d integers",
			 MOVE_ORDERING_PARAMETERS);
    params[k] = p;
    s += n;
  }

  tune_move_ordering(params);
  return gtp_success("");
}

/* Function:  Echo the parameter
 * Arguments: string
 * Fails:     never
 * Returns:   nothing
 */
static int
gtp_echo(char *s)
{
  return gtp_success("%s", s);
}


/* Function:  Echo the parameter to stdout AND stderr
 * Arguments: string
 * Fails:     never
 * Returns:   nothing
 */
static int
gtp_echo_err(char *s)
{
  fprintf(stderr, "%s", s);
  fflush(gtp_output_file);
  fflush(stderr);
  return gtp_success("%s", s);
}

/* Function:  List all known commands
 * Arguments: none
 * Fails:     never
 * Returns:   list of known commands, one per line
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_list_commands(char *s)
{
  int k;
  UNUSED(s);

  gtp_start_response(GTP_SUCCESS);

  for (k = 0; commands[k].name != NULL; k++)
    gtp_printf("%s\n", commands[k].name);

  gtp_printf("\n");
  return GTP_OK;
}


/* Function:  Tell whether a command is known.
 * Arguments: command name
 * Fails:     never
 * Returns:   "true" if command exists, "false" if not
 *
 * Status:    GTP version 2 standard command.
 */
static int
gtp_known_command(char *s)
{
  int k;
  char command[GTP_BUFSIZE];

  if (sscanf(s, "%s", command) == 1) {
    for (k = 0; commands[k].name != NULL; k++)
      if (strcmp(command, commands[k].name) == 0)
	return gtp_success("true");
  }

  return gtp_success("false");
}


/* Function:  Turn uncertainty reports from owl_attack
 *            and owl_defend on or off.
 * Arguments: "on" or "off"
 * Fails:     invalid argument
 * Returns:   nothing
 */
static int
gtp_report_uncertainty(char *s)
{
  if (!strncmp(s, "on", 2)) {
    report_uncertainty = 1;
    return gtp_success("");
  }
  if (!strncmp(s, "off", 3)) {
    report_uncertainty = 0;
    return gtp_success("");
  }
  return gtp_failure("invalid argument");
}
    

static void
gtp_print_code(int c)
{
  static int conversion[6] = { 
    0, /* LOSE */
    3, /* KO_B */
    5, /* LOSS */
    4, /* GAIN */
    2, /* KO_A */
    1, /* WIN  */
  };
  gtp_printf("%d", conversion[c]);
}

static void
gtp_print_vertices2(int n, int *moves)
{
  int movei[MAX_BOARD * MAX_BOARD];
  int movej[MAX_BOARD * MAX_BOARD];
  int k;

  for (k = 0; k < n; k++) {
    movei[k] = I(moves[k]);
    movej[k] = J(moves[k]);
  }
  
  gtp_print_vertices(n, movei, movej);
}

/*************
 * transform *
 *************/

static void
rotate_on_input(int ai, int aj, int *bi, int *bj)
{
  rotate(ai, aj, bi, bj, board_size, gtp_orientation);
}

static void
rotate_on_output(int ai, int aj, int *bi, int *bj)
{
  inv_rotate(ai, aj, bi, bj, board_size, gtp_orientation);
}


/***************
 * random seed *
 ***************/

/* Function:  Get the random seed
 * Arguments: none
 * Fails:     never
 * Returns:   random seed
 */
static int
gtp_get_random_seed(char *s)
{
  UNUSED(s);
  return gtp_success("%d", get_random_seed());
}

/* Function:  Set the random seed
 * Arguments: integer
 * Fails:     invalid data
 * Returns:   nothing
 */
static int
gtp_set_random_seed(char *s)
{
  int seed;
  if (sscanf(s, "%d", &seed) < 1)
    return gtp_failure("invalid seed");
  
  set_random_seed(seed);
  return gtp_success("");
}


/* Function:  Advance the random seed by a number of games.
 * Arguments: integer
 * Fails:     invalid data
 * Returns:   New random seed.
 */
static int
gtp_advance_random_seed(char *s)
{
  int i;
  int games;
  if (sscanf(s, "%d", &games) < 1
      || games < 0)
    return gtp_failure("invalid number of games");
  
  for (i = 0; i < games; i++)
    update_random_seed();

  return gtp_success("%d", get_random_seed());
}

/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
