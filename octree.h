/******************************************************/
/*                                                    */
/* octree.h - octrees                                 */
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

#ifndef OCTREE_H
#define OCTREE_H
#include <map>
#include "shape.h"
#include "las.h"
#include "threads.h"
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
  void setBlock(xyz pnt,long long blk);
  void sizeFit(std::vector<xyz> pnts);
  void split(xyz pnt);
  xyz getCenter()
  {
    return center;
  }
  double getSide()
  {
    return side;
  }
  Cube cube()
  {
    return Cube(center,side);
  }
private:
  xyz center;
  double side;
  uintptr_t sub[8]; // Even means Octree *; odd means a disk block.
};

extern Octree octRoot;
extern OctStore octStore;


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
  void resize(int n);
  void open(std::string fileName);
  void close();
  LasPoint get(xyz key);
  void put(LasPoint pnt);
  void dump();
  bool isConsistent();
private:
  std::fstream file;
  std::mutex fileMutex;
  std::shared_mutex setBlockMutex;
  std::recursive_mutex splitMutex;
  int nowUsed;
  int leastRecentlyUsed();
  long long nBlocks;
  OctBlock *getBlock(long long block,bool mustExist=false);
  OctBlock *getBlock(xyz key,bool writing);
  std::map<int,OctBlock> blocks;
  std::map<int,std::shared_mutex> blockMutexes;
  void split(long long block,xyz camelStraw);
  friend class OctBlock;
};
#endif
