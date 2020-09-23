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
#include <vector>
#include <set>
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
//#define shared_mutex mutex
//#define lock_shared lock
//#define unlock_shared unlock

class OctStore;

class Octree
{
public:
  long long findBlock(xyz pnt);
  Cube findCube(xyz pnt);
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
  Cube cube(int n=-1);
  int dump(std::ofstream &file);
private:
  xyz center;
  double side;
  uintptr_t sub[8]; // Even means Octree *; odd means a disk block.
};

extern Octree octRoot;
extern OctStore octStore;
extern double lowRam;

class OctBuffer
{
public:
  OctBuffer();
  void write();
  void markDirty();
  void own();
  bool ownAlone();
  bool iOwn();
  void read(long long block);
  void update();
  void flush();
  LasPoint get(xyz key);
  bool put(LasPoint pnt);
  int dump(std::ofstream &file,Cube cube);
  //bool isConsistent();
private:
  int bufferNumber;
  long long blockNumber;
  bool dirty; // The contents of the buffer may differ from the contents of the block.
  bool inTransit; // The correspondence between buffer and block is being changed.
  long long lastUsed;
  std::set<int> owningThread;
  std::vector<LasPoint> points;
  std::shared_mutex blockMutex;
  OctStore *store;
  friend class OctStore;
};

class OctStore
{
public:
  OctStore();
  ~OctStore();
  void flush(int thread=0,int nthreads=1);
  void disown();
  bool setTransit(int buffer,bool t);
  void resize(int n);
  size_t getNumBuffers()
  {
    return blocks.size();
  }
  size_t getNumBlocks()
  {
    return nBlocks;
  }
  void open(std::string fileName,int numFiles=1);
  void close();
  LasPoint get(xyz key);
  void put(LasPoint pnt,bool splitting=false);
  void dump(std::ofstream &file);
  bool isConsistent();
private:
  std::map<int,std::fstream> file;
  std::map<int,std::mutex> fileMutex; // lock when read/writing file
  std::shared_mutex setBlockMutex; // lock when adding new blocks to file
  std::shared_mutex nowUsedMutex; // lock when updating nowUsed
  std::recursive_mutex splitMutex; // lock when splitting
  std::shared_mutex revMutex; // lock when changing revBlocks
  std::mutex ownMutex; // lock when owning or disowning blocks
  std::mutex transitMutex; // lock when setting or clearing inTransit
  std::shared_mutex bufferMutex; // lock when adding new buffers to store
  long long nowUsed;
  int nFiles;
  int leastRecentlyUsed();
  long long nBlocks;
  int newBlock();
  OctBuffer *getBlock(long long block,bool mustExist=false);
  OctBuffer *getBlock(xyz key,bool writing);
  std::map<int,OctBuffer> blocks; // buffer number -> block
  std::map<int,int> revBlocks; // block number -> buffer number
  std::map<long long,int> lastUsedMap; // time counter -> buffer number
  std::map<int,std::vector<int> > ownMap; // thread -> buffer number
  void split(long long block,xyz camelStraw);
  friend class OctBuffer;
  friend class Octree;
};
#endif
