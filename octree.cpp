/******************************************************/
/*                                                    */
/* octree.cpp - octrees                               */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
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
#include "octree.h"

long long Octree::findBlock(xyz pnt)
// Returns the disk block number that contains pnt, or -1 if none.
{
  int xbit,ybit,zbit,i;
  xbit=pnt.getx()>=center.getx();
  ybit=pnt.gety()>=center.gety();
  zbit=pnt.getz()>=center.getz();
  i=zbit*4+ybit*2+xbit;
  if (sub[i]==0)
    return -1;
  else if (sub[i]&1)
    return sub[i]>>1;
  else
    return ((Octree *)sub[i])->findBlock(pnt);
}
