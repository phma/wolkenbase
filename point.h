/******************************************************/
/*                                                    */
/* point.h - classes for points                       */
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

#ifndef POINT_H
#define POINT_H
#include <string>
#include <fstream>
#include <vector>

class xyz;
class latlong;

#include "quaternion.h"

class xy
{
public:
  xy(double e,double n);
  xy(xyz point);
  xy();
  double east() const;
  double north() const;
  double getx() const;
  double gety() const;
  double length() const;
  bool isfinite() const;
  bool isnan() const;
  double dirbound(int angle);
  void _roscat(xy tfrom,int ro,double sca,xy cis,xy tto);
  virtual void roscat(xy tfrom,int ro,double sca,xy tto); // rotate, scale, translate
  friend xy operator+(const xy &l,const xy &r);
  friend xy operator+=(xy &l,const xy &r);
  friend xy operator-=(xy &l,const xy &r);
  friend xy operator-(const xy &l,const xy &r);
  friend xy operator-(const xy &r);
  friend xy operator*(const xy &l,double r);
  friend xy operator*(double l,const xy &r);
  friend xy operator/(const xy &l,double r);
  friend xy operator/=(xy &l,double r);
  friend bool operator!=(const xy &l,const xy &r);
  friend bool operator==(const xy &l,const xy &r);
  friend xy turn90(xy a);
  friend xy turn(xy a,int angle);
  friend double dist(xy a,xy b);
  friend int dir(xy a,xy b);
  friend double dot(xy a,xy b);
  friend double area3(xy a,xy b,xy c);
  friend class triangle;
  friend class point;
  friend class xyz;
  friend class qindex;
protected:
  double x,y;
};

extern const xy nanxy;

class xyz
{
public:
  xyz(double e,double n,double h);
  xyz();
  xyz(xy en,double h);
  double east() const;
  double north() const;
  double elev() const;
  double getx() const;
  double gety() const;
  double getz() const;
  bool isfinite() const;
  bool isnan() const;
  double length();
  void raise(double height);
  void _roscat(xy tfrom,int ro,double sca,xy cis,xy tto);
  void roscat(xy tfrom,int ro,double sca,xy tto);
  void setelev(double h)
  {
    z=h;
  }
  friend class xy;
  friend class point;
  friend class triangle;
  friend class Quaternion;
  friend double dist(xyz a,xyz b);
  friend double dot(xyz a,xyz b);
  friend xyz cross(xyz a,xyz b);
  friend bool operator==(const xyz &l,const xyz &r);
  friend bool operator!=(const xyz &l,const xyz &r);
  friend xyz operator/(const xyz &l,const double r);
  friend xyz operator*=(xyz &l,double r);
  friend xyz operator/=(xyz &l,double r);
  friend xyz operator+=(xyz &l,const xyz &r);
  friend xyz operator-=(xyz &l,const xyz &r);
  friend xyz operator*(const xyz &l,const double r);
  friend xyz operator*(const double l,const xyz &r);
  friend xyz operator*(const xyz &l,const xyz &r); // cross product
  friend xyz operator+(const xyz &l,const xyz &r);
  friend xyz operator-(const xyz &l,const xyz &r);
  friend xyz operator-(const xyz &r);
  friend Quaternion versor(xyz vec);
  friend Quaternion versor(xyz vec,int angle);
  friend Quaternion versor(xyz vec,double angle);
protected:
  double x,y,z;
};

extern const xyz nanxyz;
#endif
