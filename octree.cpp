/******************************************************/
/*                                                    */
/* octree.cpp - octrees                               */
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
#include <cassert>
#include <cmath>
#include "octree.h"
#define DEBUG_STORE 0
#define DEBUG_LOCK 0
using namespace std;

Octree octRoot;
OctStore octStore;

#if defined(_WIN32) || defined(__CYGWIN__)
// Linux and BSD have this function in the library; Windows doesn't.
double significand(double x)
{
  int dummy;
  return frexp(x,&dummy)*2;
}
#endif

long long Octree::findBlock(xyz pnt)
// Returns the disk block number that contains pnt, or -1 if none.
{
  int xbit,ybit,zbit,i;
  xbit=pnt.getx()>=center.getx();
  ybit=pnt.gety()>=center.gety();
  zbit=pnt.getz()>=center.getz();
  i=zbit*4+ybit*2+xbit;
  if (sub[i]==0)
    return -1;
  else if (sub[i]&1)
    return sub[i]>>1;
  else
    return ((Octree *)sub[i])->findBlock(pnt);
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
  }
  else
    ((Octree *)sub[i])->split(pnt);
}

OctBuffer::OctBuffer()
{
  int i;
  lastUsed=0;
  dirty=false;
  owningThread=-1;
  for (i=0;i<RECORDS;i++)
    points.emplace_back();
}

void OctBuffer::write()
{
  int i;
  blockMutex.lock_shared();
  store->fileMutex.lock();
#if DEBUG_STORE
  cout<<"Writing block "<<blockNumber<<endl;
#endif
  store->file.seekp(BLOCKSIZE*blockNumber);
  //cout<<store->file.rdstate()<<' '<<ios::failbit<<endl;
  store->file.clear();
  for (i=0;i<RECORDS;i++)
    points[i].write(store->file);
  dirty=false;
  store->fileMutex.unlock();
  blockMutex.unlock_shared();
}

void OctBuffer::markDirty()
{
  dirty=true;
}

bool OctBuffer::own()
{
  bool ret;
  int t=thisThread();
  store->ownMutex.lock();
  ret=owningThread<0 || owningThread==t;
  if (ret)
    owningThread=t;
  store->ownMutex.unlock();
  return ret;
}

void OctBuffer::read(long long block)
{
  int i;
  blockMutex.lock();
  store->fileMutex.lock();
#if DEBUG_STORE
  cout<<"Reading block "<<block<<" into "<<this<<' '<<this_thread::get_id()<<endl;
#endif
  store->file.seekg(BLOCKSIZE*(blockNumber=block));
  for (i=0;i<RECORDS;i++)
    points[i].read(store->file);
  store->file.clear();
  /* If a new block is read in, all points will be at (0,0,0).
   * In any other case (including all points being at (NAN,NAN,NAN)),
   * points' locations will be unequal. (NAN is not equal to NAN.)
   */
  if (points[0].location==points[1].location)
    for (i=0;i<RECORDS;i++)
      points[i].location=nanxyz;
  store->fileMutex.unlock();
  blockMutex.unlock();
}

void OctBuffer::update()
{
  store->nowUsedMutex.lock();
  lastUsed=++(store->nowUsed);
  store->nowUsedMutex.unlock();
}

void OctBuffer::flush()
{
  if (dirty)
    write();
}

LasPoint OctBuffer::get(xyz key)
{
  int i,inx=-1;
  LasPoint ret;
  blockMutex.lock_shared();
  for (i=0;i<2*RECORDS;i++)
    if (points[i%RECORDS].location==key || (i>=RECORDS && points[i%RECORDS].isEmpty()))
    {
      inx=i%RECORDS;
      break;
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
  for (i=0;i<2*RECORDS;i++)
    if (points[i%RECORDS].location==key || (i>=RECORDS && points[i%RECORDS].isEmpty()))
    {
      inx=i%RECORDS;
      break;
    }
  if (inx>=0)
  {
    markDirty();
    points[inx]=pnt;
  }
  blockMutex.unlock();
  return inx>=0;
}
  
OctStore::OctStore()
{
  int i;
  nowUsed=0;
  nBlocks=0;
  for (i=0;i<9;i++)
  {
    blocks[i].store=this;
    blocks[i].blockNumber=-1;
  }
}

OctStore::~OctStore()
{
  flush();
  close();
}

void OctStore::flush()
{
  int i;
  for (i=0;i<blocks.size();i++)
  {
    blocks[i].flush();
  }
}

void OctStore::disown()
{
  int i,t=thisThread();
  ownMutex.lock();
  for (i=0;i<blocks.size();i++)
  {
    if (blocks[i].owningThread==t)
      blocks[i].owningThread=-1;
  }
  ownMutex.unlock();
}

void OctStore::resize(int n)
// The number of blocks should be more than eight times the number of threads.
{
  int i;
  flush();
  for (i=blocks.size();i<n;i++)
  {
    blocks[i].store=this;
    blocks[i].blockNumber=-1;
  }
  for (i=blocks.size()-1;i>=n;i--)
    blocks.erase(i);
}

void OctStore::open(string fileName)
{
  int i;
  OctBuffer *blkPtr;
  file.open(fileName,ios::in|ios::out|ios::binary|ios::trunc);
  cout<<fileName<<' '<<file.is_open()<<endl;
}

void OctStore::close()
{
  flush();
  file.close();
}

LasPoint OctStore::get(xyz key)
{
  int i,inx=-1;
  LasPoint ret;
  OctBuffer *pBlock=getBlock(key,false); // locks the mutex
  assert(pBlock);
  ret=pBlock->get(key);
  unlockBlockR(pBlock->blockNumber);
#if DEBUG_LOCK
  if (thisThread()==0) cout<<"reggle "<<pBlock->blockNumber<<" get\n";
#endif
  return ret;
}

void OctStore::put(LasPoint pnt)
{
  int i,inx=-1;
  int blkn0=-1,blkn1=-1,blkn2=-1;
  xyz key=pnt.location;
  OctBuffer *pBlock=getBlock(key,true); // locks the mutex
  assert(pBlock);
  blkn0=pBlock->blockNumber;
  if (!pBlock->put(pnt))
  {
    unlockBlockW(pBlock->blockNumber);
#if DEBUG_LOCK
    if (thisThread()==0) cout<<"weggle "<<pBlock->blockNumber<<" put1\n";
#endif
    blkn1=pBlock->blockNumber;
    split(pBlock->blockNumber,key);
    pBlock=getBlock(key,true);
    pBlock->put(pnt);
  }
  blkn2=pBlock->blockNumber;
  unlockBlockW(pBlock->blockNumber);
#if DEBUG_LOCK
  if (thisThread()==0) cout<<"weggle "<<pBlock->blockNumber<<" put2\n";
#endif
}

int OctStore::leastRecentlyUsed()
{
  int i,age,maxAge=-1,ret;
  nowUsedMutex.lock_shared();
  for (i=0;i<blocks.size();i++)
  {
    age=nowUsed-blocks[i].lastUsed;
    if (age>maxAge)
    {
      maxAge=age;
      ret=i;
    }
  }
  nowUsedMutex.unlock_shared();
  return ret;
}

int OctStore::newBlock()
// The new block is owned.
{
  ownMutex.lock();
  int i=blocks.size();
  blocks[i].store=this;
  blocks[i].owningThread=thisThread();
  ownMutex.unlock();
  return i;
}

OctBuffer *OctStore::getBlock(long long block,bool mustExist)
{
  streampos fileSize;
  int lru,i;
  bool found=false;
  fileMutex.lock();
  file.seekg(0,file.end);
  fileSize=file.tellg();
  fileMutex.unlock();
  lru=leastRecentlyUsed();
  if (BLOCKSIZE*block>=fileSize && mustExist)
    return nullptr;
  else
  {
    for (i=0;i<blocks.size();i++)
      if (blocks[i].blockNumber==block)
      {
        found=true;
        lru=i;
      }
    if (!found)
    {
      int oldblock=blocks[lru].blockNumber;
      if (blocks[lru].own())
      {
	if (oldblock>=0)
	{
#if DEBUG_LOCK
	  if (thisThread()==0) cout<<"roggle "<<oldblock<<" getBlock0\n";
#endif
	  lockBlockR(oldblock);
	}
	blocks[lru].flush();
	blocks[lru].read(block);
	if (oldblock>=0)
	{
	  unlockBlockR(oldblock);
#if DEBUG_LOCK
	  if (thisThread()==0) cout<<"reggle "<<oldblock<<" getBlock0\n";
#endif
	}
      }
      else
	lru=newBlock();
    }
    blocks[lru].update();
    return &blocks[lru];
  }
}

OctBuffer *OctStore::getBlock(xyz key,bool writing)
{
  OctBuffer *ret;
  setBlockMutex.lock_shared();
  long long blknum=octRoot.findBlock(key);
  setBlockMutex.unlock_shared();
  if (blknum>=0)
  {
    if (writing)
    {
      lockBlockW(blknum);
#if DEBUG_LOCK
    if (thisThread()==0) cout<<"woggle "<<blknum<<" getBlock1\n";
#endif
    }
    else
    {
      lockBlockR(blknum);
#if DEBUG_LOCK
    if (thisThread()==0) cout<<"roggle "<<blknum<<" getBlock1\n";
#endif
    }
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
    if (writing)
    {
      lockBlockW(blknum);
#if DEBUG_LOCK
      if (thisThread()==0) cout<<"woggle "<<blknum<<" getBlock2\n";
#endif
    }
    else
    {
      lockBlockR(blknum);
#if DEBUG_LOCK
      if (thisThread()==0) cout<<"roggle "<<blknum<<" getBlock2\n";
#endif
    }
    ret=getBlock(blknum);
    setBlockMutex.unlock();
  }
  return ret;
}

void OctStore::split(long long block,xyz camelStraw)
{
  vector<LasPoint> tempPoints;
  OctBuffer *currentBlock;
  int i,fullth;
  currentBlock=getBlock(block);
  splitMutex.lock();
#if DEBUG_STORE
  cout<<"Splitting block "<<block<<endl;
#endif
  tempPoints=currentBlock->points;
  currentBlock->markDirty();
  while (true)
  {
    lockBlockW(block);
#if DEBUG_LOCK
    if (thisThread()==0) cout<<"woggle "<<block<<" split\n";
#endif
    fullth=0;
    for (i=0;i<RECORDS;i++)
    {
      if (currentBlock->points[i].location.isfinite())
	++fullth;
      currentBlock->points[i].location=nanxyz;
    }
    unlockBlockW(block);
#if DEBUG_LOCK
    if (thisThread()==0) cout<<"weggle "<<block<<" split\n";
#endif
    if (fullth<RECORDS)
      break;
    octRoot.split(camelStraw);
    for (i=0;i<RECORDS;i++)
      put(tempPoints[i]);
  }
#if DEBUG_STORE
  cout<<"Block "<<block<<" is split\n";
#endif
  splitMutex.unlock();
}
