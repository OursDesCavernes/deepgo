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
#include <stdlib.h>
#include <string.h>

#include "liberty.h"
#include "sgftree.h"
#include "gg_utils.h"

/* Return one if x doesn't equal position_number and 0 otherwise.
 * After using this macro x will always have the value
 * position_number.
 */
#define NEEDS_UPDATE(x) (x != position_number ? (x = position_number, 1) : 0)


static int do_genmove(int color, float pure_threat_value,
		      int allowed_moves[BOARDMAX], float *value, int *resign);

/* Position numbers for which various examinations were last made. */


/* Reset some things in the engine. 
 *
 * This prepares the hash table for the reading code for use.  It
 * should be called when we start examine a new position.  
 */

void
reset_engine()
{
  /* Initialize things for hashing of positions. */
  reading_cache_clear();

  hashdata_recalc(&board_hash, board, board_ko_pos);

  /* Set up depth values (see comments there for details). */
  set_depth_values(get_level(), 0);
}


/* 
 * Generate computer move for color.
 *
 * Return the generated move.
 */

int
genmove(int color, float *value, int *resign)
{
  int move = BOARDMIN;
  if (resign)
    *resign = 0;

  while(move < BOARDMAX)
  {
    if(ON_BOARD(move) && (board[move] == EMPTY))
    {
      if(is_allowed_move(move,color))
        return move;
    }
    move++;
  }

  gg_assert(move == PASS_MOVE || ON_BOARD(move));

  return PASS_MOVE;
}


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
