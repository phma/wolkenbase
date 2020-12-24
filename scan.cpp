/******************************************************/
/*                                                    */
/* scan.cpp - scan the cloud                          */
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

#include "scan.h"
#include "octree.h"
#include "leastsquares.h"
using namespace std;

void scanCylinder(Eisenstein cylAddress)
{
  Cylinder cyl=snake.cyl(cylAddress);
  vector<LasPoint> cylPoints=octStore.pointsIn(cyl);
  if (cylPoints.size())
  {
    /* Scanning a cylinder (which circumscribes a hexagonal tile) consists
     * of two parts:
     * • Find the density of points in the bottom layer.
     * • Check if there's an edge of roof (see roof.cpp).
     * To find the bottom density:
     * 1. Using least squares, find a plane through the centroid of the points.
     * 2. Clamp the slope of this plane to 1. (Steep slope can be caused by trees on the side.)
     * 3. Subtract the plane from the points.
     * 4. Discard all points more than 2r higher than the bottom point.
     * 5. Split the cylinder into seven equal parts, a central cylinder and six 60° sectors.
     * 6. Compute the RMS of the seven densities.
     */
    matrix a(cylPoints.size(),2);
    vector<double> b,slopev;
    vector<xyz> pnts,pntsUntilted;
    int i;
    for (i=0;i<cylPoints.size();i++)
    {
      pnts.push_back(cylPoints[i].location-xyz(cyl.getCenter(),0));
      a[i][0]=pnts[i].getx();
      a[i][1]=pnts[i].gety();
      b.push_back(pnts[i].getz());
    }
    slopev=linearLeastSquares(a,b);
    tileMutex.lock();
    tiles[cylAddress].nPoints=cylPoints.size();
    if (tiles[cylAddress].nPoints>maxTile.nPoints)
      maxTile.nPoints=tiles[cylAddress].nPoints;
    if (tiles[cylAddress].nPoints<minTile.nPoints)
      minTile.nPoints=tiles[cylAddress].nPoints;
    tileMutex.unlock();
  }
  octStore.disown();
}
