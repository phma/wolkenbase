/******************************************************/
/*                                                    */
/* octree.h - octrees                                 */
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

#include "las.h"
#define LASPOINT_SIZE 87
#define RECORDS 753
#define BLOCKSIZE 65536
/* If a future version of LAS adds more fields, making the size of LasPoint
 * on disk bigger, these should be changed so that LASPOINT_SIZE*RECORDS is
 * equal to or slightly smaller than BLOCKSIZE, which is a power of 2, and
 * 100<=RECORDS<=1000.
 */

class Octree
{
  xyz center;
  double side;
  uintptr_t sub[8]; // Even means Octree *; odd means a disk block.
  long long findBlock(xyz pnt);
};
