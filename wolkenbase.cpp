/******************************************************/
/*                                                    */
/* wolkenbase.cpp - main program                      */
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
/* This program cleans a point cloud as follows:
 * 1. Read in the point cloud, storing it in a temporary file in blocks
 *    of 753 points and building an octree index.
 * 2. Scan the point cloud's xy projection in Hilbert curve point sequences:
 *    1 point, 4 points, 16 points, etc., looking at cylinders with radius
 *    0.5 meter.
 * 3. Find the lowest point near the axis of the cylinder and examine a sphere
 *    of radius 0.5 meter.
 */

int main(int argc,char **argv)
{
  return 0;
}
