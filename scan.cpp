/******************************************************/
/*                                                    */
/* scan.cpp - scan the cloud                          */
/*                                                    */
/******************************************************/
/* Copyright 2020,2021 Pierre Abbat.
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
#include "angle.h"
#include "leastsquares.h"
using namespace std;

double minHyperboloidSize,maxSlope;

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
    double bottom=INFINITY,bottom2=INFINITY,top=-INFINITY;
    double density=0,hyperboloidSize=0;
    int i,sector,treeFlags=0;
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
      if (pntsUntilted[i].getz()>top)
	top=pntsUntilted[i].getz();
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
      if (xy(pntsBottom[i]).length()<cyl.getRadius()/M_SQRT7)
	sector=0;
      histo[sector]++;
    }
    for (i=0;i<7;i++)
      density+=sqr(histo[i]);
    if (pntsUntilted.size()>pntsBottom.size() && density<7)
      treeFlags=1;
    density=sqrt(density)*M_SQRT7/sqr(cyl.getRadius())/M_PI;
    if (pntsUntilted.size()>pntsBottom.size() && density<0.5)
      treeFlags=1;
    if (top-bottom>1.5)
      treeFlags=1;
    hyperboloidSize=sqrt(1/density+sqr(minHyperboloidSize)); // this may need to be multiplied by something
    snake.countNonempty();
    tileMutex.lock();
    tiles[cylAddress].nPoints=cylPoints.size();
    tiles[cylAddress].nGround=0;
    tiles[cylAddress].density=density;
    tiles[cylAddress].treeFlags=treeFlags;
    tiles[cylAddress].hyperboloidSize=hyperboloidSize;
    tiles[cylAddress].height=top-bottom;
    if (tiles[cylAddress].nPoints>maxTile.nPoints)
      maxTile.nPoints=tiles[cylAddress].nPoints;
    if (tiles[cylAddress].nPoints<minTile.nPoints)
      minTile.nPoints=tiles[cylAddress].nPoints;
    if (tiles[cylAddress].nGround>maxTile.nGround)
      maxTile.nGround=tiles[cylAddress].nGround;
    if (tiles[cylAddress].nGround<minTile.nGround)
      minTile.nGround=tiles[cylAddress].nGround;
    if (tiles[cylAddress].density>maxTile.density)
      maxTile.density=tiles[cylAddress].density;
    if (tiles[cylAddress].density<minTile.density)
      minTile.density=tiles[cylAddress].density;
    if (tiles[cylAddress].hyperboloidSize>maxTile.hyperboloidSize)
      maxTile.hyperboloidSize=tiles[cylAddress].hyperboloidSize;
    if (tiles[cylAddress].hyperboloidSize<minTile.hyperboloidSize)
      minTile.hyperboloidSize=tiles[cylAddress].hyperboloidSize;
    tileMutex.unlock();
  }
  octStore.disown();
}

void postscanCylinder(Eisenstein cylAddress)
{
  bool isalloc;
  tileMutex.lock();
  isalloc=tiles.count(cylAddress);
  tileMutex.unlock();
  if (isalloc)
  {
    Tile *thisTile;
    tileMutex.lock();
    thisTile=&tiles[cylAddress];
    tileMutex.unlock();
    if (thisTile->nPoints)
    {
      /* Look at the tiles along six rays emanating from this tile.
       * Stop when all six are empty or any one is full, but not a tree tile.
       */
      int i=1,j,nontree,ringcount,count=0;
      do
      {
	tileMutex.lock();
	for (nontree=ringcount=j=0;j<6 && (thisTile->treeFlags&1);j++)
	  if (tiles[cylAddress+root1[j]*i].nPoints)
	  {
	    ringcount++;
	    if (tiles[cylAddress+root1[j]*i].treeFlags&1)
	      count++;
	    else
	      nontree++;
	  }
	tileMutex.unlock();
	++i;
      } while (ringcount && !nontree);
      thisTile->hyperboloidSize=sqrt(sqr(thisTile->hyperboloidSize)+sqr(count*snake.getSpacing()/6));
      snake.countNonempty();
    }
  }
}
