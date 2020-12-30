/******************************************************/
/*                                                    */
/* tile.cpp - tiles                                   */
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
#include <climits>
#include "tile.h"
#include "octree.h"
using namespace std;

harray<Tile> tiles;
mutex tileMutex;
Tile minTile,maxTile;

void initTiles()
{
  minTile.nPoints=INT_MAX;
  maxTile.nPoints=0;
  minTile.density=INFINITY;
  maxTile.density=0;
  minTile.paraboloidSize=INFINITY;
  maxTile.paraboloidSize=0;
}
