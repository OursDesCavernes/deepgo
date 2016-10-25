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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "liberty.h"
#include "sgftree.h"
#include "random.h"
#include "gg_utils.h"
#include "patterns.h"





/* Helper function to compute a * pow(b, e) with some safety nets to
 * handle overflow robustly.
 */
static int
exponential_level(int a, double b, double e)
{
  double x = pow(b, e);

  if (x > (double) INT_MAX / (double) a)
    return INT_MAX;

  return (int) (a * x);
}


/* 
 * It is assumed in reading a ladder if stackp >= depth that
 * as soon as a bounding stone is in atari, the string is safe.
 * It is used similarly at other places in reading.c to implement simplifying
 * assumptions when stackp is large. DEPTH is the default value of depth.
 *
 * Unfortunately any such scheme invites the ``horizon effect.'' Increasing
 * DEPTH will make the program stronger and slower.
 *
 */

/* Tactical reading using C functions */
#define DEPTH                16
#define BRANCH_DEPTH         13
#define BACKFILL_DEPTH       12
#define BACKFILL2_DEPTH       5
#define BREAK_CHAIN_DEPTH     7
#define SUPERSTRING_DEPTH     7
#define FOURLIB_DEPTH         7
#define KO_DEPTH              8

#if 0
#undef FOURLIB_DEPTH
#define FOURLIB_DEPTH         9
#endif


#define AA_DEPTH              6

/* Pattern based reading */
#define OWL_DISTRUST_DEPTH    6
#define OWL_BRANCH_DEPTH      8
#define OWL_READING_DEPTH    20
#define SEMEAI_BRANCH_DEPTH  12
#define SEMEAI_BRANCH_DEPTH2  6

/* Connecton reading */
#define CONNECT_NODE_LIMIT 2000
#define CONNECT_DEPTH        64
#define CONNECT_DEPTH2       20

#define BREAKIN_NODE_LIMIT  400
#define BREAKIN_DEPTH	     14

/* Set the various reading depth parameters. If mandated_depth_value
 * is not -1 that value is used; otherwise the depth values are
 * set as a function of level. The parameter mandated_depth_value
 * can be set at the command line to force a particular value of
 * depth; normally it is -1.
 */

void
set_depth_values(int level, int report_levels)
{
  static int node_limits[] = {500, 500, 450, 400, 400, 325, 275,
			      200, 150, 100, 75, 50};
  int depth_level;

  /*
   * Other policies depending on level:
   * owl.c:         >=  9: use vital attack pattern database
   *                >=  8: increase depth values in owl_substantial
   *                >=  8: don't turn off owl_phase in semeai reading
   * reading.c:     >=  8: Use superstrings and do more backfilling.
   * value_moves.c: >=  6: try to find more owl attacks/defenses
   * breakin.c:     >= 10: try to find break-ins. (*)
   * worm.c:        >= 10: detect unconditionally meaningless moves
   *
   * The break-in code (*) is particularly expensive. 
   *
   * Speedups between levels 9 and 10 and between levels 7 and 8
   * are obtained by turning off services, and between these
   * levels no changes are made in the depths. The parameter
   * depth_level is the correction compared to the default settings at level
   * 10 for most reading depths.
   */
  if (level >= 10)
    depth_level = level - 10;
  else if (level == 9)
    depth_level = 0;
  else if (level == 8)
    depth_level = -1;
  else 
    depth_level = level - 8;

  depth                = gg_max(6, DEPTH 	    + depth_level);
  branch_depth         = gg_max(3, BRANCH_DEPTH	    + depth_level);
  backfill_depth       = gg_max(2, BACKFILL_DEPTH    + depth_level);
  backfill2_depth      = gg_max(1, BACKFILL2_DEPTH   + depth_level);
  break_chain_depth    = gg_max(2, BREAK_CHAIN_DEPTH + depth_level);
  if (level >= 8)
    owl_distrust_depth = gg_max(1, (2 * OWL_DISTRUST_DEPTH + depth_level) / 2);
  else
    owl_distrust_depth = gg_max(1, (2 * OWL_DISTRUST_DEPTH - 1
				    + depth_level) / 2);
  owl_branch_depth     = gg_max(2, (2 * OWL_BRANCH_DEPTH   + depth_level) / 2);
  owl_reading_depth    = gg_max(5, (2 * OWL_READING_DEPTH  + depth_level) / 2);

  /* Atari-atari depth levels are unchanged only between levels 7/8, 9/10: */
  if (level >= 10)
    aa_depth = gg_max(0, AA_DEPTH + (level - 10));
  else if (level == 9)
    aa_depth = gg_max(0, AA_DEPTH);
  else if (level >= 7)
    aa_depth = gg_max(0, AA_DEPTH - 1);
  else
    aa_depth = gg_max(0, AA_DEPTH - (8 - level));

  /* Exceptions:
   * fourlib_depth: This is constant from levels 7 to 10.
   * superstring_depth: set to 0 below level 8.
   */
  if (level >= 10)
    ko_depth            = gg_max(1, KO_DEPTH + (level - 10));
  else if (level == 9)
    ko_depth            = gg_max(1, KO_DEPTH);
  else if (level >= 7)
    ko_depth            = gg_max(1, KO_DEPTH - 1);
  else
    ko_depth            = gg_max(1, KO_DEPTH + (level - 8));

  if (level >= 10)
    fourlib_depth       = gg_max(1, FOURLIB_DEPTH + (level - 10));
  else if (level >= 7)
    fourlib_depth       = gg_max(1, FOURLIB_DEPTH);
  else
    fourlib_depth       = gg_max(1, FOURLIB_DEPTH + (level - 7));

  if (level >= 8)
    superstring_depth = gg_max(1, SUPERSTRING_DEPTH);
  else
    superstring_depth = 0;

  if (level >= 10)
    owl_node_limit      = exponential_level(OWL_NODE_LIMIT, 1.5, depth_level);
  else {
    owl_node_limit      = (OWL_NODE_LIMIT * node_limits[10 - level] /
			   node_limits[0]);
    owl_node_limit      = gg_max(20, owl_node_limit);
  }

  semeai_branch_depth  = gg_max(2, (2*SEMEAI_BRANCH_DEPTH  + depth_level) / 2);
  semeai_branch_depth2 = gg_max(2, (2*SEMEAI_BRANCH_DEPTH2 + depth_level) / 2);
  semeai_node_limit    = exponential_level(SEMEAI_NODE_LIMIT, 1.5, depth_level);

  connect_depth         = gg_max(2, CONNECT_DEPTH  + 2 * depth_level);
  connect_depth2        = gg_max(2, CONNECT_DEPTH2 + 2 * depth_level);
  connection_node_limit = exponential_level(CONNECT_NODE_LIMIT, 1.45, depth_level);
  breakin_depth 	= gg_max(2, BREAKIN_DEPTH + 2 * depth_level);
  breakin_node_limit 	= exponential_level(BREAKIN_NODE_LIMIT, 1.5, depth_level);

  if (mandated_depth != -1)
    depth = mandated_depth;
  if (mandated_backfill_depth != -1)
    backfill_depth = mandated_backfill_depth;
  if (mandated_backfill2_depth != -1)
    backfill2_depth = mandated_backfill2_depth;
  if (mandated_break_chain_depth != -1)
    break_chain_depth = mandated_break_chain_depth;
  if (mandated_superstring_depth != -1)
    superstring_depth = mandated_superstring_depth;
  if (mandated_branch_depth != -1)
    branch_depth = mandated_branch_depth;
  if (mandated_fourlib_depth != -1)
    fourlib_depth = mandated_fourlib_depth;
  if (mandated_ko_depth != -1)
    ko_depth = mandated_ko_depth;
  if (mandated_aa_depth != -1)
    aa_depth = mandated_aa_depth;
  if (mandated_owl_distrust_depth != -1)
    owl_distrust_depth = mandated_owl_distrust_depth;
  if (mandated_owl_branch_depth != -1)
    owl_branch_depth = mandated_owl_branch_depth;
  if (mandated_owl_reading_depth != -1)
    owl_reading_depth = mandated_owl_reading_depth;
  if (mandated_owl_node_limit != -1)
    owl_node_limit = mandated_owl_node_limit;
  if (mandated_semeai_node_limit != -1)
    semeai_node_limit = mandated_semeai_node_limit;

  depth_offset = 0;
  
  if (report_levels) {
    fprintf(stderr, "at level %d:\n\n\
depth: %d\n\
branch_depth: %d\n\
backfill_depth: %d\n\
backfill2_depth: %d\n\
break_chain_depth: %d\n\
owl_distrust_depth: %d\n\
owl_branch_depth: %d\n\
owl_reading_depth: %d\n\
aa_depth: %d\n\
ko_depth: %d\n\
fourlib_depth: %d\n\
superstring_depth: %d\n\
owl_node_limit: %d\n\
semeai_branch_depth: %d\n\
semeai_branch_depth2: %d\n\
semeai_node_limit: %d\n\
connect_depth: %d\n\
connect_depth2: %d\n\
connection_node_limit: %d\n\
breakin_depth: %d\n\
breakin_node_limit: %d\n\n",
	    level, depth, branch_depth, backfill_depth, backfill2_depth,
	    break_chain_depth, owl_distrust_depth, owl_branch_depth,
	    owl_reading_depth, aa_depth, ko_depth, fourlib_depth,
	    superstring_depth, owl_node_limit, semeai_branch_depth, 
	    semeai_branch_depth2, semeai_node_limit, connect_depth, 
            connect_depth2, connection_node_limit, breakin_depth, 
	    breakin_node_limit);
  }
}


static int depth_modification = 0;

/*
 * Modify the various tactical reading depth parameters. This is
 * typically used to avoid horizon effects. By temporarily increasing
 * the depth values when trying some move, one can avoid that an
 * irrelevant move seems effective just because the reading hits a
 * depth limit earlier than it did when reading only on relevant
 * moves.
 */

void
modify_depth_values(int n)
{
  depth              += n;
  backfill_depth     += n;
  backfill2_depth    += n;
  break_chain_depth  += n;
  superstring_depth  += n;
  branch_depth       += n;
  fourlib_depth      += n;
  ko_depth           += n;
  breakin_depth	     += n;
  depth_offset       += n;
  depth_modification += n;
}

void
increase_depth_values(void)
{
  modify_depth_values(1);
}

void
decrease_depth_values(void)
{
  modify_depth_values(-1);
}

int
get_depth_modification(void)
{
  return depth_modification;
}


/* Score the game and determine the winner */

void
who_wins(int color, FILE *outfile)
{
  float result;

//TODO
//  silent_examine_position(EXAMINE_DRAGONS);

#if 0
  float white_score;
  float black_score;
  int winner;
#endif

  if (color != BLACK && color != WHITE)
    color = BLACK;

#if 0
  /* Use the aftermath code to compute the final score. (Slower but
   * more reliable.) 
   */
  result = aftermath_compute_score(color, NULL);
  if (result > 0.0)
    winner = WHITE;
  else {
    winner = BLACK;
    result = -result;
  }
#endif

  result = (white_score + black_score)/2.0;
  if (result == 0.0)
    fprintf(outfile, "Result: jigo   ");
  else
    fprintf(outfile, "Result: %c+%.1f   ",
	    (result > 0.0) ? 'W' : 'B', gg_abs(result));
}



/* Find the stones of an extended string, where the extensions are
 * through the following kinds of connections:
 *
 * 1. Solid connections (just like ordinary string).
 *
 *    OO
 *
 * 2. Diagonal connection or one space jump through an intersection
 *    where an opponent move would be suicide or self-atari.
 *
 *    ...
 *    O.O
 *    XOX
 *    X.X
 *
 * 3. Bamboo joint.
 *
 *    OO
 *    ..
 *    OO
 *
 * 4. Diagonal connection where both adjacent intersections are empty.
 *
 *    .O
 *    O.
 *
 * 5. Connection through adjacent or diagonal tactically captured stones.
 *    Connections of this type are omitted when the superstring code is
 *    called from reading.c, but included when the superstring code is
 *    called from owl.c
 */

static void
do_find_superstring(int str, int *num_stones, int *stones,
		    int *num_lib, int *libs, int maxlibs,
		    int *num_adj, int *adjs, int liberty_cap,
		    int proper, int type);

static void
superstring_add_string(int str,
		       int *num_my_stones, int *my_stones,
		       int *num_stones, int *stones,
		       int *num_libs, int *libs, int maxlibs,
		       int *num_adj, int *adjs, int liberty_cap,
		       signed char mx[BOARDMAX],
		       signed char ml[BOARDMAX],
		       signed char ma[BOARDMAX],
		       int do_add);

void
find_superstring(int str, int *num_stones, int *stones)
{
  do_find_superstring(str, num_stones, stones,
		      NULL, NULL, 0,
		      NULL, NULL, 0,
		      0, 1);
}

/* This is the same as find_superstring, except that connections of
 * type 5 are omitted. This is used in semeai analysis.
 */
void
find_superstring_conservative(int str, int *num_stones, int *stones)
{
  do_find_superstring(str, num_stones, stones,
		      NULL, NULL, 0,
		      NULL, NULL, 0,
		      0, 0);
}


/* This function computes the superstring at (str) as described above,
 * but omitting connections of type 5. Then it constructs a list of
 * liberties of the superstring which are not already liberties of
 * (str).
 *
 * If liberty_cap is nonzero, only liberties of substrings of the
 * superstring which have fewer than liberty_cap liberties are
 * generated.
 */

void
find_superstring_liberties(int str,
			   int *num_libs, int *libs, int liberty_cap)
{
  do_find_superstring(str, NULL, NULL,
		      num_libs, libs, MAX_LIBERTIES,
		      NULL, NULL, liberty_cap,
		      0, 0);
}

/* This function is the same as find_superstring_liberties, but it
 * omits those liberties of the string (str), presumably since
 * those have already been treated elsewhere.
 *
 * If liberty_cap is nonzero, only liberties of substrings of the
 * superstring which have at most liberty_cap liberties are
 * generated.
 */

void
find_proper_superstring_liberties(int str, 
				  int *num_libs, int *libs, 
				  int liberty_cap)
{
  do_find_superstring(str, NULL, NULL,
		      num_libs, libs, MAX_LIBERTIES,
		      NULL, NULL, liberty_cap,
		      1, 0);
}

/* This function computes the superstring at (str) as described above,
 * but omitting connections of type 5. Then it constructs a list of
 * liberties of the superstring which are not already liberties of
 * (str).
 *
 * If liberty_cap is nonzero, only liberties of substrings of the
 * superstring which have fewer than liberty_cap liberties are
 * generated.
 */

void
find_superstring_stones_and_liberties(int str,
				      int *num_stones, int *stones,
				      int *num_libs, int *libs,
				      int liberty_cap)
{
  do_find_superstring(str, num_stones, stones,
		      num_libs, libs, MAX_LIBERTIES,
		      NULL, NULL, liberty_cap,
		      0, 0);
}

/* analogous to chainlinks, this function finds boundary chains of the
 * superstring at (str), including those which are boundary chains of
 * (str) itself. If liberty_cap != 0, only those boundary chains with
 * <= liberty_cap liberties are reported.
 */

void
superstring_chainlinks(int str, 
		       int *num_adj, int adjs[MAXCHAIN],
		       int liberty_cap)
{
  do_find_superstring(str, NULL, NULL,
		      NULL, NULL, 0,
		      num_adj, adjs, liberty_cap,
		      0, 2);
}


/* analogous to chainlinks, this function finds boundary chains of the
 * superstring at (str), omitting those which are boundary chains of
 * (str) itself. If liberty_cap != 0, only those boundary chains with
 * <= liberty_cap liberties are reported.
 */

void
proper_superstring_chainlinks(int str,
			      int *num_adj, int adjs[MAXCHAIN],
			      int liberty_cap)
{
  do_find_superstring(str, NULL, NULL,
		      NULL, NULL, 0,
		      num_adj, adjs, liberty_cap,
		      1, 2);
}

/* Do the real work finding the superstring and recording stones,
 * liberties, and/or adjacent strings.
 */
static void
do_find_superstring(int str, int *num_stones, int *stones,
		    int *num_libs, int *libs, int maxlibs,
		    int *num_adj, int *adjs, int liberty_cap,
		    int proper, int type)
{
  int num_my_stones;
  int my_stones[MAX_BOARD * MAX_BOARD];
  
  signed char mx[BOARDMAX]; /* stones */
  signed char ml[BOARDMAX]; /* liberties */
  signed char ma[BOARDMAX]; /* adjacent strings */

  int k, l, r;
  int color = board[str];
  int other = OTHER_COLOR(color);

  memset(mx, 0, sizeof(mx));
  memset(ml, 0, sizeof(ml));
  memset(ma, 0, sizeof(ma));

  if (num_stones)
    *num_stones = 0;
  if (num_libs)
    *num_libs = 0;
  if (num_adj)
    *num_adj = 0;

  /* Include the string itself in the superstring. Only record stones,
   * liberties, and/or adjacent strings if proper==0.
   */
  num_my_stones = 0;
  superstring_add_string(str, &num_my_stones, my_stones,
			 num_stones, stones,
			 num_libs, libs, maxlibs,
			 num_adj, adjs, liberty_cap,
			 mx, ml, ma,
			 !proper);

  /* Loop over all found stones, looking for more strings to include
   * in the superstring. The loop is automatically extended over later
   * found stones as well.
   */
  for (r = 0; r < num_my_stones; r++) {
    int pos = my_stones[r];

    for (k = 0; k < 4; k++) {
      /* List of relative coordinates. (pos) is marked by *.
       *
       *  ef.
       *  gb.
       *  *ac
       *  .d.
       *
       */
      int right = delta[k];
      int up = delta[(k+1)%4];
      
      int apos = pos + right;
      int bpos = pos + right + up;
      int cpos = pos + 2*right;
      int dpos = pos + right - up;
      int epos = pos + 2*up;
      int fpos = pos + right + 2*up;
      int gpos = pos + up;
      int unsafe_move;
      
      if (!ON_BOARD(apos))
	continue;
      
      /* Case 1. Nothing to do since stones are added string by string. */
            
      /* Case 2. */
      if (board[apos] == EMPTY) {
	if (type == 2)
	  unsafe_move = (approxlib(apos, other, 2, NULL) < 2);
	else
	  unsafe_move = is_self_atari(apos, other);
	
	if (unsafe_move && type == 1 && is_ko(apos, other, NULL))
	  unsafe_move = 0;
	
	if (unsafe_move) {
	  if (board[bpos] == color && !mx[bpos])
	    superstring_add_string(bpos, &num_my_stones, my_stones,
				   num_stones, stones,
				   num_libs, libs, maxlibs,
				   num_adj, adjs, liberty_cap,
				   mx, ml, ma, 1);
	  if (board[cpos] == color && !mx[cpos])
	    superstring_add_string(cpos, &num_my_stones, my_stones,
				   num_stones, stones,
				   num_libs, libs, maxlibs,
				   num_adj, adjs, liberty_cap,
				   mx, ml, ma, 1);
	  if (board[dpos] == color && !mx[dpos])
	    superstring_add_string(dpos, &num_my_stones, my_stones,
				   num_stones, stones,
				   num_libs, libs, maxlibs,
				   num_adj, adjs, liberty_cap,
				   mx, ml, ma, 1);
	}
      }
      
      /* Case 3. */
      /* Notice that the order of these tests is significant. We must
       * check bpos before fpos and epos to avoid accessing memory
       * outside the board array. (Notice that fpos is two steps away
       * from pos, which we know is on the board.)
       */
      if (board[apos] == color && board[bpos] == EMPTY
	  && board[fpos] == color && board[epos] == color && !mx[epos]
	  && board[gpos] == EMPTY)
	superstring_add_string(epos, &num_my_stones, my_stones,
			       num_stones, stones,
			       num_libs, libs, maxlibs,
			       num_adj, adjs, liberty_cap,
			       mx, ml, ma, 1);
      /* Don't bother with f, it is part of the string just added. */
      
      /* Case 4. */
      if (board[bpos] == color && !mx[bpos]
	  && board[apos] == EMPTY && board[gpos] == EMPTY)
	superstring_add_string(bpos, &num_my_stones, my_stones,
			       num_stones, stones,
			       num_libs, libs, maxlibs,
			       num_adj, adjs, liberty_cap,
			       mx, ml, ma, 1);
      
      /* Case 5. */
      if (type == 1)
	for (l = 0; l < 2; l++) {
	  int upos;
	  
	  if (l == 0) {
	    /* adjacent lunch */
	    upos = apos;
	  }
	  else {
	    /* diagonal lunch */
	    upos = bpos;
	  }
	  
	  if (board[upos] != other)
	    continue;
	  
	  upos = find_origin(upos);
	  
	  /* Only do the reading once. */
	  if (mx[upos] == 1)
	    continue;
	  
	  mx[upos] = 1;
	  
	  if (attack(upos, NULL)
	      && !find_defense(upos, NULL)) {
	    int lunch_stones[MAX_BOARD*MAX_BOARD];
	    int num_lunch_stones = findstones(upos, MAX_BOARD*MAX_BOARD,
					      lunch_stones);
	    int m, n;
	    for (m = 0; m < num_lunch_stones; m++)
	      for (n = 0; n < 8; n++) {
		int vpos = lunch_stones[m] + delta[n];
		if (board[vpos] == color && !mx[vpos])
		  superstring_add_string(vpos,
					 &num_my_stones, my_stones,
					 num_stones, stones,
					 num_libs, libs, maxlibs,
					 num_adj, adjs, liberty_cap,
					 mx, ml, ma, 1);
	      }
	  }
	}
      if (num_libs && maxlibs > 0 && *num_libs >= maxlibs)
	return;
    }
  }
}

/* Add a new string to a superstring. Record stones, liberties, and
 * adjacent strings as asked for.
 */
static void
superstring_add_string(int str,
		       int *num_my_stones, int *my_stones,
		       int *num_stones, int *stones,
		       int *num_libs, int *libs, int maxlibs,
		       int *num_adj, int *adjs, int liberty_cap,
		       signed char mx[BOARDMAX],
		       signed char ml[BOARDMAX],
		       signed char ma[BOARDMAX],
		       int do_add)
{
  int num_my_libs;
  int my_libs[MAXLIBS];
  int num_my_adj;
  int my_adjs[MAXCHAIN];
  int new_stones;
  int k;
  
  ASSERT1(mx[str] == 0, str);

  /* Pick up the stones of the new string. */
  new_stones = findstones(str, board_size * board_size,
			  &(my_stones[*num_my_stones]));
  
  mark_string(str, mx, 1);
  if (stones) {
    gg_assert(num_stones);
    for (k = 0; k < new_stones; k++) {
      if (do_add) {
	stones[*num_stones] = my_stones[*num_my_stones + k];
	(*num_stones)++;
      }
    }
  }
  (*num_my_stones) += new_stones;

  /* Pick up the liberties of the new string. */
  if (libs) {
    gg_assert(num_libs);
    /* Get the liberties of the string. */
    num_my_libs = findlib(str, MAXLIBS, my_libs);

    /* Remove this string from the superstring if it has too many
     * liberties.
     */
    if (liberty_cap > 0 && num_my_libs > liberty_cap)
      (*num_my_stones) -= new_stones;

    for (k = 0; k < num_my_libs; k++) {
      if (ml[my_libs[k]])
	continue;
      ml[my_libs[k]] = 1;
      if (do_add && (liberty_cap == 0 || num_my_libs <= liberty_cap)) {
	libs[*num_libs] = my_libs[k];
	(*num_libs)++;
	if (*num_libs == maxlibs)
	  break;
      }
    }
  }

  /* Pick up adjacent strings to the new string. */
  if (adjs) {
    gg_assert(num_adj);
    num_my_adj = chainlinks(str, my_adjs);
    for (k = 0; k < num_my_adj; k++) {
      if (liberty_cap > 0 && countlib(my_adjs[k]) > liberty_cap)
	continue;
      if (ma[my_adjs[k]])
	continue;
      ma[my_adjs[k]] = 1;
      if (do_add) {
	adjs[*num_adj] = my_adjs[k];
	(*num_adj)++;
      }
    }
  }
}

/* Internal timers for assessing time spent on various tasks. */
#define NUMBER_OF_TIMERS 4
static double timers[NUMBER_OF_TIMERS];

/* Start a timer. */
void
start_timer(int n)
{
  gg_assert(n >= 0 && n < NUMBER_OF_TIMERS);
  if (!showtime)
    return;

  timers[n] = gg_cputime();
}

/* Report time spent and restart the timer. Make no report if elapsed
 * time is less than mintime.
 */
double
time_report(int n, const char *occupation, int move, double mintime)
{
  double t;
  double dt;
  gg_assert(n >= 0 && n < NUMBER_OF_TIMERS);

  if (!showtime)
    return 0.0;

  t = gg_cputime();
  dt = t - timers[n];
  if (dt > mintime) {
    gprintf("%s", occupation);
    if (move != NO_MOVE)
      gprintf("%1m", move);
    fprintf(stderr, ": %.2f sec\n", dt);
  }
  timers[n] = t;
  return dt;
}

void
clearstats()
{
  stats.nodes                    = 0;
  stats.read_result_entered      = 0;
  stats.read_result_hits         = 0;
  stats.trusted_read_result_hits = 0;
}
  
void
showstats()
{
  gprintf("Nodes:                    %d\n", stats.nodes);
  gprintf("Read results entered:     %d\n", stats.read_result_entered);
  gprintf("Read result hits:         %d\n", stats.read_result_hits);
  gprintf("Trusted read result hits: %d\n", stats.trusted_read_result_hits);
}


/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
