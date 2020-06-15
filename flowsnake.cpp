/******************************************************/
/*                                                    */
/* flowsnake.cpp - flowsnake curve                    */
/*                                                    */
/******************************************************/
/* Copyright 2020 Pierre Abbat.
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

#include "flowsnake.h"

const Eisenstein flowBase(2,-1);

int base7dig(Eisenstein d)
/* Input: d has norm 0 or 1.
 * Output: [-3,3].
 */
{
  return d.getx()+2*d.gety();
}

Eisenstein baseFlowDig(int d)
{
  int x,y;
  y=(d+4)/3-1;
  x=d-2*y;
  return Eisenstein(x,y);
}

int baseSeven(Eisenstein e)
/* Converts e, in centered base 2-ω, to an integer in centered base 7.
 */
{
  int ret=0;
  int pow7=1;
  Eisenstein edig;
  while (e.norm())
  {
    edig=e%flowBase;
    e=e/flowBase;
    ret+=base7dig(edig)*pow7;
    pow7*=7;
  }
  return ret;
}

Eisenstein baseFlow(int n)
/* Converts n, in centered base 7, to an Eisenstein integer in centered base 2-ω.
 */
{
  Eisenstein ret=0;
  Eisenstein powF=1;
  int dig;
  while (n)
  {
    dig=n%7;
    if (dig>3)
      dig-=7;
    if (dig<-3)
      dig+=7;
    n=(n-dig)/7;
    ret+=baseFlowDig(dig)*powF;
    powF*=flowBase;
  }
  return ret;
}
