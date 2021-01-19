/******************************************************/
/*                                                    */
/* tile.h - tiles                                     */
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
#ifndef TILE_H
#define TILE_H
#include "flowsnake.h"
#include "threads.h"

class Tile
{
public:
  int nPoints,nGround;
  short roofFlags,treeFlags;
  double density; // of bottom layer
  double paraboloidSize; // radius of curvature
  double height; // after untilting
};

extern harray<Tile> tiles;
extern std::mutex tileMutex;
extern Tile minTile,maxTile;

void initTiles();
#endif
