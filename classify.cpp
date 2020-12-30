/******************************************************/
/*                                                    */
/* classify.cpp - classify points                     */
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

#include "classify.h"
#include "octree.h"
#include "angle.h"
#include "leastsquares.h"
using namespace std;

/* Point classes:
 * 0	Created, never classified
 * 1	Unclassified (here: not ground, but could be tree, power line, whatever)
 * 2	Ground
 * 3	Low vegetation
 * 4	Medium vegetation
 * 5	High vegetation
 * 6	Building
 * 7	Low point (noise)
 * 8	Reserved
 * 9	Water
 * 10	Rail
 * 11	Road surface
 * 12	Reserved
 * 13	Wire - Guard
 * 14	Wire - Conductor
 * 15	Transmission tower
 * 16	Wire-structure connector
 * 17	Bridge deck
 * 18	High noise (e.g. bird)
 * 19	Reserved
 * :63	Reserved
 * 64	User definable
 * :255	User definable
 * Classes 32-255 cannot appear in formats 1-5.
 */

void classifyCylinder(Eisenstein cylAddress)
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
     * 4. Discard all points more than 2r higher than the second bottom point.
     * 5. Split the cylinder into seven equal parts, a central cylinder and six 60° sectors.
     * 6. Compute the RMS of the seven densities.
     * The reason for using the second bottom point is that occasionally, there is
     * a stray point below ground. If it's far enough below ground, it would result
     * in the density being only one point in the tile.
     */
    matrix a(cylPoints.size(),3);
    vector<double> b,slopev;
    vector<xyz> pnts,pntsUntilted,pntsBottom;
    xy slope;
    double bottom=INFINITY,bottom2=INFINITY;
    double density=0,paraboloidSize=0;
    int i,sector;
    int histo[7];
    for (i=0;i<cylPoints.size();i++)
    {
      pnts.push_back(cylPoints[i].location-xyz(cyl.getCenter(),0));
      a[i][0]=pnts[i].getx();
      a[i][1]=pnts[i].gety();
      a[i][2]=1;
      b.push_back(pnts[i].getz());
    }
    slopev=linearLeastSquares(a,b);
    slope=xy(slopev[0],slopev[1]);
    if (slope.length()>1)
      slope/=slope.length();
    if (slope.isnan())
      slope=xy(0,0);
    for (i=0;i<pnts.size();i++)
    {
      double z=dot(slope,xy(pnts[i]));
      pntsUntilted.push_back(xyz(xy(pnts[i]),pnts[i].getz()-z));
      if (pntsUntilted[i].getz()<bottom)
      {
	bottom2=bottom;
	bottom=pntsUntilted[i].getz();
      }
    }
    if (isinf(bottom2))
      bottom2=bottom;
    for (i=0;i<pntsUntilted.size();i++)
      if (pntsUntilted[i].getz()<bottom2+2*cyl.getRadius())
	pntsBottom.push_back(pntsUntilted[i]);
    for (i=0;i<7;i++)
      histo[i]=0;
    for (i=0;i<pntsBottom.size();i++)
    {
      sector=lrint(atan2(pntsBottom[i].gety(),pntsBottom[i].getx())*3/M_PI);
      if (sector<0)
	sector+=6;
      sector=(sector%6)+1;
      histo[sector]++;
    }
    for (i=0;i<7;i++)
      density+=sqr(histo[i]);
    paraboloidSize=1/sqrt(density); // this may need to be multiplied by something
    snake.countNonempty();
    tileMutex.lock();
    tiles[cylAddress].nPoints=cylPoints.size();
    tiles[cylAddress].density=density;
    tiles[cylAddress].paraboloidSize=paraboloidSize;
    if (tiles[cylAddress].nPoints>maxTile.nPoints)
      maxTile.nPoints=tiles[cylAddress].nPoints;
    if (tiles[cylAddress].nPoints<minTile.nPoints)
      minTile.nPoints=tiles[cylAddress].nPoints;
    if (tiles[cylAddress].density>maxTile.density)
      maxTile.density=tiles[cylAddress].density;
    if (tiles[cylAddress].density<minTile.density)
      minTile.density=tiles[cylAddress].density;
    if (tiles[cylAddress].paraboloidSize>maxTile.paraboloidSize)
      maxTile.paraboloidSize=tiles[cylAddress].paraboloidSize;
    if (tiles[cylAddress].paraboloidSize<minTile.paraboloidSize)
      minTile.paraboloidSize=tiles[cylAddress].paraboloidSize;
    tileMutex.unlock();
  }
  octStore.disown();
}
