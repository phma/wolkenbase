/******************************************************/
/*                                                    */
/* shape.cpp - shapes                                 */
/*                                                    */
/******************************************************/
/* Copyright 2019,2020 Pierre Abbat.
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

bool Shape::intersect(Cube cube)
{
  return in(closestPoint(cube));
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

xyz Paraboloid::closestPoint(Cube cube)
{
  xyz ret=cube.getCenter();
  double x=ret.getx(),y=ret.gety(),z=ret.getz();
  if (fabs(vertex.getx()-x)<cube.getSide()/2)
    x=vertex.getx();
  else if (vertex.getx()>x)
    x+=cube.getSide()/2;
  else
    x-=cube.getSide()/2;
  if (fabs(vertex.gety()-y)<cube.getSide()/2)
    y=vertex.gety();
  else if (vertex.gety()>y)
    y+=cube.getSide()/2;
  else
    y-=cube.getSide()/2;
  if (radiusCurvature>0)
    z-=cube.getSide()/2;
  if (radiusCurvature<0)
    z+=cube.getSide()/2;
  ret=xyz(x,y,z);
  return ret;
}

Cylinder::Cylinder()
{
  radius=0;
}

Cylinder::Cylinder(xy c,double r)
{
  center=c;
  radius=r;
}

bool Cylinder::in(xyz pnt)
{
  double xydist=dist(center,xy(pnt));
  return xydist<=radius;
}

xyz Cylinder::closestPoint(Cube cube)
{
  xyz ret=cube.getCenter();
  double x=ret.getx(),y=ret.gety(),z=ret.getz();
  if (fabs(center.getx()-x)<cube.getSide()/2)
    x=center.getx();
  else if (center.getx()>x)
    x+=cube.getSide()/2;
  else
    x-=cube.getSide()/2;
  if (fabs(center.gety()-y)<cube.getSide()/2)
    y=center.gety();
  else if (center.gety()>y)
    y+=cube.getSide()/2;
  else
    y-=cube.getSide()/2;
  ret=xyz(x,y,z);
  return ret;
}

Column::Column()
{
  side=0;
}

Column::Column(xy c,double s)
{
  center=c;
  side=s;
}

bool Column::in(xyz pnt)
{
  return fabs(center.getx()-pnt.getx())<=side/2 && fabs(center.gety()-pnt.gety())<=side/2;
}

xyz Column::closestPoint(Cube cube)
{
  xyz ret=cube.getCenter();
  double x=ret.getx(),y=ret.gety(),z=ret.getz();
  if (fabs(center.getx()-x)<cube.getSide()/2)
    x=center.getx();
  else if (center.getx()>x)
    x+=cube.getSide()/2;
  else
    x-=cube.getSide()/2;
  if (fabs(center.gety()-y)<cube.getSide()/2)
    y=center.gety();
  else if (center.gety()>y)
    y+=cube.getSide()/2;
  else
    y-=cube.getSide()/2;
  ret=xyz(x,y,z);
  return ret;
}
