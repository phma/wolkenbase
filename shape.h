/******************************************************/
/*                                                    */
/* shape.h - shapes                                   */
/*                                                    */
/******************************************************/
/* Copyright 2019-2021 Pierre Abbat.
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

#ifndef SHAPE_H
#define SHAPE_H
#include "point.h"

class Cube
{
public:
  Cube();
  Cube(xyz c,double s);
  bool in(xyz pnt);
  xyz getCenter()
  {
    return center;
  }
  double getSide()
  {
    return side;
  }
  double minX()
  {
    return center.getx()-side/2;
  }
  double maxX()
  {
    return center.getx()+side/2;
  }
  double minY()
  {
    return center.gety()-side/2;
  }
  double maxY()
  {
    return center.gety()+side/2;
  }
  double minZ()
  {
    return center.getz()-side/2;
  }
  double maxZ()
  {
    return center.getz()+side/2;
  }
private:
  xyz center;
  double side;
};

class Shape
{
public:
  virtual bool in(xyz pnt) const=0;
  virtual xyz closestPoint(Cube cube) const=0; // closest point to the shape in the cube
  virtual bool intersect(Cube cube) const;
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
  virtual bool in(xyz pnt) const;
  virtual xyz closestPoint(Cube cube) const;
private:
  xyz vertex;
  double radiusCurvature;
};

class Hyperboloid: public Shape
/* One sheet of a two-sheet hyperboloid. Points down if slope is positive.
 * The parameters are the vertex, the radius of curvature, and the slope.
 * The members are the center, the polar radius squared, and the slope.
 * The radius of curvature at the vertex (pole) of an ellipsoid is
 * the square of the equatorial radius divided by the polar radius. The same
 * formula holds for the hyperboloid, but as the equatorial radius is imaginary,
 * the radius of curvature is negative, meaning that it is convex away from
 * the center. This sign is ignored.
 */
{
public:
  Hyperboloid();
  Hyperboloid(xyz v,double r,double s);
  virtual bool in(xyz pnt) const;
  virtual xyz closestPoint(Cube cube) const;
private:
  xyz center;
  double por2,slope;
};

class Sphere: public Shape
/* Points down if radiusCurvature is positive.
 * radiusCurvature is the radius of curvature at the vertex,
 * which is twice the focal length and half the latus rectum.
 */
{
public:
  Sphere();
  Sphere(xyz c,double r);
  virtual bool in(xyz pnt) const;
  virtual xyz closestPoint(Cube cube) const;
private:
  xyz center;
  double radius;
};

class Cylinder: public Shape
/* Used for traversing the point cloud in flowsnake order.
 * A hexagon in the lattice has a diapothem, which is also the distance to
 * the adjacent hexagons. Construct a cylinder whose radius is 41/71 times
 * the diapothem. This provides a tiny overlap at the corners so that
 * no points are missed.
 */
{
public:
  Cylinder();
  Cylinder(xy c,double r);
  double getRadius()
  {
    return radius;
  }
  xy getCenter()
  {
    return center;
  }
  virtual bool in(xyz pnt) const;
  virtual xyz closestPoint(Cube cube) const;
private:
  xy center;
  double radius;
};

class Column: public Shape
/* Represents a pixel in the GUI.
 * An infinitely tall prism with a square cross section.
 */
{
public:
  Column();
  Column(xy c,double s);
  virtual bool in(xyz pnt) const;
  virtual xyz closestPoint(Cube cube) const;
private:
  xy center;
  double side;
};
#endif
