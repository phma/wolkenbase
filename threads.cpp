/******************************************************/
/*                                                    */
/* threads.cpp - multithreading                       */
/*                                                    */
/******************************************************/
/* Copyright 2020-2022 Pierre Abbat.
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
#include <queue>
#include <set>
#include <cassert>
#include <climits>
#include <atomic>
#include "testpattern.h"
#include "threads.h"
#include "angle.h"
#include "random.h"
#include "relprime.h"
#include "brevno.h"
#include "las.h"
#include "cloud.h"
#include "freeram.h"
#include "octree.h"
#include "fileio.h"
#include "scan.h"
#include "classify.h"

using namespace std;
namespace cr=std::chrono;

#define CHUNKSIZE 137

mutex actMutex;
mutex startMutex;
map<int,mutex> pointBufferMutex,cubeMutex;
shared_mutex threadStatusMutex;
mutex tileDoneMutex;
mutex classTotalMutex;

atomic<int> threadCommand;
vector<thread> threads;
vector<int> threadStatus; // Bit 8 indicates whether the thread is sleeping.
vector<double> sleepTime;
double stageTolerance;
double minArea;
queue<ThreadAction> actQueue,resQueue;
queue<Eisenstein> tileDoneQueue;
map<int,vector<LasPoint> > pointBuffer;
map<int,size_t> pbsz,classTotals;
map<int,int> bufferPos;
map<int,map<int,Cube> > lockedCubes,readLockedCubes;
vector<Cube> heldCubes; // Used in unlockCube to know which mutexes to get
int currentAction;
map<thread::id,int> threadNums;
Flowsnake snake;
#ifdef GUI
CloudOutput cloudOutput;
#endif

cr::steady_clock clk;
const char statusNames[][8]=
{
  "None","Run","Pause","Wait","Stop"
};

double busyFraction()
{
  int i,numBusy=0;
  threadStatusMutex.lock_shared();
  for (i=0;i<threadStatus.size();i++)
    if ((threadStatus[i]&256)==0)
      numBusy++;
  threadStatusMutex.unlock_shared();
  return (double)numBusy/i;
}

void startThreads(int n)
{
  int i,m;
  threadCommand=TH_WAIT;
  openThreadLog();
  logStartThread();
  sleepTime.resize(n);
  m=n*3;
  threadNums[this_thread::get_id()]=-1;
  heldCubes.resize(n);
  for (i=0;i<m;i++)
  {
    cubeMutex[i];
    lockedCubes[i];
    readLockedCubes[i];
  }
  for (i=0;i<n;i++)
  {
    threads.push_back(thread(WolkenThread(),i));
    sleepTime[i]=i+1;
    this_thread::sleep_for(chrono::milliseconds(10));
  }
}

void joinThreads()
{
  int i;
  for (i=0;i<threads.size();i++)
    threads[i].join();
}

ThreadAction dequeueAction()
{
  ThreadAction ret;
  ret.opcode=0;
  actMutex.lock();
  if (actQueue.size())
  {
    ret=actQueue.front();
    actQueue.pop();
    if (ret.opcode)
      currentAction=ret.opcode;
  }
  actMutex.unlock();
  return ret;
}

void enqueueAction(ThreadAction a)
{
  actMutex.lock();
  actQueue.push(a);
  actMutex.unlock();
}

bool actionQueueEmpty()
{
  return actQueue.size()==0;
}

Eisenstein dequeueTileDone()
{
  Eisenstein ret(INT_MIN,INT_MIN);
  tileDoneMutex.lock();
  if (tileDoneQueue.size())
  {
    ret=tileDoneQueue.front();
    tileDoneQueue.pop();
  }
  tileDoneMutex.unlock();
  return ret;
}

void enqueueTileDone(Eisenstein e)
{
  tileDoneMutex.lock();
  tileDoneQueue.push(e);
  tileDoneMutex.unlock();
}

bool tileDoneQueueEmpty()
{
  return tileDoneQueue.size()==0;
}

ThreadAction dequeueResult()
{
  ThreadAction ret;
  ret.opcode=0;
  actMutex.lock();
  if (resQueue.size())
  {
    ret=resQueue.front();
    resQueue.pop();
  }
  actMutex.unlock();
  return ret;
}

void enqueueResult(ThreadAction a)
{
  actMutex.lock();
  resQueue.push(a);
  actMutex.unlock();
}

bool resultQueueEmpty()
{
  return resQueue.size()==0;
}

void embufferPoint(LasPoint point,bool fromFile)
{
  int thread;
  static int anyThread=0;
  octStore.setBlockMutex.lock_shared();
  thread=octRoot.findBlock(point.location);
  octStore.setBlockMutex.unlock_shared();
  if (thread<0)
  {
    thread=anyThread;
    anyThread--;
    if (anyThread<0)
      anyThread+=threadStatus.size();
  }
  thread%=threadStatus.size();
  if (!point.isEmpty())
  {
    pointBufferMutex[thread].lock();
    pointBuffer[thread].push_back(point);
    pbsz[thread]=pointBuffer[thread].size();
    bufferPos[thread]=(bufferPos[thread]+relprime(pbsz[thread]))%pbsz[thread];
    swap(pointBuffer[thread].back(),pointBuffer[thread][bufferPos[thread]]);
    pointBufferMutex[thread].unlock();
  }
  while (fromFile && pointBufferSize()*sizeof(point)>lowRam)
    this_thread::sleep_for(chrono::milliseconds(1));
}

void embufferPoints(vector<LasPoint> points,int thread)
/* Puts a whole block of points from a split block at the end of the thread's
 * buffer, without shuffling. Usually, most of them will be reembuffered to
 * other threads' buffers.
 */
{
  int i;
  if (thread<0)
    thread+=threadStatus.size();
  pointBufferMutex[thread].lock();
  pointBuffer[thread].reserve(pointBuffer[thread].size()+points.size());
  for (i=0;i<points.size();i++)
    pointBuffer[thread].push_back(points[i]);
  pbsz[thread]=pointBuffer[thread].size();
  pointBufferMutex[thread].unlock();
}

LasPoint debufferPoint(int thread)
{
  LasPoint ret;
  pointBufferMutex[thread].lock();
  if (pointBuffer[thread].size())
  {
    ret=pointBuffer[thread].back();
    pointBuffer[thread].pop_back();
    if (ret.isEmpty())
      cout<<"Debuffered empty point\n";
    if (pointBuffer[thread].capacity()>2*pointBuffer[thread].size())
      pointBuffer[thread].shrink_to_fit();
  }
  pbsz[thread]=pointBuffer[thread].size();
  pointBufferMutex[thread].unlock();
  return ret;
}

size_t pointBufferSize()
{
  size_t sum=0,i;
  for (i=0;i<pointBuffer.size();i++)
  {
    sum+=pbsz[i];
  }
  return sum;
}

bool pointBuffersNonempty()
{
  bool ret=true;
  int i;
  for (i=0;ret && i<pointBuffer.size();i++)
  {
    ret=ret && pbsz[i];
  }
  return ret;
}

bool pointBufferEmpty()
{
  return pointBufferSize()==0;
}

void sleep(int thread)
{
  sleepTime[thread]*=1.0625;
  if (sleepTime[thread]>1e5)
    sleepTime[thread]*=0.9375;
  threadStatusMutex.lock();
  threadStatus[thread]|=256;
  threadStatusMutex.unlock();
  this_thread::sleep_for(chrono::microseconds(lrint(sleepTime[thread])));
  threadStatusMutex.lock();
  threadStatus[thread]&=255;
  threadStatusMutex.unlock();
}

void sleepDead(int thread)
// Sleep to get out of deadlock.
{
  sleepTime[thread]*=1.0625;
  if (sleepTime[thread]>1e5)
    sleepTime[thread]*=0.9375;
  threadStatusMutex.lock();
  threadStatus[thread]|=256;
  threadStatusMutex.unlock();
  this_thread::sleep_for(chrono::microseconds(lrint(sleepTime[thread])));
  threadStatusMutex.lock();
  threadStatus[thread]&=255;
  threadStatusMutex.unlock();
}

void unsleep(int thread)
{
  sleepTime[thread]*=0.9375;
  if (sleepTime[thread]<1)
    sleepTime[thread]*=1.125;
  if (sleepTime[thread]<0 || std::isnan(sleepTime[thread]))
    sleepTime[thread]=1;
}

double maxSleepTime()
{
  int i;
  double max=0;
  for (i=0;i<sleepTime.size();i++)
    if (sleepTime[i]>max)
      max=sleepTime[i];
  return max;
}

void setThreadCommand(int newStatus)
{
  threadCommand=newStatus;
  //cout<<statusNames[newStatus]<<endl;
}

int getThreadCommand()
{
  return threadCommand;
}

int getThreadStatus()
/* Returns aaaaaaaaaabbbbbbbbbbcccccccccc where
 * aaaaaaaaaa is the status all threads should be in,
 * bbbbbbbbbb is 0 if all threads are in the same state, and
 * cccccccccc is the state the threads are in.
 * If all threads are in the commanded state, but some may be asleep and others awake,
 * getThreadStatus()&0x3ffbfeff is a multiple of 1048577.
 */
{
  int i,oneStatus,minStatus=-1,maxStatus=0;
  threadStatusMutex.lock_shared();
  for (i=0;i<threadStatus.size();i++)
  {
    oneStatus=threadStatus[i];
    maxStatus|=oneStatus;
    minStatus&=oneStatus;
  }
  threadStatusMutex.unlock_shared();
  return (threadCommand<<20)|((minStatus^maxStatus)<<10)|(minStatus&0x3ff);
}

void setMinArea(double area)
{
  minArea=area;
}

void waitForThreads(int newStatus)
// Waits until all threads are in the commanded status.
{
  int i,n;
  threadCommand=newStatus;
  do
  {
    threadStatusMutex.lock_shared();
    for (i=n=0;i<threadStatus.size();i++)
      if ((threadStatus[i]&255)!=threadCommand)
	n++;
    threadStatusMutex.unlock_shared();
    this_thread::sleep_for(chrono::milliseconds(n));
  } while (n);
}

void waitForQueueEmpty()
// Waits until the action queue and point buffer are empty and all threads have completed their actions.
{
  int i,n;
  do
  {
    n=actQueue.size()+pointBufferSize();
    threadStatusMutex.lock_shared();
    for (i=0;i<threadStatus.size();i++)
      if (threadStatus[i]<256)
	n++;
    threadStatusMutex.unlock_shared();
    //if (freeRam()<lowRam/4)
      //cout<<"aoeu\n";
    cout<<n<<"    \r";
    cout.flush();
    writeBufLog();
    this_thread::sleep_for(chrono::milliseconds(30));
  } while (n);
}

int thisThread()
{
  return threadNums[this_thread::get_id()];
}

int nThreads()
// Returns number of worker threads, not counting main thread.
{
  return threadStatus.size();
}

void countClasses(int thread)
{
  map<int,size_t> threadTotals,blockCounts;
  int i;
  map<int,size_t>::iterator j;
  if (thread==0) // Count points read in from XYZ or PLY as raw in LASify
    threadTotals[0]=cloud.size();
  for (i=thread;i<octStore.getNumBlocks();i+=nThreads())
  {
    blockCounts=octStore.countClasses(i);
    for (j=blockCounts.begin();j!=blockCounts.end();++j)
      threadTotals[j->first]+=j->second;
  }
  while (actQueue.size())
    sleep(thread); // Make sure that each thread runs one countClasses instance
  classTotalMutex.lock();
  for (j=threadTotals.begin();j!=threadTotals.end();++j)
    classTotals[j->first]+=j->second;
  classTotalMutex.unlock();
}

void WolkenThread::operator()(int thread)
{
  long long h=0,i=0,j=0,n,nPoints=0,nChunks;
  long long blknum;
  bool dropZeros;
  xyz offset,scale;
  ThreadAction act;
  BoundRect br;
  LasPoint point,gotPoint;
  Eisenstein cylAddress;
  logStartThread();
  startMutex.lock();
  if (threadStatus.size()!=thread)
  {
    cout<<"Starting thread "<<threadStatus.size()<<", was passed "<<thread<<endl;
    thread=threadStatus.size();
  }
  threadStatus.push_back(0);
  threadNums[this_thread::get_id()]=thread;
  pointBuffer[thread];
  startMutex.unlock();
  while (threadCommand!=TH_STOP)
  {
    if (threadCommand==TH_READ)
    { // The threads are reading the input files.
      threadStatusMutex.lock();
      threadStatus[thread]=TH_READ;
      threadStatusMutex.unlock();
      act=dequeueAction();
      switch (act.opcode)
      {
	case ACT_READ:
	  /* Some clouds have points with return number 0, which is invalid.
	   * There are two kinds:
	   * * Velodyne point clouds have completely zeroed points placed
	   *   regularly through the file. These must be dropped.
	   * * Some photogrammetric point clouds have the return number set to 0
	   *   on all points. The return number should be set to 1.
	   */
	  try
	  {
	    dropZeros=false;
	    i=n=0;
	    h=relprime(act.hdr->numberPoints(),thread);
	    for (i=0;i<100;i++)
	    {
	      point=act.hdr->readPoint(n);
	      if (point.returnNum)
	      {
		dropZeros=true;
		break;
	      }
	    }
	  }
	  catch (...)
	  {
	  }
	  act.hdr->clearNumberReadPoints();
	  i=j=n=0;
	  cout<<"Thread "<<thread<<" reading "<<act.hdr->getFileName()<<endl;
	  offset=act.hdr->getOffset();
	  scale=act.hdr->getScale();
	  cout<<"Offset "<<offset.getx()<<','<<offset.gety()<<','<<offset.getz();
	  cout<<" Scale "<<scale.getx()<<','<<scale.gety()<<','<<scale.getz();
	  if (dropZeros)
	    cout<<" Dropping zeros\n";
	  else
	    cout<<" Keeping zeros\n";
	  nChunks=(act.hdr->numberPoints()+CHUNKSIZE-1)/CHUNKSIZE;
	  h=relprime(nChunks,thread);
	  try
	  {
	    while (j<nChunks && threadCommand!=TH_STOP)
	    {
	      if (pointBufferSize()*sizeof(point)<lowRam)
	      {
		if (n*CHUNKSIZE+i<act.hdr->numberPoints())
		{
		  point=act.hdr->readPoint(n*CHUNKSIZE+i);
		  if (point.returnNum==0 && !dropZeros)
		    point.returnNum=1;
		  if (point.returnNum)
		    embufferPoint(point,false); // It is from file, but sleeping is handled here.
		}
		i++;
		if (i==CHUNKSIZE)
		{
		  i=0;
		  j++;
		  n=(n+h)%nChunks;
		}
	      }
	      if (pointBuffersNonempty() || pointBufferSize()*sizeof(point)>lowRam || pointBufferSize()>65536)
	      {
		point=debufferPoint(thread);
		if (point.isEmpty() && pointBufferSize()*sizeof(point)>lowRam)
		  sleep(thread);
		else
		{
		  blknum=octRoot.findBlock(point.location);
		  if (blknum<0 || blknum%threadStatus.size()==thread)
		  {
		    nPoints++;
		    octStore.put(point);
		    octStore.disown();
		    unsleep(thread);
		  }
		  else
		    embufferPoint(point,false);
		}
	      }
	    }
	  }
	  catch (...)
	  {
	    cerr<<"Error reading file\n";
	  }
	  break;
      }
      point=debufferPoint(thread);
      if (point.isEmpty())
	sleep(thread);
      else
      {
	blknum=octRoot.findBlock(point.location);
	if (blknum<0 || blknum%threadStatus.size()==thread)
	{
	  nPoints++;
	  octStore.put(point);
	  /*gotPoint=octStore.get(point.location);
	  if (gotPoint.isEmpty())
	    cout<<"Point missing\n";*/
	  octStore.disown();
	  unsleep(thread);
	}
	else
	{
	  embufferPoint(point,false);
	  unsleep(thread);
	}
      }
    }
    if (threadCommand==TH_PAUSE)
    { // The job is ongoing, but has to pause to write out the files.
      threadStatusMutex.lock();
      threadStatus[thread]=TH_PAUSE;
      threadStatusMutex.unlock();
      act=dequeueAction();
      switch (act.opcode)
      {
	case ACT_READ:
	  cerr<<"Can't read a file in pause state\n";
	  unsleep(thread);
	  break;
	case ACT_COUNT:
	  countClasses(thread);
	  enqueueResult(act);
	  octStore.disown();
	  break;
	case ACT_WRITE:
#ifdef GUI
	  cloudOutput.writeFiles();
	  cloudOutput.closeFiles();
#endif
	  censusPoints();
	  break;
	default:
	  sleep(thread);
      }
    }
    if (threadCommand==TH_SCAN)
    { // Scan the tiles to find the point density of the bottom.
      threadStatusMutex.lock();
      threadStatus[thread]=TH_SCAN;
      threadStatusMutex.unlock();
      cylAddress=snake.next();
      if (cylAddress.getx()!=INT_MIN)
      {
	scanCylinder(cylAddress);
	enqueueTileDone(cylAddress);
      }
      else
	sleep(thread);
    }
    if (threadCommand==TH_POSTSCAN)
    { // After scanning, set the paraboloid size for forests and roofs.
      threadStatusMutex.lock();
      threadStatus[thread]=TH_POSTSCAN;
      threadStatusMutex.unlock();
      cylAddress=snake.next();
      if (cylAddress.getx()!=INT_MIN)
      {
	postscanCylinder(cylAddress);
	enqueueTileDone(cylAddress);
      }
      else
	sleep(thread);
    }
    if (threadCommand==TH_SPLIT)
    {
      threadStatusMutex.lock();
      threadStatus[thread]=TH_SPLIT;
      threadStatusMutex.unlock();
      cylAddress=snake.next();
      if (cylAddress.getx()!=INT_MIN)
      {
	classifyCylinder(cylAddress);
	enqueueTileDone(cylAddress);
      }
      else
	sleep(thread);
    }
    if (threadCommand==TH_WAIT)
    { // There is no job. The threads are waiting for a job.
      threadStatusMutex.lock();
      threadStatus[thread]=TH_WAIT;
      threadStatusMutex.unlock();
      if (thread)
	act.opcode=0;
      else
	act=dequeueAction();
      switch (act.opcode)
      {
#ifdef XYZ
	case ACT_LOAD:
	  cloud.clear();
	  readCloud(act.filename,1,0);
	  br.clear();
	  for (i=0;i<cloud.size();i++)
	    br.include(cloud[i]);
	  act.hdr->setMinMax(xyz(br.left(),br.bottom(),br.low()),
			     xyz(br.right(),br.top(),br.high()));
	  enqueueResult(act);
	  unsleep(thread);
	  break;
#endif
	default:
	  sleep(thread);
      }
    }
  }
  //octStore.flush(thread,threads.size());
  threadStatusMutex.lock();
  threadStatus[thread]=TH_STOP;
  //cout<<"Thread "<<thread<<" processed "<<nPoints<<" points\n";
  threadStatusMutex.unlock();
}
