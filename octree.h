/******************************************************/
/*                                                    */
/* octree.h - octrees                                 */
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

#include <map>
#include "las.h"
#define LASPOINT_SIZE 91
#define RECORDS 720
#define BLOCKSIZE 65536
/* If a future version of LAS adds more fields, making the size of LasPoint
 * on disk bigger, these should be changed so that LASPOINT_SIZE*RECORDS is
 * equal to or slightly smaller than BLOCKSIZE, which is a power of 2 at least
 * 4096, and RECORDS<=1000.
 */
#include <vector>

class OctStore;

class Octree
{
public:
  long long findBlock(xyz pnt);
  void sizeFit(std::vector<xyz> pnts);
  xyz getCenter()
  {
    return center;
  }
  double getSide()
  {
    return side;
  }
private:
  xyz center;
  double side;
  uintptr_t sub[8]; // Even means Octree *; odd means a disk block.
};

extern Octree octRoot;

class OctBlock
{
public:
  OctBlock();
  void write();
  void markDirty();
  void read(long long block);
  void update();
  void flush();
  //void dump();
  //bool isConsistent();
private:
  long long blockNumber;
  bool dirty;
  int lastUsed;
  std::vector<LasPoint> points;
  OctStore *store;
  friend class OctStore;
};

class OctStore
{
public:
  OctStore();
  ~OctStore();
  void flush();
  void open(std::string fileName);
  void close();
  LasPoint &operator[](xyz key);
  void dump();
  bool isConsistent();
private:
  std::fstream file;
  int nowUsed;
  int leastRecentlyUsed();
  long long nBlocks;
  OctBlock *getBlock(long long block,bool mustExist=false);
  OctBlock *getBlock(xyz key);
  std::map<int,OctBlock> blocks;
  void split(int block,xyz camelStraw);
  friend class OctBlock;
};
