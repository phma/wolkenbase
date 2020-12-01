/******************************************************/
/*                                                    */
/* boundrect.h - bounding rectangles                  */
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
#ifndef BOUNDRECT_H
#define BOUNDRECT_H
class BoundRect;
#include <array>
#include "point.h"

class BoundRect
{
private:
  int orientation;
  std::array<double,6> bounds;
  // The six numbers are left, bottom, -right, -top, low, and -high.
public:
  BoundRect();
  BoundRect(int ori);
  void clear();
  void setOrientation(int ori);
  int getOrientation();
  void include(xyz obj);
  double left()
  {
    return bounds[0];
  }
  double bottom()
  {
    return bounds[1];
  }
  double right()
  {
    return -bounds[2];
  }
  double top()
  {
    return -bounds[3];
  }
  double low()
  {
    return bounds[4];
  }
  double high()
  {
    return -bounds[5];
  }
};
#endif
