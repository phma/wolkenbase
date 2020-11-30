/******************************************************/
/*                                                    */
/* peano.cpp - Peano curve                            */
/*                                                    */
/******************************************************/
/* Copyright 2020 Pierre Abbat.
 * This file is part of Wolkenbase.
 *
 * Wolkenbase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wolkenbase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wolkenbase. If not, see <http://www.gnu.org/licenses/>.
 */
#include "peano.h"
using namespace std;

/* Wolkenbase uses the base-3 Peano curve to update the window
 * while reading files and constructing the octree.
 */

array<int,3> peanoPoint(int width,int height,unsigned phase,int direction)
/* ret[0] and ret[1] are x and y; ret[2] is how much to add to phase
 * to get the next point. Sometimes (up to 3/7 of the time), it returns
 * (-1,-1,h), which means that you have to skip this point because it split
 * a range of length 2 into (1,0,1), and this point is the range of length 0.
 *
 * Directions are:
 * 0: Bottom left to top right, landscape
 * 1: Bottom right to top left, landscape
 * 2: Top left to bottom right, landscape
 * 3: Top right to bottom left, landscape
 * 4: Bottom left to top right, portrait
 * 5: Bottom right to top left, portrait
 * 6: Top left to bottom right, portrait
 * 7: Top right to bottom left, portrait.
 *
 * This function may behave erratically if width or height is
 * greater than 59049.
 */
{
  int part1,part2,partbase=0,partsize;
  unsigned partphase=0;
  array<int,3> ret;
  if (width>height)
    direction&=3;
  if (width<height)
    direction|=4;
  if (phase>=THREE19)
    partphase=THREE19;
  if (phase>=2*THREE19)
    partphase=2*THREE19;
  if (width==0 || height==0)
  {
    ret[0]=-1;
    ret[1]=-1;
    ret[2]=THREE20;
  }
  else if (width==1 && height==1)
  {
    ret[0]=0;
    ret[1]=0;
    ret[2]=THREE20;
  }
  else if (direction&4) // portrait
  {
    part1=(height+1)/3;
    part2=(2*height+1)/3;
    switch (partphase)
    {
      case 0:
	partbase=(direction&2)?part2:0;
	partsize=part1;
	break;
      case THREE19:
	partbase=part1;
	partsize=part2-part1;
	direction^=1;
	break;
      case 2*THREE19:
	partbase=(direction&2)?0:part2;
	partsize=part1;
	break;
    }
    ret=peanoPoint(width,partsize,3*(phase-partphase),direction^4);
    if (ret[1]>=0)
      ret[1]+=partbase;
    ret[2]=(unsigned)ret[2]/3;
  }
  else // landscape
  {
    part1=(width+1)/3;
    part2=(2*width+1)/3;
    switch (partphase)
    {
      case 0:
	partbase=(direction&1)?part2:0;
	partsize=part1;
	break;
      case THREE19:
	partbase=part1;
	partsize=part2-part1;
	direction^=2;
	break;
      case 2*THREE19:
	partbase=(direction&1)?0:part2;
	partsize=part1;
	break;
    }
    ret=peanoPoint(partsize,height,3*(phase-partphase),direction^4);
    if (ret[0]>=0)
      ret[0]+=partbase;
    ret[2]=(unsigned)ret[2]/3;
  }
  return ret;
}
