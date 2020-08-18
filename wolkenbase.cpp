/******************************************************/
/*                                                    */
/* wolkenbase.cpp - main program                      */
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
/* This program cleans a point cloud as follows:
 * 1. Read in the point cloud, storing it in a temporary file in blocks
 *    of RECORDS points and building an octree index.
 * 2. Scan the point cloud's xy projection in Hilbert curve point sequences:
 *    1 point, 4 points, 16 points, etc., looking at cylinders with radius
 *    0.5 meter.
 * 3. Find the lowest point near the axis of the cylinder and examine a sphere
 *    of radius 0.5 meter.
 */
#include <cmath>
#include <boost/program_options.hpp>
#include "las.h"
#include "threads.h"
#include "relprime.h"
#include "ldecimal.h"
#include "octree.h"
#include "freeram.h"
using namespace std;
namespace po=boost::program_options;

int main(int argc,char **argv)
{
  LasPoint lPoint;
  int i;
  int nthreads=thread::hardware_concurrency();
  size_t j;
  vector<string> inputFiles;
  vector<LasHeader> files;
  vector<xyz> limits;
  xyz center;
  ofstream testFile("testfile");
  bool validArgs,validCmd=true;
  po::options_description generic("Options");
  po::options_description hidden("Hidden options");
  po::options_description cmdline_options;
  po::positional_options_description p;
  po::variables_map vm;
  if (nthreads<2)
    nthreads=2;
  hidden.add_options()
    ("input",po::value<vector<string> >(&inputFiles),"Input file");
  p.add("input",-1);
  cmdline_options.add(generic).add(hidden);
  try
  {
    po::store(po::command_line_parser(argc,argv).options(cmdline_options).positional(p).run(),vm);
    po::notify(vm);
  }
  catch (exception &e)
  {
    cerr<<e.what()<<endl;
    validCmd=false;
  }
  files.resize(inputFiles.size());
  for (i=0;i<inputFiles.size();i++)
    files[i].open(inputFiles[i]);
  for (i=0;i<files.size();i++)
  {
    limits.push_back(files[i].minCorner());
    limits.push_back(files[i].maxCorner());
    cout<<files[i].numberPoints()<<" points\n";
  }
  lowRam=freeRam()/7;
  octRoot.sizeFit(limits);
  center=octRoot.getCenter();
  cout<<'('<<ldecimal(center.getx())<<','<<ldecimal(center.gety())<<','<<ldecimal(center.getz())<<")±";
  cout<<octRoot.getSide()<<endl;
  lPoint.location=xyz(M_PI,exp(1),sqrt(2));
  for (i=0;i<RECORDS;i++)
    lPoint.write(testFile);
  if (nthreads<1)
    nthreads=1;
  octStore.open("store.oct",nthreads+relprime(nthreads));
  octStore.resize(8*nthreads+1);
  startThreads(nthreads);
  waitForThreads(TH_RUN);
  for (i=0;i<files.size();i++)
  {
    for (j=0;j<files[i].numberPoints();j++)
    {
      lPoint=files[i].readPoint(j);
      embufferPoint(lPoint);
    }
    cout<<files[i].numberPoints()<<" points, "<<pointBufferSize()<<" points in buffer\n";
    cout<<octStore.getNumBuffers()<<" buffers, "<<octStore.getNumBlocks()<<" blocks\n";
  }
  waitForQueueEmpty();
  cout<<"All points in octree\n";
  cout<<octStore.getNumBuffers()<<" buffers, "<<octStore.getNumBlocks()<<" blocks\n";
  waitForThreads(TH_STOP);
  octStore.flush();
  joinThreads();
  return 0;
}
