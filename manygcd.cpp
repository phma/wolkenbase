/******************************************************/
/*                                                    */
/* manygcd.cpp - GCD of many numbers                  */
/*                                                    */
/******************************************************/
/* Copyright 2022 Pierre Abbat.
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
#include "manygcd.h"
using namespace std;

void bubbleup(vector<double> &numbers)
{
  int i;
  for (i=0;i<numbers.size()-1;i++)
    if (numbers[i]>numbers[i+1])
      swap(numbers[i],numbers[i+1]);
}

void bubbledown(vector<double> &numbers)
{
  int i;
  for (i=numbers.size()-2;i>=0;i--)
    if (numbers[i]>numbers[i+1])
      swap(numbers[i],numbers[i+1]);
}

double manygcd(vector<double> numbers,double toler)
/* Computes the greatest common divisor of many numbers. The numbers are
 * the xOffset, yOffset, and zOffset of many parts of a point cloud, which
 * are normally multiples of xScale, yScale, and zScale respectively, which
 * is typically a negative power of 10 such as 0.001. The numbers are inexact;
 * the tolerance is a fraction of the scale.
 */
{
  vector<double> discard;
  int i,sz;
  for (i=0;i<numbers.size();i++)
    numbers[i]=fabs(numbers[i]);
  bubbleup(numbers);
  bubbledown(numbers);
  bubbleup(numbers);
  while (numbers.size() && numbers.back()>numbers[0]+toler)
  {
    sz=numbers.size();
    if (numbers[sz-1]-numbers[sz-2]>toler)
    {
      numbers[sz-1]-=numbers[sz-2];
      bubbledown(numbers);
    }
    else
    {
      discard.push_back(numbers[sz-1]);
      numbers.resize(sz-1);
    }
    bubbleup(numbers);
  }
  return numbers.size()?numbers[0]:0;
}

