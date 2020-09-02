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
#include "ldecimal.h"
#include "octree.h"
#include "freeram.h"
#define DEBUG_STORE 0
#define DEBUG_LOCK 0
#define WATCH_BLOCK_START 0
#define WATCH_BLOCK_END 10
using namespace std;

Octree octRoot;
OctStore octStore;
double lowRam;

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
  uintptr_t subi;
  xbit=pnt.getx()>=center.getx();
  ybit=pnt.gety()>=center.gety();
  zbit=pnt.getz()>=center.getz();
  i=zbit*4+ybit*2+xbit;
  subi=sub[i];
  if (subi==0)
    return -1;
  else if (subi&1)
    return subi>>1;
  else
    return ((Octree *)subi)->findBlock(pnt);
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
      totalPoints+=octStore.getBlock(subi>>1)->dump(file,cube(i));
    else if (subi)
      totalPoints+=((Octree *)subi)->dump(file);
  }
  return totalPoints;
}

OctBuffer::OctBuffer()
{
  int i;
  lastUsed=0;
  dirty=inTransit=false;
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
#if DEBUG_STORE
  cout<<"Writing block "<<blockNumber<<endl;
#endif
  store->file[f].seekp(BLOCKSIZE*b);
  //cout<<store->file.rdstate()<<' '<<ios::failbit<<endl;
  store->file[f].clear();
  for (i=0;i<RECORDS;i++)
  {
    points[i].write(store->file[f]);
    nPoints+=!points[i].isEmpty();
  }
  dirty=false;
  for (i=RECORDS*LASPOINT_SIZE;i<BLOCKSIZE;i++)
    store->file[f].put(0);
  if (blockNumber>=WATCH_BLOCK_START && blockNumber<WATCH_BLOCK_END)
    cout<<"Writing block "<<blockNumber<<' '<<nPoints<<" points\n";
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
  store->ownMutex.lock();
  owningThread.insert(t);
  store->ownMap[t].push_back(bufferNumber);
  store->ownMutex.unlock();
}

bool OctBuffer::ownAlone()
{
  bool ret;
  int t=thisThread();
  store->ownMutex.lock();
  ret=owningThread.size()==owningThread.count(t);
  if (ret)
  {
    owningThread.insert(t);
    store->ownMap[t].push_back(bufferNumber);
  }
  store->ownMutex.unlock();
  return ret;
}

void OctBuffer::read(long long block)
{
  int i,f=block%store->nFiles,b=block/store->nFiles;
  int nPoints=0;
  blockMutex.lock();
  store->fileMutex[f].lock();
#if DEBUG_STORE
  cout<<"Reading block "<<block<<" into "<<this<<' '<<this_thread::get_id()<<endl;
#endif
  blockNumber=block;
  store->file[f].seekg(BLOCKSIZE*b);
  for (i=0;i<RECORDS;i++)
  {
    points[i].read(store->file[f]);
    nPoints+=!points[i].isEmpty();
  }
  store->file[f].clear();
  /* If a new block is read in, all points will be at (0,0,0).
   * In any other case (including all points being at (NAN,NAN,NAN)),
   * points' locations will be unequal. (NAN is not equal to NAN.)
   */
  if (points[0].location==points[1].location)
    for (i=0;i<RECORDS;i++)
      points[i].location=nanxyz;
  if (blockNumber>=WATCH_BLOCK_START && blockNumber<WATCH_BLOCK_END)
    cout<<"Reading block "<<blockNumber<<' '<<nPoints<<" points\n";
  store->fileMutex[f].unlock();
  blockMutex.unlock();
}

void OctBuffer::update()
{
  store->nowUsedMutex.lock();
  store->lastUsedMap.erase(lastUsed);
  lastUsed=++(store->nowUsed);
  store->lastUsedMap[lastUsed]=bufferNumber;
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

int OctBuffer::dump(ofstream &file,Cube cube)
{
  int i,nPoints=0,nIn=0;
  xyz ctr=cube.getCenter();
  file<<'('<<ldecimal(ctr.getx())<<','<<ldecimal(ctr.gety())<<','<<ldecimal(ctr.getz())<<")±";
  file<<ldecimal(cube.getSide()/2)<<' ';
  for (i=0;i<RECORDS;i++)
  {
    nPoints+=points[i].location.isfinite();
    nIn+=cube.in(points[i].location);
  }
  file<<nPoints<<" points";
  if (nPoints-nIn)
    file<<nPoints-nIn<<", stray points";
  file<<endl;
  return nPoints;
}

OctStore::OctStore()
{
  int i;
  nowUsed=0;
  nBlocks=0;
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
  flush();
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
  ownMutex.lock();
  for (i=0;i<ownMap[t].size();i++)
  {
    bufferMutex.lock_shared();
    n=ownMap[t][i];
    assert(n>=0);
    blocks[n].owningThread.erase(t);
    bufferMutex.unlock_shared();
  }
  ownMap[t].clear();
  ownMutex.unlock();
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
  flush();
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
  return ret;
}

void OctStore::put(LasPoint pnt)
{
  int i,inx=-1;
  int blkn0=-1,blkn1=-1,blkn2=-1;
  xyz key=pnt.location;
  OctBuffer *pBlock=getBlock(key,true);
  assert(pBlock);
  blkn0=pBlock->blockNumber;
  assert(blkn0>=0);
  if (!pBlock->put(pnt))
  {
    blkn1=pBlock->blockNumber;
    split(pBlock->blockNumber,key);
    pBlock=getBlock(key,true);
    pBlock->put(pnt);
  }
  blkn2=pBlock->blockNumber;
  if (blkn1<0 && blkn0!=blkn2)
    cout<<"Block number changed\n";
}

void OctStore::dump(ofstream &file)
{
  file<<octRoot.dump(file)<<" total points\n";
}

int OctStore::leastRecentlyUsed()
{
  int i,age,maxAge=-1,ret;
  map<long long,int>::iterator j;
  nowUsedMutex.lock_shared();
  j=lastUsedMap.begin();
  if (lastUsedMap.size())
    ret=j->second;
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
  nowUsedMutex.unlock_shared();
  return ret;
}

int OctStore::newBlock()
// The new block is owned.
{
  bufferMutex.lock();
  int i=blocks.size();
  OctBuffer *buf=&blocks[i];
  bufferMutex.unlock();
  ownMutex.lock();
  buf->store=this;
  buf->bufferNumber=i;
  buf->owningThread.insert(thisThread());
  ownMap[thisThread()].push_back(i);
  ownMutex.unlock();
  return i;
}

OctBuffer *OctStore::getBlock(long long block,bool mustExist)
{
  streampos fileSize;
  int lru=-1,bufnum=-1,i=0,f=block%nFiles,b=block/nFiles;
  bool found=false,transitResult,ownResult;
  int gotBlock=0;
  OctBuffer *buf;
  double fram;
  fileMutex[f].lock();
  file[f].seekg(0,file[f].end); // With more than one file, seek the file the block is in.
  fileSize=file[f].tellg();
  fileMutex[f].unlock();
  assert(block>=0);
  if (BLOCKSIZE*b>=fileSize && mustExist)
    return nullptr;
  else
  {
    revMutex.lock_shared();
    found=revBlocks.count(block);
    if (found)
      bufnum=revBlocks[block];
    revMutex.unlock_shared();
    if (!found)
    {
      while (!gotBlock)
      {
	lru=(leastRecentlyUsed()+i)%blocks.size();
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
	  gotBlock=2;
	else if (fram>lowRam/2)
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
      }
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
	blocks[bufnum].blockNumber=block;
      }
      revMutex.lock();
      revBlocks[block]=bufnum;
      revMutex.unlock();
    }
    assert(bufnum>=0);
    blocks[bufnum].update();
    assert(lru>=-1);
    if (lru>=0)
      setTransit(lru,false);
    return &blocks[bufnum];
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
    ret=getBlock(blknum);
    setBlockMutex.unlock();
  }
  assert(ret->blockNumber>=0);
  return ret;
}

void OctStore::split(long long block,xyz camelStraw)
{
  vector<LasPoint> tempPoints;
  OctBuffer *currentBlock;
  int i,fullth;
  currentBlock=getBlock(block);
#if DEBUG_STORE
  cout<<"Splitting block "<<block<<endl;
#endif
  while (true)
  {
    fullth=0;
    currentBlock->blockMutex.lock();
    tempPoints=currentBlock->points;
    currentBlock->markDirty();
    for (i=0;i<RECORDS;i++)
    {
      if (currentBlock->points[i].location.isfinite())
	++fullth;
      currentBlock->points[i].location=nanxyz;
    }
    currentBlock->blockMutex.unlock();
    if (fullth<RECORDS)
      break;
    octRoot.split(camelStraw);
    for (i=0;i<RECORDS;i++)
      put(tempPoints[i]);
  }
  for (i=0;i<RECORDS;i++)
    put(tempPoints[i]);
#if DEBUG_STORE
  cout<<"Block "<<block<<" is split\n";
#endif
}
