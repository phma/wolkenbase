/******************************************************/
/*                                                    */
/* octree.cpp - octrees                               */
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
#include <cassert>
#include <cmath>
#include "octree.h"
#define DEBUG_STORE 1
using namespace std;

Octree octRoot;

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

OctBlock::OctBlock()
{
  int i;
  lastUsed=0;
  dirty=false;
  for (i=0;i<RECORDS;i++)
    points.emplace_back(this);
}

void OctBlock::write()
{
  int i;
#if DEBUG_STORE
  cout<<"Writing block "<<blockNumber<<endl;
#endif
  store->file.seekp(BLOCKSIZE*blockNumber);
  //cout<<store->file.rdstate()<<' '<<ios::failbit<<endl;
  store->file.clear();
  for (i=0;i<RECORDS;i++)
    points[i].write(store->file);
  dirty=false;
}

void OctBlock::markDirty()
{
  dirty=true;
}

void OctBlock::read(long long block)
{
  int i;
#if DEBUG_STORE
  cout<<"Reading block "<<block<<endl;
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
}

void OctBlock::update()
{
  lastUsed=++(store->nowUsed);
}

void OctBlock::flush()
{
  if (dirty)
    write();
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

void OctStore::open(string fileName)
{
  int i;
  OctBlock *blkPtr;
  file.open(fileName,ios::in|ios::out|ios::binary|ios::trunc);
  cout<<fileName<<' '<<file.is_open()<<endl;
}

void OctStore::close()
{
  flush();
  file.close();
}

LasPoint &OctStore::operator[](xyz key)
{
  int i,inx=-1;
  OctBlock *pBlock=getBlock(key);
  assert(pBlock);
  for (i=0;i<2*RECORDS;i++)
    if (pBlock->points[i%RECORDS].location==key || (i>=RECORDS && pBlock->points[i%RECORDS].isEmpty()))
    {
      inx=i%RECORDS;
      break;
    }
  if (i>=RECORDS)
    pBlock->markDirty();
  if (inx<0)
  {
    split(pBlock->blockNumber,key);
    pBlock=getBlock(key);
    for (i=0;i<RECORDS;i++)
      if (pBlock->points[i%RECORDS].isEmpty())
      {
        inx=i;
	pBlock->markDirty();
        break;
      }
  }
  return pBlock->points[inx];
}

int OctStore::leastRecentlyUsed()
{
  int i,age,maxAge=-1,ret;
  for (i=0;i<blocks.size();i++)
  {
    age=nowUsed-blocks[i].lastUsed;
    if (age>maxAge)
    {
      maxAge=age;
      ret=i;
    }
  }
  return ret;
}

OctBlock *OctStore::getBlock(long long block,bool mustExist)
{
  streampos fileSize;
  int lru,i;
  bool found=false;
  file.seekg(0,file.end);
  fileSize=file.tellg();
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
      blocks[lru].flush();
      blocks[lru].read(block);
    }
    blocks[lru].update();
    return &blocks[lru];
  }
}

OctBlock *OctStore::getBlock(xyz key)
{
  long long blknum=octRoot.findBlock(key);
  if (blknum>=0)
    return getBlock(blknum);
  else
  {
    blknum=nBlocks++;
    octRoot.setBlock(key,blknum);
    return getBlock(blknum);
  }
}

void OctStore::split(long long block,xyz camelStraw)
{
  vector<LasPoint> tempPoints;
  OctBlock *currentBlock;
  int i,fullth;
  currentBlock=getBlock(block);
  tempPoints=currentBlock->points;
  currentBlock->markDirty();
  while (true)
  {
    fullth=0;
    for (i=0;i<RECORDS;i++)
    {
      if (currentBlock->points[i].location.isfinite())
	++fullth;
      currentBlock->points[i].location=nanxyz;
    }
    if (fullth<RECORDS)
      break;
    octRoot.split(camelStraw);
    for (i=0;i<RECORDS;i++)
      (*this)[tempPoints[i].location]=tempPoints[i];
  }
}
