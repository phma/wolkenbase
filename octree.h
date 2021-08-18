/******************************************************/
/*                                                    */
/* octree.h - octrees                                 */
/*                                                    */
/******************************************************/
/* Copyright 2019-2021 Pierre Abbat.
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
#include "config.h"
#include "shape.h"
#include "las.h"
#include "threads.h"
#include "ps.h"

#ifdef WAVEFORM
#define LASPOINT_SIZE 91
#define RECORDS 720
#define BLOCKSIZE 65536
#else
#define LASPOINT_SIZE 61
#define RECORDS 537
#define BLOCKSIZE 32768
#endif
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
  std::vector<long long> findBlocks(const Shape &sh);
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
  void plot(PostScript &ps);
  uint64_t countPoints();
private:
  xyz center;
  double side;
  uintptr_t sub[8]; // Even means Octree *; odd means a disk block.
  uint64_t count;
};

extern Octree octRoot;
extern OctStore octStore;
extern double lowRam;
extern std::vector<xyz> alreadyInOctree;

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
  void shrink();
  LasPoint get(xyz key);
  bool put(LasPoint pnt);
  std::map<int,size_t> countClasses();
  uint64_t countPoints()
  {
    return points.size();
  }
  std::vector<LasPoint> getAll()
  {
    return points;
  }
  int dump(std::ofstream &file,Cube cube);
  void plot(PostScript &ps,Cube cube);
  double getLow()
  {
    return low;
  }
  double getHigh()
  {
    return high;
  }
  //bool isConsistent();
private:
  bool dirty; // The contents of the buffer may differ from the contents of the block.
  bool inTransit; // The correspondence between buffer and block is being changed.
  int bufferNumber;
  long long blockNumber;
  long long lastUsed;
  double low,high;
  std::set<int> owningThread;
  std::vector<LasPoint> points;
  std::shared_mutex blockMutex;
  std::mutex ownMutex; // lock when owning or disowning blocks
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
  void shrink();
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
  std::map<int,size_t> countClasses(long long block);
  std::vector<LasPoint> getAll(long long block);
  void dump(std::ofstream &file);
  void plot(PostScript &ps);
  void setIgnoreDupes(bool ig);
  void dumpBuffers();
  bool isConsistent();
  std::vector<LasPoint> pointsIn(const Shape &sh,bool sorted=false);
  std::array<double,2> hiLoPointsIn(const Shape &sh);
  std::shared_mutex setBlockMutex; // lock when adding new blocks to file
private:
  std::map<int,std::fstream> file;
  std::map<int,std::mutex> fileMutex; // lock when read/writing file
  std::shared_mutex nowUsedMutex; // lock when updating nowUsed
  std::recursive_mutex splitMutex; // lock when splitting
  std::shared_mutex revMutex; // lock when changing revBlocks
  std::mutex transitMutex; // lock when setting or clearing inTransit
  std::shared_mutex bufferMutex; // lock when adding new buffers to store
  long long nowUsed;
  int nFiles;
  bool ignoreDupes;
  int leastRecentlyUsed(int thread,int nthreads);
  long long nBlocks;
  int newBlock();
  OctBuffer *getBlock(long long block,bool mustExist=false);
  OctBuffer *getBlock(xyz key,bool writing);
  std::map<int,OctBuffer> blocks; // buffer number -> block
  std::map<int,int> revBlocks; // block number -> buffer number
  std::multimap<long long,int> lastUsedMap; // time counter -> buffer number
  std::map<int,std::vector<int> > ownMap; // thread -> buffer number
  void split(long long block,xyz camelStraw);
  friend class OctBuffer;
  friend class Octree;
};
#endif
