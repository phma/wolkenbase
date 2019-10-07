/******************************************************/
/*                                                    */
/* shape.cpp - shapes                                 */
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
#include "shape.h"
#include "angle.h"

Cube::Cube()
{
  side=0;
}

Cube::Cube(xyz c,double s)
{
  side=s;
  center=c;
}

bool Cube::in(xyz pnt)
{
  return fabs(pnt.getx()-center.getx())<=side/2
      && fabs(pnt.gety()-center.gety())<=side/2
      && fabs(pnt.getz()-center.getz())<=side/2;
}

Paraboloid::Paraboloid()
{
  radiusCurvature=0;
}

Paraboloid::Paraboloid(xyz v,double r)
{
  vertex=v;
  radiusCurvature=r;
}

bool Paraboloid::in(xyz pnt)
{
  double xydist=dist(xy(vertex),xy(pnt));
  double zdist=vertex.getz()-pnt.getz(); // so because opens downward
  if (radiusCurvature==0)
    return xydist==0;
  else
    return 2*zdist/radiusCurvature>=sqr(xydist/radiusCurvature);
}

bool Paraboloid::intersectCube(Cube cube)
{
  return false; //stub
}
