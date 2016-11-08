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

#define _GNU_SOURCE

#include "gnugo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "liberty.h"
#include "sgftree.h"
#include "gg_utils.h"

/* Return one if x doesn't equal position_number and 0 otherwise.
 * After using this macro x will always have the value
 * position_number.
 */
#define NEEDS_UPDATE(x) (x != position_number ? (x = position_number, 1) : 0)

int dclto;
int dclfr;
int dclpid = 0;

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
  if(dclpid == 0)
  	deepcl_init();
  else
  {
  	wait(dclpid);
	deepcl_init();
  }
}



int
deepcl_init()
{
	int _dclto[2],_dclfr[2];
	int dim[3];
	if(pipe2(_dclto,O_DIRECT))
		return -1;
	if(pipe2(_dclfr,O_DIRECT))
		return -1;
	dclpid = fork();
	switch(dclpid)
	{
		case -1:
			return -1;
		case 0:
			close(_dclto[1]);
			close(_dclfr[0]);
			dup2(_dclto[0],STDIN_FILENO);
			close(_dclto[0]);
			dup2(_dclfr[1],STDOUT_FILENO);
			close(_dclfr[1]);
			execlp("deepcl_predict", "weightsfile=/home/thomas/deepcl/kgsgo/go_weights.dat", "batchsize=1", "outputformat=binary", (char*) NULL);
			return -1;
		default:
			close(_dclto[0]);
			close(_dclfr[1]);
			dclto=_dclto[1];
			dclfr=_dclfr[0];
			dim[0]=8; //numplanes
			dim[1]=19; //numplanes
			dim[2]=19; //numplanes
			write(dclto,(char*)dim,3*4l);
			return 0;
	}
}


/* 
 * Get probabilities from NN.
 *
 * Return probabilities.
 */

int
deepcl_get_probs(float* probs,Intersection* lboard, int color )
{
	float b[19*19*8] = { 0 };
	int i,j,pos,libs =3;
//	memset(b,0,sizeof(float)*19*19*8);
	for(i=0;i<19;i++)
	{
		for(j=0;j<19;j++)
		{
			pos = POS(i, j);
			b[i*19*8+j*8+7]=255;
			if(board[pos]==color)
			{
				libs = countlib(pos);
				if(libs == 1)
					b[i*19*8+j*8]=255;
				else if(libs == 1)
					b[i*19*8+j*8+1]=255;
				else
					b[i*19*8+j*8+2]=255;
			}
			else if(board[pos]==OTHER_COLOR(color))
			{
				libs = countlib(pos);
				if(libs == 1)
					b[i*19*8+j*8+3]=255;
				else if(libs == 1)
					b[i*19*8+j*8+4]=255;
				else
					b[i*19*8+j*8+5]=255;
			}
			else if(pos == board_ko_pos )
				b[i*19*8+j*8+6]=255;


		}
	}

	write(dclto,(char*)b,19*19*8*4l);
	printf("toto\n");
	read(dclto,probs,19*19*4l);

	return 0;
}



/* 
 * Get probabilities from NN.
 *
 * Return probabilities.
 */

int
weighted_rand_move(float* probs)
{
	double sum = 0;
	int i;
	float prob;

	for(i=0;i<19*19;i++)
	{
		if(probs[i] != probs[i] || probs[i] < 0)
			probs[i] = 0;
		sum += probs[i];
	}

	prob = (float)rand()*sum/(RAND_MAX);
	printf("prob: %1.0e, sum: %1.0e\n",prob, sum);
	sum=0;
	for(i=0;i<19*19;i++)
	{
		sum += probs[i];
		if(prob<sum)
			return POS(i/19,i%19);
	}
	return -1;
}

/* 
 * Generate computer move for color.
 *
 * Return the generated move.
 */

int
genmove(int color, float *value, int *resign)
{
  int move = PASS_MOVE;
  int tmp_move, i;
  float probs[19*19];
  if (resign)
    *resign = 0;

  deepcl_get_probs(probs, board, color);

  for(i=0;i<10;i++)
  {
    tmp_move = weighted_rand_move(probs);
  printf("genmove round %i, move %i\n",i,tmp_move);
    if(ON_BOARD(tmp_move) && (board[tmp_move] == EMPTY) && (tmp_move != -1))
    {
      if(is_allowed_move(tmp_move,color))
      {
        move = tmp_move;
	break;
      }
    }
  }

  gg_assert(move == PASS_MOVE || ON_BOARD(move));
  return move;
}




/*
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * End:
 */
