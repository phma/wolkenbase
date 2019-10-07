/******************************************************/
/*                                                    */
/* shape.h - shapes                                   */
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

#include "point.h"

class Cube
{
public:
  Cube();
  Cube(xyz c,double s);
  bool in(xyz pnt);
private:
  xyz center;
  double side;
};

class Shape
{
public:
  virtual bool in(xyz pnt)=0;
  virtual bool intersectCube(Cube cube)=0;
};

class Paraboloid: public Shape
/* Points down if radiusCurvature is positive.
 * radiusCurvature is the radius of curvature at the vertex,
 * which is twice the focal length and half the latus rectum.
 */
{
public:
  Paraboloid();
  Paraboloid(xyz v,double r);
  virtual bool in(xyz pnt);
  virtual bool intersectCube(Cube cube);
private:
  xyz vertex;
  double radiusCurvature;
};
