/******************************************************/
/*                                                    */
/* threads.h - multithreading                         */
/*                                                    */
/******************************************************/
/* Copyright 2020,2021 Pierre Abbat.
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

#ifndef THREADS_H
#define THREADS_H

class Flowsnake;

#ifdef __MINGW64__
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#include <../mingw-std-threads/mingw.thread.h>
#include <../mingw-std-threads/mingw.mutex.h>
#include <../mingw-std-threads/mingw.shared_mutex.h>
#else
#include <thread>
#include <mutex>
#include <shared_mutex>
#endif
#include <chrono>
#include <vector>
#include <array>
#include "las.h"
#include "eisenstein.h"
#include "flowsnake.h"
#ifdef GUI
#include "cloudoutput.h"
#endif

// These are used as both commands to the threads and status from the threads.
#define TH_WAIT 1
#define TH_READ 2
#define TH_SCAN 3
#define TH_POSTSCAN 4
#define TH_SPLIT 5
#define TH_PAUSE 6
#define TH_STOP 7
#define TH_ASLEEP 256

// These are used to tell threads to do things, mostly file-related.
#define ACT_READ 1
#define ACT_COUNT 2
#define ACT_WRITE 3

struct ThreadAction
{
  int opcode;
  int param0;
  double param1;
  double param2;
  LasHeader *hdr;
  std::string filename;
  int flags;
  int result;
};

extern int currentAction;
extern std::chrono::steady_clock clk;
extern Flowsnake snake;
extern std::map<int,size_t> classTotals;
#ifdef GUI
extern CloudOutput cloudOutput;
#endif

double busyFraction();
void startThreads(int n);
void joinThreads();
void enqueueAction(ThreadAction a);
ThreadAction dequeueResult();
bool actionQueueEmpty();
Eisenstein dequeueTileDone();
bool tileDoneQueueEmpty();
bool resultQueueEmpty();
void embufferPoint(LasPoint point,bool fromFile);
void embufferPoints(std::vector<LasPoint> points,int thread);
LasPoint debufferPoint(int thread);
size_t pointBufferSize();
bool pointBufferEmpty();
void sleepDead(int thread);
void setThreadCommand(int newStatus);
int getThreadStatus();
void waitForThreads(int newStatus);
void waitForQueueEmpty();
int thisThread();
int nThreads();

class WolkenThread
{
public:
  void operator()(int thread);
};

#endif
