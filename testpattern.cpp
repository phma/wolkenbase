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
