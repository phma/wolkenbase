/******************************************************/
/*                                                    */
/* octree.h - octrees                                 */
/*                                                    */
/******************************************************/
/* Copyright 2019-2022 Pierre Abbat.
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
#include <deque>
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
  Octree();
  ~Octree();
  void clear();
  int64_t findBlock(xyz pnt);
  Cube findCube(xyz pnt);
  std::vector<int64_t> findBlocks(const Shape &sh);
  void setBlock(xyz pnt,int64_t blk);
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
  uint64_t getCount()
  {
    return count;
  }
  void countPoints(unsigned int n);
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
  void read(int64_t block);
  void update();
  void flush();
  void clear();
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
  int64_t blockNumber;
  int64_t lastUsed;
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
  void clearBlocks();
  void clear();
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
  std::map<int,size_t> countClasses(int64_t block);
  std::vector<LasPoint> getAll(int64_t block);
  void dump(std::ofstream &file);
  void plot(PostScript &ps);
  void setIgnoreDupes(bool ig);
  void dumpBuffers();
  bool isConsistent();
  std::vector<LasPoint> pointsIn(const Shape &sh,bool sorted=false);
  uint64_t countPointsIn(const Shape &sh);
  std::array<double,2> hiLoPointsIn(const Shape &sh);
  uint64_t countPoints();
  std::shared_mutex setBlockMutex; // lock when adding new blocks to file
private:
  std::map<int,std::fstream> file;
  std::map<int,std::mutex> fileMutex; // lock when read/writing file
  std::shared_mutex nowUsedMutex; // lock when updating nowUsed
  std::recursive_mutex splitMutex; // lock when splitting
  std::shared_mutex revMutex; // lock when changing revBlocks
  std::mutex transitMutex; // lock when setting or clearing inTransit
  std::shared_mutex bufferMutex; // lock when adding new buffers to store
  std::mutex countMutex; // lock when growing blockPointCount and blockGroupCount
  int64_t nowUsed;
  uint64_t clockNum,clockDen;
  int nFiles;
  uint32_t blockGroup;
  bool ignoreDupes;
  int leastRecentlyUsed(int thread,int nthreads);
  int64_t nBlocks;
  int newBlock();
  void updateCount(int64_t block,int nPoints);
  OctBuffer *getBlock(int64_t block,bool mustExist=false);
  OctBuffer *getBlock(xyz key,bool writing);
  std::map<int,OctBuffer> blocks; // buffer number -> block
  std::map<int,int> revBlocks; // block number -> buffer number
  std::multimap<int64_t,int> lastUsedMap; // time counter -> buffer number
  std::map<int,std::vector<int> > ownMap; // thread -> buffer number
  std::deque<uint16_t> blockPointCount; // block number -> number of points
  std::deque<uint32_t> blockGroupCount; // 256 blocks group -> total number of points
  uint64_t totalPoints;
  void split(int64_t block,xyz camelStraw);
  friend class OctBuffer;
  friend class Octree;
};
#endif
