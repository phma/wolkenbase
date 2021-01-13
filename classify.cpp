/******************************************************/
/*                                                    */
/* classify.cpp - classify points                     */
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

#include <climits>
#include <ctime>
#include <set>
#include "classify.h"
#include "octree.h"
#include "angle.h"
#include "threads.h"
#include "relprime.h"
#include "leastsquares.h"
using namespace std;
namespace cr=std::chrono;

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

bool surround(set<int> &directions)
/* Returns true if none of the angles between successive directions is greater
 * than 72°. There must be at least 6 directions.
 */
{
  int n=0,first,last,penult;
  int parity=time(nullptr)&1;
  bool ret=directions.size()>5;
  set<int>::iterator i;
  vector<int> delenda;
  for (i=directions.begin();i!=directions.end();++i,++n)
  {
    if (i==directions.begin())
      first=*i;
    else
      if (((*i-last)&INT_MAX)>=DEG72)
	ret=false;
    if ((n&1)==parity && n>1 && ((*i-last)&INT_MAX)<DEG30 && ((last-penult)&INT_MAX)<DEG30)
      delenda.push_back(last);
    penult=last;
    last=*i;
  }
  if (((first-last)&INT_MAX)>=DEG72)
    ret=false;
  for (n=0;n<delenda.size();n++)
    directions.erase(delenda[n]);
  return ret;
}

void classifyCylinder(Eisenstein cylAddress)
{
  Cylinder cyl=snake.cyl(cylAddress);
  vector<LasPoint> cylPoints=octStore.pointsIn(cyl);
  vector<LasPoint> upPoints,downPoints,sphPoints;
  Paraboloid downward,upward;
  Sphere sphere;
  Tile *thisTile;
  if (cylPoints.size())
  {
    /* Classifying a cylinder (which circumscribes a hexagonal tile) is done
     * like this:
     * • For each point in the cylinder, construct a downward-facing paraboloid
     *   with the point as vertex. Also construct an upward-facing paraboloid
     *   and a sphere.
     * • For each point in the downward paraboloid, if it's not on the axis,
     *   compute its bearing from the axis.
     * • If six of the points surround the axis, and the angles between them
     *   are less than 72°, then the point is off ground.
     * • If the point is not off ground, check for points, other than the point
     *   itself, in the upward paraboloid and the sphere. If there is at least
     *   one point in the upward paraboloid but none in the sphere, the point
     *   is low noise.
     * • Otherwise it is ground.
     */
    int i,j,h,sz,n;
    bool aboveGround,isTreeTile=false;
    cr::time_point<cr::steady_clock> timeStart=clk.now();
    tileMutex.lock();
    thisTile=&tiles[cylAddress];
    tileMutex.unlock();
    if (cylPoints.size()>=343 && thisTile->paraboloidSize>cyl.getRadius())
      isTreeTile=true;
    for (i=0;i<cylPoints.size();i++)
    {
      set<int> directions;
      aboveGround=false;
      downward=Paraboloid(cylPoints[i].location,thisTile->paraboloidSize);
      downPoints=octStore.pointsIn(downward);
      sz=downPoints.size();
      h=relprime(sz);
      for (j=n=0;j<sz && !aboveGround;j++,n=(n+h)%sz)
      {
	if (dist(xy(cylPoints[i].location),xy(downPoints[n].location)))
	  directions.insert(dir(xy(cylPoints[i].location),xy(downPoints[n].location)));
	if (surround(directions))
	  aboveGround=true;
      }
      if (aboveGround)
	cylPoints[i].classification=1;
      else
      {
	cylPoints[i].classification=2;
	thisTile->nGround++;
      }
      octStore.put(cylPoints[i]);
    }
    cr::nanoseconds elapsed=clk.now()-timeStart;
    if (isTreeTile)
      cout<<"Classifying "<<cylPoints.size()<<" points took "<<elapsed.count()*1e-6<<" ms\n";
    snake.countNonempty();
  }
  octStore.disown();
}
