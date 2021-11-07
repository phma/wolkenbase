/******************************************************/
/*                                                    */
/* cloud.cpp - point cloud                            */
/*                                                    */
/******************************************************/
/* Copyright 2019,2021 Pierre Abbat.
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

#include "cloud.h"
#include "octree.h"
using namespace std;

vector<xyz> cloud;

int64_t getNumCloudBlocks()
{
  return (cloud.size()+RECORDS-1)/RECORDS;
}

vector<LasPoint> getCloudBlock(int64_t n)
{
  LasPoint pnt;
  vector<LasPoint> ret;
  int64_t i;
  for (i=n*RECORDS;i<(n+1)*RECORDS && i<cloud.size();i++)
  {
    pnt.location=cloud[i];
    ret.push_back(pnt);
  }
  return ret;
}
