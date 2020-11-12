/******************************************************/
/*                                                    */
/* flowsnake.h - flowsnake curve                      */
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
/* The flowsnake is a sequence of all Eisenstein integers, where each is
 * followed by an adjacent number. This program proceeds through the
 * point cloud in flowsnake order to minimize thrashing of the octree.
 *
 *      5 6
 *     2 3 4 5 6
 *  5 6.0 1.2 3 4
 * 2 3 4.5 6.0 1
 *  0 1.2 3 4.5 6
 *   5 6.0 1.2 3 4
 *  2 3 4 5 6 0 1
 *   0 1 2 3 4
 *        5 6
 *
 *      5 5
 *     5 5 5 6 6
 *  2 2 5 5 6 6 6
 * 2 2 2 3 3 6 6
 *  2 2 3 3 3 4 4
 *   0 0 3 3 4 4 4
 *  0 0 0 1 1 4 4
 *   0 0 1 1 1
 *        1 1
 *
 *      5 4
 *     6 2 3 5 4
 *  5 0 1 0 6 2 3
 * 6 4 1 1 2 1 0
 *  3 2 0 4 3 1 6
 *   3 6 5 6 0 2 5
 *  2 4 5 5 4 3 4
 *   1 0 6 2 3
 *        1 0
 *
 *      5 5
 *     5 5 5 4 4
 *  6 6 5 5 4 4 4
 * 6 6 6 2 2 4 4
 *  6 6 2 2 2 3 3
 *   1 1 2 2 3 3 3
 *  1 1 1 0 0 3 3
 *   1 1 0 0 0
 *        0 0
 *
 */

#ifndef FLOWSNAKE_H
#define FLOWSNAKE_H

#include <vector>
#include "eisenstein.h"
#include "threads.h"
#include "shape.h"

extern const Eisenstein flowBase;
extern const std::complex<double> cFlowBase;;

int baseSeven(Eisenstein e);
Eisenstein baseFlow(int n);
Eisenstein toFlowsnake(int n);
std::vector<std::complex<double> > crinklyLine(std::complex<double> begin,std::complex<double> end,double precision);
double biggestSquare(int size);

class Flowsnake
{
public:
  void setSize(Cube cube,double desiredSpacing);
  //Cylinder next;
private:
  xy center;
  double spacing;
};

#endif
