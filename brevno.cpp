/******************************************************/
/*                                                    */
/* brevno.cpp - Mitobrevno interface                  */
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
#include "brevno.h"

#if defined(Mitobrevno_FOUND) && defined(ENABLE_MITOBREVNO)
#define MB_START_THREAD 0
#define MB_POINT_NOT_FOUND 1
#define MB_THREAD_STATUS 0x1000
#define MB_BEGIN_SPLIT 0x2000
#define MB_END_SPLIT 0x3000
#define MB_BEGIN_READ 0x2001
#define MB_END_READ 0x3001
#define MB_BEGIN_WRITE 0x2002
#define MB_END_WRITE 0x3002
using namespace std;
using namespace mitobrevno;

void openThreadLog()
{
  openLogFile("thread.log");
  describeEvent(MB_BEGIN_SPLIT,"split block");
  describeParam(MB_BEGIN_SPLIT,0,"buffer");
  describeParam(MB_BEGIN_SPLIT,1,"block");
  formatParam(MB_BEGIN_SPLIT,2,0);
  describeEvent(MB_BEGIN_READ,"read block");
  describeParam(MB_BEGIN_READ,0,"buffer");
  describeParam(MB_BEGIN_READ,1,"block");
  formatParam(MB_BEGIN_READ,2,0);
  describeEvent(MB_BEGIN_WRITE,"write block");
  describeParam(MB_BEGIN_WRITE,0,"buffer");
  describeParam(MB_BEGIN_WRITE,1,"block");
  formatParam(MB_BEGIN_WRITE,2,0);
  describeEvent(MB_POINT_NOT_FOUND,"point not found");
  describeParam(MB_POINT_NOT_FOUND,0,"buffer");
  describeParam(MB_POINT_NOT_FOUND,1,"block");
  describeParam(MB_POINT_NOT_FOUND,2,"x");
  describeParam(MB_POINT_NOT_FOUND,3,"y");
  describeParam(MB_POINT_NOT_FOUND,4,"z");
  formatParam(MB_POINT_NOT_FOUND,2,3);
  describeEvent(MB_START_THREAD,"start thread");
  describeEvent(MB_THREAD_STATUS,"thread status");
  describeParam(MB_THREAD_STATUS,0,"status");
  formatParam(MB_THREAD_STATUS,1,0);
}

void logStartThread()
{
  vector<int> intParams;
  vector<float> floatParams;
  logEvent(MB_START_THREAD,intParams,floatParams);
}

void logPointNotFound(int buf,int blk,float x,float y,float z)
{
  vector<int> intParams;
  vector<float> floatParams;
  intParams.push_back(buf);
  intParams.push_back(blk);
  floatParams.push_back(x);
  floatParams.push_back(y);
  floatParams.push_back(z);
  logEvent(MB_POINT_NOT_FOUND,intParams,floatParams);
}

void logBeginSplit(int buf,int blk)
{
  vector<int> intParams;
  vector<float> floatParams;
  intParams.push_back(buf);
  intParams.push_back(blk);
  logEvent(MB_BEGIN_SPLIT,intParams,floatParams);
}

void logEndSplit(int buf,int blk)
{
  vector<int> intParams;
  vector<float> floatParams;
  intParams.push_back(buf);
  intParams.push_back(blk);
  logEvent(MB_END_SPLIT,intParams,floatParams);
}

void logBeginRead(int buf,int blk)
{
  vector<int> intParams;
  vector<float> floatParams;
  intParams.push_back(buf);
  intParams.push_back(blk);
  logEvent(MB_BEGIN_READ,intParams,floatParams);
}

void logEndRead(int buf,int blk)
{
  vector<int> intParams;
  vector<float> floatParams;
  intParams.push_back(buf);
  intParams.push_back(blk);
  logEvent(MB_END_READ,intParams,floatParams);
}

void logBeginWrite(int buf,int blk)
{
  vector<int> intParams;
  vector<float> floatParams;
  intParams.push_back(buf);
  intParams.push_back(blk);
  logEvent(MB_BEGIN_WRITE,intParams,floatParams);
}

void logEndWrite(int buf,int blk)
{
  vector<int> intParams;
  vector<float> floatParams;
  intParams.push_back(buf);
  intParams.push_back(blk);
  logEvent(MB_END_WRITE,intParams,floatParams);
}

void logThreadStatus(int status)
{
  vector<int> intParams;
  vector<float> floatParams;
  intParams.push_back(status);
  logEvent(MB_THREAD_STATUS,intParams,floatParams);
}

void writeBufLog()
{
  writeBufferedLog();
}

#endif
