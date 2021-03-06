/******************************************************/
/*                                                    */
/* peano.h - Peano curve                            */
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

#include <array>

#define THREE20 3486784401
#define THREE19 (unsigned)1162261467

std::array<int,3> peanoPoint(int width,int height,unsigned phase,int direction=0);

class Peano
{
public:
  Peano();
  void resize(int w,int h);
  std::array<int,2> step();
private:
  int width,height;
  unsigned phase;
};
