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
#include <cassert>
#include "testpattern.h"
#include "angle.h"
using namespace std;

unsigned int areaPhase;
int anglePhase;

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

Quaternion rotateTo(xyz normal)
/* Returns a quaternion which rotates a vertical line to normal.
 * If normal points straight down, it returns the identity.
 */
{
  double len=normal.length();
  xyz midway;
  assert(len);
  normal/=len;
  midway=normal+xyz(0,0,1);
  if (midway.length())
    return versor(midway,DEG180);
  else
    return Quaternion(1);
}

vector<xyz> diskSpatter(xyz center,xyz normal,double radius,double density)
/* Places dots evenly in a disk. If the area times the density is less than one,
 * may return an empty vector. Used for terrain and leaves.
 */
{
  vector<xyz> ret;
  xyz dot;
  double dotRadius;
  Quaternion ro=rotateTo(normal);
  double nDots=2*M_PI*sqr(radius)*density;
  long long nDotsFixed=llrintl(4294967296e0*nDots);
  while (nDotsFixed>=4294967296 || nDotsFixed>=areaPhase)
  {
    if (nDotsFixed>=areaPhase)
    {
      nDotsFixed-=areaPhase;
      areaPhase=0;
    }
    else if (nDotsFixed>=4294967296)
      nDotsFixed-=4294967296;
    dotRadius=sqrt(nDotsFixed/4294967296e0/2/M_PI/density);
    dot=ro.rotate(xyz(cossin(anglePhase)*dotRadius,0))+center;
    anglePhase+=PHITURN;
    ret.push_back(dot);
  }
  areaPhase-=nDotsFixed;
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
