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
#include <atomic>
#include "threads.h"
#include "angle.h"
#include "random.h"
#include "relprime.h"
#include "las.h"
#include "octree.h"
#define MOD_RW_MUTEX 0
using namespace std;
namespace cr=std::chrono;

mutex actMutex;
mutex startMutex;
mutex opTimeMutex;
mutex bufferMutex;
shared_mutex threadStatusMutex;
#if MOD_RW_MUTEX
map<int,shared_mutex> modMutex;
#else
map<int,mutex> modMutex;
#endif
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
vector<LasPoint> pointBuffer;
int bufferPos=0;
int currentAction;
int modMutexSize;
map<thread::id,int> threadNums;
map<int,int> modReaders,modWriters;

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
  heldTriangles.resize(n);
  sleepTime.resize(n);
  opTime=0;
  threadNums[this_thread::get_id()]=-1;
  modMutexSize=relprime(55*n);
  for (i=0;i<modMutexSize;i++)
  {
    modReaders[i]=0;
    modWriters[i]=-1;
    modMutex[i];
  }
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

void lockBlockR(int block)
{
  metaMutex.lock();
  modReaders[block%modMutexSize]++;
  metaMutex.unlock();
#if MOD_RW_MUTEX
  modMutex[block%modMutexSize].lock_shared();
#else
  modMutex[block%modMutexSize].lock();
#endif
}

void lockBlockW(int block)
{
  modMutex[block%modMutexSize].lock();
  if (modWriters[block%modMutexSize]>=0)
    cout<<"Deadlock\n";
  modWriters[block%modMutexSize]=thisThread();
}

void unlockBlockR(int block)
{
  metaMutex.lock();
  if (--modReaders[block%modMutexSize]<0)
    cout<<"Read-unlocked "<<block<<" too many times\n";
  metaMutex.unlock();
#if MOD_RW_MUTEX
  modMutex[block%modMutexSize].unlock_shared();
#else
  modMutex[block%modMutexSize].unlock();
#endif
}

void unlockBlockW(int block)
{
  if (modWriters[block%modMutexSize]!=thisThread())
    cout<<"False unlock\n";
  modWriters[block%modMutexSize]=-1;
  modMutex[block%modMutexSize].unlock();
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

void embufferPoint(LasPoint point)
{
  int sz;
  bufferMutex.lock();
  pointBuffer.push_back(point);
  sz=pointBuffer.size();
  bufferPos=(bufferPos+relprime(sz))%sz;
  swap(pointBuffer.back(),pointBuffer[bufferPos]);
  bufferMutex.unlock();
}

LasPoint debufferPoint()
{
  LasPoint ret;
  bufferMutex.lock();
  if (pointBuffer.size())
  {
    ret=pointBuffer.back();
    pointBuffer.pop_back();
  }
  bufferMutex.unlock();
  return ret;
}

void sleepRead()
// Called when reading a ptin file that has many dots per triangle.
{
  this_thread::sleep_for(chrono::milliseconds(10));
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
// Waits until the action queue is empty and all threads have completed their actions.
{
  int i,n;
  do
  {
    n=actQueue.size();
    threadStatusMutex.lock_shared();
    for (i=0;i<threadStatus.size();i++)
      if (threadStatus[i]<256)
	n++;
    threadStatusMutex.unlock_shared();
    this_thread::sleep_for(chrono::milliseconds(n));
  } while (n);
}

int thisThread()
{
  return threadNums[this_thread::get_id()];
}

void WolkenThread::operator()(int thread)
{
  int e=0,t=0,d=0;
  int triResult,edgeResult;
  ThreadAction act;
  LasPoint point;
  startMutex.lock();
  if (threadStatus.size()!=thread)
  {
    cout<<"Starting thread "<<threadStatus.size()<<", was passed "<<thread<<endl;
    thread=threadStatus.size();
  }
  threadStatus.push_back(0);
  threadNums[this_thread::get_id()]=thread;
  startMutex.unlock();
  while (threadCommand!=TH_STOP)
  {
    if (threadCommand==TH_RUN)
    {
      threadStatusMutex.lock();
      threadStatus[thread]=TH_RUN;
      threadStatusMutex.unlock();
      point=debufferPoint();
      if (point.isEmpty())
	sleep(thread);
      else
      {
	octStore.put(point);
	unsleep(thread);
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
  threadStatusMutex.lock();
  threadStatus[thread]=TH_STOP;
  threadStatusMutex.unlock();
}
