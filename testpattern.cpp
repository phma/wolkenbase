/******************************************************/
/*                                                    */
/* testpattern.cpp - test patterns                    */
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
#include <cmath>
#include "testpattern.h"
using namespace std;

/* Terrain with street intersection:
 * 100 m diameter, with two 15 m wide streets intersecting at right angles.
 * Power poles with wires between them. Eventually, forbs and trees.
 */

double street(double x)
// Cross section of a street 15 m wide
{
  double elev;
  x=fabs(x);
  if (x<7.2)
    elev=-x/50;
  else if (x<7.4)
    elev=-0.15;
  else
    elev=0;
  return elev;
}

vector<xyz> diskSpatter(xyz center,xyz normal,double radius,double density)
/* Places dots evenly in a disk. If the area times the density is less than one,
 * may return an empty vector. Used for terrain and leaves.
 */
{
  vector<xyz> ret;
  return ret;
}

vector<xyz> cylinderSpatter(xyz begin,xyz end,double radius,double density)
/* Places dots evenly on a cylinder. Used for tree stems and power poles.
 */
{
  vector<xyz> ret;
  return ret;
}

vector<xyz> catenarySpatter(xyz vertex,double scale,int bearing,double radius,double density)
/* Places dots evenly on a catenary, except that they're slightly more dense
 * on the inside of the bend. Used for power lines.
 */
{
  vector<xyz> ret;
  return ret;
}
