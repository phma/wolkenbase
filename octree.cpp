/******************************************************/
/*                                                    */
/* octree.cpp - octrees                               */
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
#include <cassert>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "ldecimal.h"
#include "octree.h"
#include "threads.h"
#include "brevno.h"
#include "freeram.h"
#define DEBUG_STORE 0
#define DEBUG_LOCK 0
#define WATCH_BLOCK_START 0
#define WATCH_BLOCK_END 0
using namespace std;

Octree octRoot;
OctStore octStore;
double lowRam;
set<int> watchedBuffers;
mutex msgMutex;
mutex alreadyMutex;
vector<xyz> alreadyInOctree;

#if defined(_WIN32) || defined(__CYGWIN__)
// Linux and BSD have this function in the library; Windows doesn't.
double significand(double x)
{
  int dummy;
  return frexp(x,&dummy)*2;
}
#endif

int64_t band(xyz pnt)
// Separates the space into bands, which are used for locking cubes.
{
  return llrint((pnt.getx()+pnt.gety()+pnt.getz())*cubeMutex.size()/
		octRoot.getSide()+((sqrt(5)-1)/2));
}

set<int> whichLocks(Cube cube)
{
  set<int> ret;
  int64_t lo,hi,i;
  lo=band(cube.corner(0));
  hi=band(cube.corner(7));
  if (lo<0)
  {
    i=((-lo-hi)/cubeMutex.size())*cubeMutex.size();
    lo+=i;
    hi+=i;
  }
  for (i=0;cube.getSide() && i<=hi;i++)
    ret.insert(i%cubeMutex.size());
  return ret;
}

bool cubeLocked(xyz pnt)
/* cubeMutex must be locked when calling this.
 * Returns true iff pnt is in a cube locked by another thread.
 */
{
  int t=thisThread();
  map<int,Cube>::iterator i;
  bool ret=false;
  for (i=lockedCubes[0].begin();!ret && i!=lockedCubes[0].end();++i)
    ret=t!=i->first && i->second.in(pnt);
  return ret;
}

bool cubeReadLocked(xyz pnt)
/* cubeMutex must be locked when calling this.
 * Returns true iff pnt is in a cube read-locked by another thread.
 */
{
  int t=thisThread();
  map<int,Cube>::iterator i;
  bool ret=false;
  for (i=readLockedCubes[0].begin();!ret && i!=readLockedCubes[0].end();++i)
    ret=t!=i->first && i->second.in(pnt);
  return ret;
}

bool lockCube(Cube cube)
// Returns true if successful.
{
  bool ret;
  int t=thisThread();
  cubeMutex[0].lock();
  ret=!(cubeLocked(cube.getCenter()) || cubeReadLocked(cube.getCenter()));
  if (ret)
  {
    heldCubes[t]=cube;
    lockedCubes[0].insert(pair<int,Cube>(t,cube));
  }
  cubeMutex[0].unlock();
  return ret;
}

bool readLockCube(Cube cube)
// Returns true if successful.
{
  bool ret;
  int t=thisThread();
  set<int> lockSet=whichLocks(cube);
  cubeMutex[0].lock();
  ret=!cubeLocked(cube.getCenter());
  if (ret)
  {
    heldCubes[t]=cube;
    readLockedCubes[0].insert(pair<int,Cube>(t,cube));
  }
  cubeMutex[0].unlock();
  return ret;
}

void unlockCube()
{
  int t=thisThread();
  cubeMutex[0].lock();
  lockedCubes[0].erase(t);
  readLockedCubes[0].erase(t);
  heldCubes[t]=Cube();
  cubeMutex[0].unlock();
}

Octree::Octree()
{
  for (count=0;count<8;count++)
    sub[count]=0;
  count=0;
}

Octree::~Octree()
{
  clear();
}

void Octree::clear()
{
  int i;
  for (i=0;i<8;i++)
  {
    if (sub[i] && !(sub[i]&1))
      delete (Octree *)sub[i];
    sub[i]=0;
  }
}

long long Octree::findBlock(xyz pnt)
// Returns the disk block number that contains pnt, or -1 if none.
{
  int xbit,ybit,zbit,i;
  uintptr_t subi;
  xbit=pnt.getx()>=center.getx();
  ybit=pnt.gety()>=center.gety();
  zbit=pnt.getz()>=center.getz();
  i=zbit*4+ybit*2+xbit;
  assert(side>0);
  subi=sub[i];
  if (subi==0)
    return -1;
  else if (subi&1)
    return subi>>1;
  else
    return ((Octree *)subi)->findBlock(pnt);
}

Cube Octree::findCube(xyz pnt)
// Returns the cube that contains pnt.
{
  int xbit,ybit,zbit,i;
  uintptr_t subi;
  xbit=pnt.getx()>=center.getx();
  ybit=pnt.gety()>=center.gety();
  zbit=pnt.getz()>=center.getz();
  i=zbit*4+ybit*2+xbit;
  subi=sub[i];
  if (subi==0 || (subi&1))
    return cube(i);
  else
    return ((Octree *)subi)->findCube(pnt);
}

vector<long long> Octree::findBlocks(const Shape &sh)
// Find all blocks that intersect the shape.
{
  vector<long long> ret,part;
  int i,j;
  if (sh.intersect(cube()))
    for (i=0;i<8;i++)
    {
      part.clear();
      if (sub[i]&1 && sh.intersect(cube(i)))
	part.push_back(sub[i]>>1);
      if (!(sub[i]&1) && sub[i])
	part=((Octree *)sub[i])->findBlocks(sh);
      for (j=0;j<part.size();j++)
	ret.push_back(part[j]);
    }
  return ret;
}

void Octree::setBlock(xyz pnt,long long blk)
{
  int xbit,ybit,zbit,i;
  xbit=pnt.getx()>=center.getx();
  ybit=pnt.gety()>=center.gety();
  zbit=pnt.getz()>=center.getz();
  i=zbit*4+ybit*2+xbit;
  if (sub[i]==0)
    sub[i]=blk*2+1;
  else if (sub[i]&1)
    cerr<<"Tried to set block "<<blk<<", point already in block "<<(sub[i]>>1)<<endl;
  else
    ((Octree *)sub[i])->setBlock(pnt,blk);
}

void Octree::sizeFit(vector<xyz> pnts)
/* Computes side and center such that side is a power of 2, x, y, and z are multiples
 * of side/16, and all points are in the resulting cube.
 */
{
  double minx=HUGE_VAL,miny=HUGE_VAL,minz=HUGE_VAL;
  double maxx=-HUGE_VAL,maxy=-HUGE_VAL,maxz=-HUGE_VAL;
  double x,y,z;
  int i;
  for (i=0;i<pnts.size();i++)
  {
    if (pnts[i].east()>maxx)
      maxx=pnts[i].east();
    if (pnts[i].east()<minx)
      minx=pnts[i].east();
    if (pnts[i].north()>maxy)
      maxy=pnts[i].north();
    if (pnts[i].north()<miny)
      miny=pnts[i].north();
    if (pnts[i].elev()>maxz)
      maxz=pnts[i].elev();
    if (pnts[i].elev()<minz)
      minz=pnts[i].elev();
  }
  if (maxz<=minz && maxy<=miny && maxx<=minx)
    side=0;
  else
  {
    side=(maxx+maxy+maxz-minx-miny-minz)/3;
    side/=significand(side);
    x=minx-side;
    y=miny-side;
    z=minz-side;
    while (x+side<=maxx || y+side<=maxy || z+side<=maxz)
    {
      side*=2;
      x=(rint((minx+maxx)/side*8)-8)*side/16;
      y=(rint((miny+maxy)/side*8)-8)*side/16;
      z=(rint((minz+maxz)/side*8)-8)*side/16;
    }
    center=xyz(x+side/2,y+side/2,z+side/2);
  }
}

void Octree::split(xyz pnt)
/* Splits the block containing pnt into eight blocks. The subblock with pnt in it
 * is the same as the original block; the others are set to none.
 * Called from OctStore::split.
 */
{
  int xbit,ybit,zbit,i;
  uintptr_t blknum;
  Octree *newblk;
  xbit=pnt.getx()>=center.getx();
  ybit=pnt.gety()>=center.gety();
  zbit=pnt.getz()>=center.getz();
  i=zbit*4+ybit*2+xbit;
  if (sub[i]==0)
    cerr<<"Can't split empty block\n";
  else if (sub[i]&1)
  {
    octStore.setBlockMutex.lock();
    blknum=sub[i];
    sub[i]=(uintptr_t)(newblk=new Octree);
    newblk->center=xyz(center.getx()+(2*xbit-1)*side/4,
		       center.gety()+(2*ybit-1)*side/4,
		       center.getz()+(2*zbit-1)*side/4);
    newblk->side=side/2;
    for (i=0;i<8;i++)
      newblk->sub[i]=0;
    xbit=pnt.getx()>=newblk->center.getx();
    ybit=pnt.gety()>=newblk->center.gety();
    zbit=pnt.getz()>=newblk->center.getz();
    i=zbit*4+ybit*2+xbit;
    newblk->sub[i]=blknum;
    octStore.setBlockMutex.unlock();
  }
  else
    ((Octree *)sub[i])->split(pnt);
}

Cube Octree::cube(int n)
{
  int xbit=n&1,ybit=(n&2)/2,zbit=(n&4)/4;
  if (n<0)
    return Cube(center,side);
  else
    return Cube(xyz(center.getx()+(2*xbit-1)*side/4,
		    center.gety()+(2*ybit-1)*side/4,
		    center.getz()+(2*zbit-1)*side/4),side/2);
}

int Octree::dump(ofstream &file)
{
  int i,totalPoints=0;
  uintptr_t subi;
  for (i=0;i<8;i++)
  {
    subi=sub[i];
    if (subi&1)
    {
      totalPoints+=octStore.getBlock(subi>>1)->dump(file,cube(i));
      octStore.disown();
    }
    else if (subi)
      totalPoints+=((Octree *)subi)->dump(file);
  }
  return totalPoints;
}

void Octree::plot(PostScript &ps)
{
  int i;
  uintptr_t subi;
  for (i=0;i<8;i++)
  {
    subi=sub[i];
    if (subi&1)
    {
      octStore.getBlock(subi>>1)->plot(ps,cube(i));
      octStore.disown();
    }
    else if (subi)
      ((Octree *)subi)->plot(ps);
  }
}

void Octree::countPoints(unsigned int n)
// Descends the subtree specified by n, updating count.
{
  int i;
  uint64_t total=0;
  uintptr_t subi;
  subi=sub[n&7];
  if ((subi&1)==0 && subi)
    ((Octree *)subi)->countPoints(n>>3);
  for (i=0;i<8;i++)
  {
    subi=sub[i];
    if (subi&1)
      total+=octStore.getBlock(subi>>1)->countPoints();
    else if (subi)
      total+=((Octree *)subi)->count;
  }
  count=total;
}

OctBuffer::OctBuffer()
{
  int i;
  lastUsed=0;
  dirty=inTransit=false;
  high=-INFINITY;
  low=INFINITY;
  points.reserve(RECORDS);
  for (i=0;i<RECORDS;i++)
    points.emplace_back();
}

void OctBuffer::write()
{
  int i,f=blockNumber%store->nFiles,b=blockNumber/store->nFiles;
  int nPoints=0;
  blockMutex.lock_shared();
  store->fileMutex[f].lock();
  logBeginWrite(bufferNumber,blockNumber);
#if DEBUG_STORE
  cout<<"Writing block "<<blockNumber<<endl;
#endif
  store->file[f].seekp(BLOCKSIZE*b);
  //cout<<store->file.rdstate()<<' '<<ios::failbit<<endl;
  store->file[f].clear();
  for (i=0;i<points.size() && i<RECORDS;i++)
  {
    points[i].write(store->file[f]);
    nPoints+=!points[i].isEmpty();
  }
  for (;i<RECORDS;i++)
  {
    noPoint.write(store->file[f]);
  }
  dirty=false;
  for (i=RECORDS*LASPOINT_SIZE;i<BLOCKSIZE;i++)
    store->file[f].put(0);
  if ((blockNumber>=WATCH_BLOCK_START && blockNumber<WATCH_BLOCK_END) || watchedBuffers.count(bufferNumber))
  {
    msgMutex.lock();
    cout<<"Writing block "<<blockNumber<<" buffer "<<bufferNumber<<' '<<nPoints<<" points\n";
    watchedBuffers.insert(bufferNumber);
    msgMutex.unlock();
  }
  logEndWrite(bufferNumber,blockNumber);
  store->fileMutex[f].unlock();
  blockMutex.unlock_shared();
}

void OctBuffer::markDirty()
{
  dirty=true;
}

void OctBuffer::own()
{
  int t=thisThread();
  ownMutex.lock();
  owningThread.insert(t);
  ownMutex.unlock();
  if (store->ownMap[t].size()<=bufferNumber || store->ownMap[t][bufferNumber]!=bufferNumber)
  {
    store->ownMap[t].push_back(bufferNumber);
    if (store->ownMap[t].size()>bufferNumber)
      swap(store->ownMap[t][bufferNumber],store->ownMap[t].back());
  }
}

bool OctBuffer::ownAlone()
{
  bool ret;
  int t=thisThread();
  ownMutex.lock();
  ret=owningThread.size()==owningThread.count(t);
  if (ret)
  {
    owningThread.insert(t);
    store->ownMap[t].push_back(bufferNumber);
  }
  ownMutex.unlock();
  return ret;
}

bool OctBuffer::iOwn()
{
  bool ret;
  int t=thisThread();
  ownMutex.lock();
  ret=owningThread.count(t);
  ownMutex.unlock();
  return ret;
}

void OctBuffer::read(long long block)
{
  int i,f=block%store->nFiles,b=block/store->nFiles;
  int nPoints=0;
  LasPoint pnt;
  blockMutex.lock();
  store->fileMutex[f].lock();
  logBeginRead(bufferNumber,block);
#if DEBUG_STORE
  cout<<"Reading block "<<block<<" into "<<this<<' '<<this_thread::get_id()<<endl;
#endif
  blockNumber=block;
  store->file[f].seekg(BLOCKSIZE*b);
  points.clear();
  high=-INFINITY;
  low=INFINITY;
  for (i=0;i<RECORDS;i++)
  {
    pnt.read(store->file[f]);
    if (!pnt.isEmpty())
    {
      points.push_back(pnt);
      if (pnt.location.elev()>high)
	high=pnt.location.elev();
      if (pnt.location.elev()<low)
	low=pnt.location.elev();
      nPoints++;
    }
  }
  store->file[f].clear();
  /* If a new block is read in, all points will be at (0,0,0).
   * In any other case (including all points being at (NAN,NAN,NAN)),
   * points' locations will be unequal. (NAN is not equal to NAN.)
   */
  if (points.size()>1 && points[0].location==points[1].location)
  {
    points.clear();
    high=-INFINITY;
    low=INFINITY;
  }
  points.shrink_to_fit();
  if ((blockNumber>=WATCH_BLOCK_START && blockNumber<WATCH_BLOCK_END) || watchedBuffers.count(bufferNumber))
  {
    msgMutex.lock();
    cout<<"Reading block "<<blockNumber<<" buffer "<<bufferNumber<<' '<<nPoints<<" points\n";
    watchedBuffers.erase(bufferNumber);
    msgMutex.unlock();
  }
  logEndRead(bufferNumber,block);
  store->fileMutex[f].unlock();
  blockMutex.unlock();
}

void OctBuffer::update()
{
  store->nowUsedMutex.lock();
  store->nowUsed=clk.now().time_since_epoch().count()
		*chrono::steady_clock::period::num*1000
		/chrono::steady_clock::period::den;
  if (lastUsed+618<store->nowUsed)
  {
    pair<multimap<long long,int>::iterator,multimap<long long,int>::iterator> range=store->lastUsedMap.equal_range(lastUsed);
    while (range.first!=range.second && range.first->second!=bufferNumber)
      ++range.first;
    if (range.first!=range.second)
      store->lastUsedMap.erase(range.first);
    lastUsed=store->nowUsed;
    store->lastUsedMap.insert(pair<long long,int>(lastUsed,bufferNumber));
  }
  store->nowUsedMutex.unlock();
}

void OctBuffer::flush()
{
  if (dirty)
    write();
}

void OctBuffer::clear()
{
  blockMutex.lock();
  points.clear();
  blockMutex.unlock();
}

void OctBuffer::shrink()
{
  blockMutex.lock();
  points.shrink_to_fit();
  blockMutex.unlock();
}

LasPoint OctBuffer::get(xyz key)
{
  int i,inx=-1;
  LasPoint ret;
  blockMutex.lock_shared();
  for (i=0;i<points.size();i++)
  {
    if (points[i].location==key)
    {
      inx=i;
      break;
    }
  }
  if (inx>=0)
  {
    ret=points[inx];
  }
  blockMutex.unlock_shared();
  return ret;
}

bool OctBuffer::put(LasPoint pnt)
// Returns true if put, false if no room.
{
  int i,inx=-1;
  xyz key=pnt.location;
  blockMutex.lock();
  for (i=0;i<points.size();i++)
  {
    if (points[i].location==key)
    {
      inx=i;
      break;
    }
  }
  if (inx>=0)
  {
    if (points[inx].location==key && !store->ignoreDupes)
    {
      alreadyMutex.lock();
      alreadyInOctree.push_back(key);
      alreadyMutex.unlock();
    }
    markDirty();
    points[inx]=pnt;
  }
  else if (points.size()<RECORDS)
  {
    inx=points.size();
    markDirty();
    points.push_back(pnt);
    if (pnt.location.elev()>high)
      high=pnt.location.elev();
    if (pnt.location.elev()<low)
      low=pnt.location.elev();
    if (points.capacity()>RECORDS)
    {
      points.shrink_to_fit();
      points.reserve(RECORDS);
    }
  }
  blockMutex.unlock();
  return inx>=0;
}

map<int,size_t> OctBuffer::countClasses()
{
  map<int,size_t> ret;
  int i;
  for (i=0;i<points.size();i++)
    ret[points[i].classification]++;
  return ret;
}

int OctBuffer::dump(ofstream &file,Cube cube)
{
  int i,nPoints=0,nIn=0;
  xyz ctr=cube.getCenter();
  file<<'('<<ldecimal(ctr.getx())<<','<<ldecimal(ctr.gety())<<','<<ldecimal(ctr.getz())<<")±";
  file<<ldecimal(cube.getSide()/2)<<' ';
  for (i=0;i<points.size();i++)
  {
    nPoints+=points[i].location.isfinite();
    nIn+=cube.in(points[i].location);
  }
  file<<nPoints<<" points";
  if (nPoints-nIn)
    file<<", "<<nPoints-nIn<<" stray points";
  file<<endl;
  return nPoints;
}

void OctBuffer::plot(PostScript &ps,Cube cube)
{
  int i;
  xyz ctr=cube.getCenter();
  for (i=0;i<points.size();i++)
    ps.dot(points[i].location);
}

OctStore::OctStore()
{
  int i;
  nowUsed=0;
  nBlocks=0;
  ignoreDupes=false;
  for (i=0;i<9;i++)
  {
    blocks[i].store=this;
    blocks[i].bufferNumber=i;
    blocks[i].blockNumber=-1;
    blocks[i].update();
  }
}

OctStore::~OctStore()
{
  //flush();
  close();
}

void OctStore::flush(int thread,int nthreads)
{
  int i;
  OctBuffer *buf;
  assert(nthreads>0);
  for (i=thread;i<blocks.size();i+=nthreads)
  {
    assert(blocks.count(i));
    buf=&blocks[i];
    buf->flush();
  }
}

void OctStore::disown()
{
  int i,n,t=thisThread();
  OctBuffer *buf;
  for (i=0;i<ownMap[t].size();i++)
  {
    n=ownMap[t][i];
    assert(n>=0);
    bufferMutex.lock_shared();
    buf=&blocks[n];
    bufferMutex.unlock_shared();
    buf->ownMutex.lock();
    buf->owningThread.erase(t);
    buf->ownMutex.unlock();
  }
  ownMap[t].clear();
}

bool OctStore::setTransit(int buffer,bool t)
/* Returns true if successful. If t is true, and the buffer is already
 * in transit, returns false.
 */
{
  bool ret=true;
  bufferMutex.lock_shared();
  assert(buffer>=0);
  OctBuffer *buf=&blocks[buffer];
  bufferMutex.unlock_shared();
  transitMutex.lock();
  if (t)
    if (buf->inTransit)
      ret=false;
    else
      buf->inTransit=true;
  else
    buf->inTransit=false;
  transitMutex.unlock();
  return ret;
}

void OctStore::resize(int n)
// The number of blocks should be more than eight times the number of threads.
{
  int i;
  flush();
  bufferMutex.lock();
  for (i=blocks.size();i<n;i++)
  {
    blocks[i].store=this;
    blocks[i].bufferNumber=i;
    blocks[i].blockNumber=-1;
    blocks[i].update();
  }
  for (i=blocks.size()-1;i>=n;i--)
    blocks.erase(i);
  bufferMutex.unlock();
}

void OctStore::shrink()
{
  int i;
  for (i=0;i<blocks.size();i++)
    blocks[i].shrink();
}

void OctStore::open(string fileName,int numFiles)
{
  int i;
  OctBuffer *blkPtr;
  nFiles=numFiles;
  if (nFiles<1)
    nFiles=1;
  for (i=0;i<nFiles;i++)
  {
    file[i].open(fileName+to_string(i),ios::in|ios::out|ios::binary|ios::trunc);
    //cout<<fileName+to_string(i)<<' '<<file[i].is_open()<<endl;
  }
}

void OctStore::close()
{
  int i;
  for (i=0;i<nFiles;i++)
    file[i].close();
}

LasPoint OctStore::get(xyz key)
{
  int i,inx=-1;
  LasPoint ret;
  OctBuffer *pBlock=getBlock(key,false);
  assert(pBlock);
  ret=pBlock->get(key);
  if (ret.isEmpty())
  {
    cout<<"Point not found\n";
    logPointNotFound(pBlock->bufferNumber,pBlock->blockNumber,key.getx(),key.gety(),key.getz());
  }
  unlockCube();
  return ret;
}

void OctStore::put(LasPoint pnt,bool splitting)
{
  int i,inx=-1;
  int blkn0=-1,blkn1=-1,blkn2=-1;
  xyz key=pnt.location;
  if (pnt.isEmpty())
    return;
  OctBuffer *pBlock=getBlock(key,true); // Leaves cube read-locked
  assert(pBlock);
  assert(pBlock->store);
  assert(pBlock->iOwn());
  blkn0=pBlock->blockNumber;
  assert(blkn0>=0);
  if (!pBlock->put(pnt))
  {
    if (splitting)
      cout<<"split called inside split\n";
    blkn1=pBlock->blockNumber;
    split(pBlock->blockNumber,key); // Write-locks cube, then unlocks it
    pBlock=getBlock(key,true); // Read-locks a smaller cube
    pBlock->put(pnt);
  }
  blkn2=pBlock->blockNumber;
  if (!splitting)
    unlockCube();
  if (blkn1<0 && blkn0!=blkn2)
    cout<<"Block number changed\n";
}

map<int,size_t> OctStore::countClasses(long long block)
{
  return getBlock(block)->countClasses();
}

vector<LasPoint> OctStore::getAll(long long block)
{
  return getBlock(block)->getAll();
}

void OctStore::dump(ofstream &file)
{
  file<<octRoot.dump(file)<<" total points\n";
}

void OctStore::plot(PostScript &ps)
{
  ps.startpage();
  ps.setscale(octRoot.cube().minX(),octRoot.cube().minY(),
	      octRoot.cube().maxX(),octRoot.cube().maxY());
  octRoot.plot(ps);
  ps.endpage();
}

void OctStore::setIgnoreDupes(bool ig)
/* When reading the point cloud, duplicate points should be told to the user.
 * When classifying, all points put into the octree are already in the octree,
 * so ignore the duplicates.
 */
{
  ignoreDupes=ig;
}

void OctStore::dumpBuffers()
{
  int i;
  set<int>::iterator j;
  for (i=0;i<blocks.size();i++)
  {
    cout<<"buf "<<i<<" blk "<<blocks[i].blockNumber<<" lastUsed "<<blocks[i].lastUsed;
    if (blocks[i].dirty)
      cout<<" dirty";
    if (blocks[i].inTransit)
      cout<<" inTransit";
    cout<<' '<<blocks[i].points.size()<<" points";
    if (blocks[i].owningThread.size())
    {
      cout<<" owned by";
      for (j=blocks[i].owningThread.begin();j!=blocks[i].owningThread.end();++j)
	cout<<' '<<*j;
    }
    cout<<endl;
  }
}

uint64_t OctStore::countPoints()
// The count is not immediately up to date.
{
  uint64_t ret;
  uint64_t i,nblk,ngrp;
  countMutex.lock();
  if (--blockGroup>=blockGroupCount.size())
    blockGroup=blockGroupCount.size()-1;
  ngrp=blockGroupCount.size();
  nblk=blockPointCount.size();
  countMutex.unlock();
  for (ret=0,i=blockGroup*256;i<(blockGroup+1)*256 && i<nblk;++i)
    ret+=blockPointCount[i];
  if (blockGroup<ngrp)
    blockGroupCount[blockGroup]=ret;
  for (ret=i=0;i<ngrp;++i)
    ret+=blockGroupCount[i];
  return ret;
}

void OctStore::updateCount(long long block,int nPoints)
{
  uint64_t group=block/256;
  countMutex.lock();
  while (blockPointCount.size()<=block)
    blockPointCount.push_back(0);
  while (blockGroupCount.size()<=group)
    blockGroupCount.push_back(0);
  blockPointCount[block]=nPoints;
  countMutex.unlock();
}

int OctStore::leastRecentlyUsed(int thread,int nthreads)
{
  int i,age,maxAge=-1,ret=-1;
  map<long long,int>::iterator j;
  if (thread<0) // called by dump in the main thread after worker threads have finished
  {
    thread=0;
    nthreads=1;
  }
  nowUsedMutex.lock_shared();
  j=lastUsedMap.begin();
  if (lastUsedMap.size())
    for (;ret<0 && j!=lastUsedMap.end();++j)
    {
      i=j->second;
      if (!blocks[i].inTransit && blocks[i].owningThread.size()==0)
	ret=i;
    }
  else
    for (i=0;i<blocks.size();i++)
    {
      age=nowUsed-blocks[i].lastUsed;
      if (age>maxAge)
      {
	maxAge=age;
	ret=i;
      }
    }
  if (ret<0)
    dumpBuffers();
  assert(ret>=0);
  nowUsedMutex.unlock_shared();
  return ret;
}

int OctStore::newBlock()
// The new block is owned.
{
  int t=thisThread();
  bufferMutex.lock();
  int i=blocks.size();
  OctBuffer *buf=&blocks[i];
  buf->ownMutex.lock();
  bufferMutex.unlock();
  buf->store=this;
  buf->bufferNumber=i;
  buf->owningThread.insert(t);
  buf->ownMutex.unlock();
  ownMap[t].push_back(i);
  return i;
}

OctBuffer *OctStore::getBlock(long long block,bool mustExist)
{
  streampos fileSize;
  int lru=-1,bufnum=-1,i=0,f=block%nFiles,b=block/nFiles;
  bool found=false,transitResult,ownResult;
  int t=thisThread();
  int gotBlock=0;
  OctBuffer *buf;
  double fram;
  fileMutex[f].lock();
  file[f].seekg(0,file[f].end); // With more than one file, seek the file the block is in.
  fileSize=file[f].tellg();
  fileMutex[f].unlock();
  if (block>=WATCH_BLOCK_START && block<WATCH_BLOCK_END)
  {
    msgMutex.lock();
    cout<<"Getting block "<<block<<endl;
    msgMutex.unlock();
  }
  assert(block>=0);
  if (BLOCKSIZE*b>=fileSize && mustExist)
    return nullptr;
  else
  {
    revMutex.lock_shared();
    found=revBlocks.count(block);
    if (found)
    {
      bufnum=revBlocks[block];
      bufferMutex.lock_shared();
      buf=&blocks[bufnum];
      bufferMutex.unlock_shared();
      buf->own();
    }
    revMutex.unlock_shared();
    if (!found)
    {
      while (!gotBlock)
      {
	lru=leastRecentlyUsed(t,nThreads());
	bufnum=lru;
	bufferMutex.lock_shared();
	assert(bufnum>=0);
	buf=&blocks[bufnum];
	bufferMutex.unlock_shared();
	fram=freeRam();
	/* If fram>lowRam, always allocate a new buffer.
	 * If fram<lowRam but fram>lowRam/2, allocate a new buffer or reuse an old one.
	 * If fram<lowRam/2, wait until the LRU buffer can be used.
	 */
	if (fram>lowRam)
	  gotBlock=2; // new buffer
	else if (fram>lowRam/2)
	{
	  transitResult=setTransit(bufnum,true);
	  if (transitResult)
	    if (t>=0)
	      ownResult=buf->ownAlone();
	    else
	      ownResult=true;
	  else
	    ;
	  if (transitResult && ownResult)
	    gotBlock=1; // reuse buffer
	  else
	  {
	    if (transitResult)
	      setTransit(bufnum,false);
	    gotBlock=2;
	  }
	}
	else
	{
	  transitResult=setTransit(bufnum,true);
	  if (transitResult)
	    ownResult=buf->ownAlone();
	  if (transitResult && ownResult)
	    gotBlock=1;
	  else
	  {
	    if (transitResult)
	      setTransit(bufnum,false);
	    ++i;
	  }
	}
      } // while (!gotBlock)
      if (gotBlock==1)
      {
	revMutex.lock();
	revBlocks.erase(buf->blockNumber);
	revMutex.unlock();
	buf->flush();
	buf->read(block);
      }
      else
      {
	bufnum=newBlock();
	assert(bufnum>=0);
	bufferMutex.lock_shared();
	buf=&blocks[bufnum];
	bufferMutex.unlock_shared();
	buf->read(block);
	buf->blockNumber=block;
      }
      revMutex.lock();
      revBlocks[block]=bufnum;
      revMutex.unlock();
    }
    assert(bufnum>=0);
    bufferMutex.lock_shared();
    buf=&blocks[bufnum];
    bufferMutex.unlock_shared();
    buf->update();
    assert(lru>=-1);
    if (lru>=0)
      setTransit(lru,false);
    if (block>=WATCH_BLOCK_START && block<WATCH_BLOCK_END)
    {
      msgMutex.lock();
      cout<<"Got block "<<block<<" in buffer "<<bufnum<<endl;
      msgMutex.unlock();
    }
    bufferMutex.lock_shared();
    buf=&blocks[bufnum];
    bufferMutex.unlock_shared();
    assert(buf->iOwn());
    updateCount(block,buf->points.size());
    return buf;
  }
}

OctBuffer *OctStore::getBlock(xyz key,bool writing)
// Leaves the cube locked.
{
  OctBuffer *ret;
  long long blknum;
  Cube cube;
  bool gotCubeLock=false;
  while (!gotCubeLock)
  {
    setBlockMutex.lock_shared();
    cube=octRoot.findCube(key);
    setBlockMutex.unlock_shared();
    gotCubeLock=lockCube(cube);
    if (gotCubeLock)
    {
      setBlockMutex.lock_shared();
      blknum=octRoot.findBlock(key);
      setBlockMutex.unlock_shared();
    }
  }
  if (blknum>=0)
  {
    ret=getBlock(blknum);
  }
  else
  {
    setBlockMutex.lock();
    blknum=octRoot.findBlock(key);
    if (blknum<0)
    {
      blknum=nBlocks++;
      octRoot.setBlock(key,blknum);
    }
    setBlockMutex.unlock();
    ret=getBlock(blknum);
  }
  assert(ret->blockNumber>=0);
  return ret;
}

void OctStore::clearBlocks()
// Do this before clearing the octree.
{
  long long i;
  OctBuffer *buf;
  for (i=0;i<nBlocks;i++)
  {
    buf=getBlock(i);
    buf->clear();
  }
}

void OctStore::clear()
// Do this after clearing the octree.
{
  nBlocks=0;
  blockGroupCount.clear();
  blockPointCount.clear();
}

bool lowerThan(const LasPoint &a,const LasPoint &b)
{
  return a.location.getz()<b.location.getz();
}

vector<LasPoint> OctStore::pointsIn(const Shape &sh,bool sorted)
{
  vector<long long> blockList=octRoot.findBlocks(sh);
  OctBuffer *buf;
  Cube cube;
  vector<LasPoint> ret;
  int i,j;
  ret.reserve(blockList.size()*RECORDS);
  for (i=0;i<blockList.size();i++)
  {
    buf=getBlock(blockList[i]);
    if (buf->points.size())
      cube=octRoot.findCube(buf->points[0].location);
    if (sh.in(cube))
    {
      j=ret.size();
      ret.resize(j+buf->points.size());
      memmove(&ret[j],&buf->points[0],buf->points.size()*sizeof(LasPoint));
    }
    else
      for (j=0;j<buf->points.size();j++)
	if (sh.in(buf->points[j].location))
	  ret.push_back(buf->points[j]);
  }
  if (sorted)
    sort(ret.begin(),ret.end(),lowerThan);
  ret.shrink_to_fit();
  return ret;
}

uint64_t OctStore::countPointsIn(const Shape &sh)
{
  vector<long long> blockList=octRoot.findBlocks(sh);
  OctBuffer *buf;
  Cube cube;
  uint64_t ret=0;
  int i,j;
  for (i=0;i<blockList.size();i++)
  {
    buf=getBlock(blockList[i]);
    if (buf->points.size())
      cube=octRoot.findCube(buf->points[0].location);
    if (sh.in(cube))
      ret+=buf->points.size();
    else
      for (j=0;j<buf->points.size();j++)
	if (sh.in(buf->points[j].location))
	  ret++;
  }
  return ret;
}

array<double,2> OctStore::hiLoPointsIn(const Shape &sh)
/* Returns the highest and lowest elevations of the points in the shape.
 * Used to color pixels when painting the scene.
 * Caller is responsible for disowning.
 */
{
  vector<long long> blockList=octRoot.findBlocks(sh);
  OctBuffer *buf;
  double hi=-INFINITY,lo=INFINITY;
  array<double,2> ret;
  int i,j;
  for (i=0;i<blockList.size();i++)
  {
    buf=getBlock(blockList[i]);
    if (buf->getLow()<lo || buf->getHigh()>hi)
      for (j=0;j<buf->points.size();j++)
	if (sh.in(buf->points[j].location))
	{
	  if (buf->points[j].location.elev()>hi)
	    hi=buf->points[j].location.elev();
	  if (buf->points[j].location.elev()<lo)
	    lo=buf->points[j].location.elev();
	}
  }
  ret[0]=lo;
  ret[1]=hi;
  return ret;
}

void OctStore::split(long long block,xyz camelStraw)
{
  vector<LasPoint> tempPoints;
  OctBuffer *currentBlock;
  Cube thisCube;
  int i,fullth;
  bool gotCubeLock=false;
  while (!gotCubeLock)
  {
    setBlockMutex.lock_shared();
    gotCubeLock=lockCube(thisCube=octRoot.findCube(camelStraw));
    setBlockMutex.unlock_shared();
    if (!gotCubeLock)
    {
      unlockCube();
      sleepDead(thisThread());
    }
  }
  currentBlock=getBlock(block);
  logBeginSplit(currentBlock->bufferNumber,block);
#if DEBUG_STORE
  cout<<"Splitting block "<<block<<endl;
#endif
  fullth=0;
  currentBlock->blockMutex.lock();
  tempPoints=currentBlock->points;
  currentBlock->markDirty();
  for (i=0;i<currentBlock->points.size();i++)
  {
    if (currentBlock->points[i].location.isfinite())
      ++fullth;
  }
  currentBlock->points.clear();
  currentBlock->high=-INFINITY;
  currentBlock->low=INFINITY;
  currentBlock->blockMutex.unlock();
  octRoot.split(camelStraw);
  embufferPoints(tempPoints,thisThread());
  logEndSplit(currentBlock->bufferNumber,block);
#if DEBUG_STORE
  cout<<"Block "<<block<<" is split\n";
#endif
  unlockCube();
}
