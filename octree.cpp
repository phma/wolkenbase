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
#include "octree.h"
using namespace std;

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
  for (i=0;i<9;i++)
  {
    blocks[i].store=this;
    blocks[i].blockNumber=-1;
  }
}

OctStore::~OctStore()
{
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
