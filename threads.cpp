/******************************************************/
/*                                                    */
/* threads.cpp - multithreading                       */
/*                                                    */
/******************************************************/
/* Copyright 2020 Pierre Abbat.
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
#include <map>
#include <set>
#include <cassert>
#include <atomic>
#include "threads.h"
#include "angle.h"
#include "random.h"
#include "relprime.h"
#include "brevno.h"
#include "las.h"
#include "freeram.h"
#include "octree.h"
using namespace std;
namespace cr=std::chrono;

mutex actMutex;
mutex startMutex;
mutex opTimeMutex;
mutex bufferMutex;
shared_mutex threadStatusMutex;
mutex metaMutex;

atomic<int> threadCommand;
vector<thread> threads;
vector<int> threadStatus; // Bit 8 indicates whether the thread is sleeping.
vector<double> sleepTime;
vector<int> triangleHolders; // one per triangle
vector<vector<int> > heldTriangles; // one list of triangles per thread
double stageTolerance;
double minArea;
queue<ThreadAction> actQueue,resQueue;
map<int,vector<LasPoint> > pointBuffer;
int bufferPos=0;
int currentAction;
map<thread::id,int> threadNums;

cr::steady_clock clk;
vector<int> cleanBuckets;
/* Indicates whether the buckets used by areaDone are clean or dirty.
 * A bucket is clean if the last thing done was add it up; it is dirty
 * if a triangle whose area is added in the bucket has changed
 * since the bucket was added up.
 */
vector<double> allBuckets,doneBuckets,doneq2Buckets;
int opcount,trianglesToPaint;
double opTime; // time for triop and edgeop, in milliseconds
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
  int i;
  threadCommand=TH_WAIT;
  openThreadLog();
  logStartThread();
  heldTriangles.resize(n);
  sleepTime.resize(n);
  opTime=0;
  threadNums[this_thread::get_id()]=-1;
  for (i=0;i<n;i++)
  {
    threads.push_back(thread(WolkenThread(),i));
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
  int sz,thread;
  octStore.setBlockMutex.lock_shared();
  thread=octRoot.findBlock(point.location);
  octStore.setBlockMutex.unlock_shared();
  if (thread<0)
    thread+=threadStatus.size();
  thread%=threadStatus.size();
  bufferMutex.lock();
  pointBuffer[thread].push_back(point);
  sz=pointBuffer[thread].size();
  bufferPos=(bufferPos+relprime(sz))%sz;
  swap(pointBuffer[thread].back(),pointBuffer[thread][bufferPos]);
  bufferMutex.unlock();
  while (fromFile && pointBufferSize()*sizeof(point)>lowRam)
    this_thread::sleep_for(chrono::milliseconds(1));
}

LasPoint debufferPoint(int thread)
{
  LasPoint ret;
  bufferMutex.lock();
  if (pointBuffer[thread].size())
  {
    ret=pointBuffer[thread].back();
    pointBuffer[thread].pop_back();
    if (pointBuffer[thread].capacity()>2*pointBuffer[thread].size())
      pointBuffer[thread].shrink_to_fit();
  }
  bufferMutex.unlock();
  return ret;
}

size_t pointBufferSize()
{
  size_t sz,sum=0,i;
  bufferMutex.lock();
  for (i=0;i<pointBuffer.size();i++)
  {
    sz=pointBuffer[i].size();
    sum+=sz;
  }
  bufferMutex.unlock();
  return sum;
}

bool pointBufferEmpty()
{
  return pointBufferSize()==0;
}

void sleep(int thread)
{
  sleepTime[thread]+=1+sleepTime[thread]/1e3;
  if (sleepTime[thread]>opTime*sleepTime.size()+1000)
    sleepTime[thread]=opTime*sleepTime.size()+1000;
  threadStatusMutex.lock();
  threadStatus[thread]|=256;
  threadStatusMutex.unlock();
  this_thread::sleep_for(chrono::milliseconds(lrint(sleepTime[thread])));
  threadStatusMutex.lock();
  threadStatus[thread]&=255;
  threadStatusMutex.unlock();
}

void sleepDead(int thread)
// Sleep to get out of deadlock.
{
  sleepTime[thread]*=1+thread/1e3;
  threadStatusMutex.lock();
  threadStatus[thread]|=256;
  threadStatusMutex.unlock();
  this_thread::sleep_for(chrono::milliseconds(lrint(sleepTime[thread])));
  threadStatusMutex.lock();
  threadStatus[thread]&=255;
  threadStatusMutex.unlock();
}

void unsleep(int thread)
{
  sleepTime[thread]-=1+sleepTime[thread]/1e3;
  if (sleepTime[thread]<0 || std::isnan(sleepTime[thread]))
    sleepTime[thread]=0;
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

void randomizeSleep()
{
  int i;
  for (i=0;i<sleepTime.size();i++)
    sleepTime[i]=rng.usrandom()*opTime*sleepTime.size()/32768;
}

void updateOpTime(cr::nanoseconds duration)
{
  double time=duration.count()/1e6;
  opTimeMutex.lock();
  opTime*=0.999;
  if (time>opTime)
    opTime=time;
  opTimeMutex.unlock();
}

void setThreadCommand(int newStatus)
{
  threadCommand=newStatus;
  //cout<<statusNames[newStatus]<<endl;
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

void WolkenThread::operator()(int thread)
{
  int i=0,nPoints=0;
  long long blknum;
  ThreadAction act;
  LasPoint point,gotPoint;
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
    if (threadCommand==TH_RUN)
    {
      threadStatusMutex.lock();
      threadStatus[thread]=TH_RUN;
      threadStatusMutex.unlock();
      point=debufferPoint(thread);
      if (point.isEmpty())
	sleep(thread);
      else
      {
	blknum=octRoot.findBlock(point.location);
	if (blknum<0)
	  blknum+=threadStatus.size();
	if (blknum%threadStatus.size()==thread)
	{
	  nPoints++;
	  octStore.put(point);
	  gotPoint=octStore.get(point.location);
	  if (gotPoint.isEmpty())
	    cout<<"Point missing\n";
	  octStore.disown();
	  unsleep(thread);
	}
	else
	  embufferPoint(point,false);
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
	case ACT_LOAD:
	  cerr<<"Can't load a file in pause state\n";
	  unsleep(thread);
	  break;
	case ACT_READ_PTIN:
	  cerr<<"Can't open file in pause state\n";
	  unsleep(thread);
	  break;
	case ACT_OCTAGON:
	  cerr<<"Can't make octagon in pause state\n";
	  unsleep(thread);
	  break;
	default:
	  sleep(thread);
      }
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
	default:
	  sleep(thread);
      }
    }
  }
  octStore.flush(thread,threads.size());
  threadStatusMutex.lock();
  threadStatus[thread]=TH_STOP;
  cout<<"Thread "<<thread<<" processed "<<nPoints<<" points\n";
  threadStatusMutex.unlock();
}
