/******************************************************/
/*                                                    */
/* wkt.h - well-known text                            */
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
#ifndef WKT_H
#define WKT_H
#include <vector>
#include <string>

class WellKnownTree;

enum WellKnownType
{
  quoted,
  number, // TODO add date
  branch
};

struct WellKnownItem
{
  WellKnownType typeTag;
  std::string quotedVal;
  double numberVal;
  WellKnownTree *branchVal;
};

class WellKnownTree
{
public:
  WellKnownTree();
private:
  std::string keyword;
  std::vector<WellKnownItem> items;
};

#endif
