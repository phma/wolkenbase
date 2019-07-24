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
 *    of RECORDS points and building an octree index.
 * 2. Scan the point cloud's xy projection in Hilbert curve point sequences:
 *    1 point, 4 points, 16 points, etc., looking at cylinders with radius
 *    0.5 meter.
 * 3. Find the lowest point near the axis of the cylinder and examine a sphere
 *    of radius 0.5 meter.
 */
#include <cmath>
#include "las.h"
#include "octree.h"
using namespace std;

int main(int argc,char **argv)
{
  LasPoint lPoint;
  int i;
  vector<LasHeader> files;
  ofstream testFile("testfile");
  files.resize(1);
  files[0].open("las file export.las");
  cout<<files[0].numberPoints()<<" points\n";
  lPoint.location=xyz(M_PI,exp(1),sqrt(2));
  for (i=0;i<RECORDS;i++)
    lPoint.write(testFile);
  return 0;
}
