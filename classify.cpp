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
  vector<LasPoint> upPoints,downPoints,sphPoints;
  Paraboloid downward,upward;
  Sphere sphere;
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
    int i,sector;
    for (i=0;i<cylPoints.size();i++)
    {
    }
    snake.countNonempty();
    tileMutex.lock();
    tileMutex.unlock();
  }
  octStore.disown();
}
