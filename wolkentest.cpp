/******************************************************/
/*                                                    */
/* wolkentest.cpp - test program                      */
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

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <csignal>
#include <cfloat>
#include <cstring>
#include <vector>
#include <string>
#include "octree.h"
#include "shape.h"
#include "testpattern.h"
#include "eisenstein.h"
#include "random.h"
#include "ldecimal.h"
#include "ps.h"
#include "freeram.h"
#include "flowsnake.h"

#define tassert(x) testfail|=(!(x))
//#define tassert(x) if (!(x)) {testfail=true; sleep(10);}
// so that tests still work in non-debug builds

using namespace std;

bool testfail=false;
vector<string> args;

void outsizeof(string typeName,int size)
{
  cout<<"size of "<<typeName<<" is "<<size<<endl;
}

void testsizeof()
// This is not run as part of "make test".
{
  outsizeof("LasPoint",sizeof(LasPoint));
  outsizeof("OctBuffer",sizeof(OctBuffer));
  /* LasPoint	104
   * OctBuffer	152
   */
}

void testfreeram()
{
  cout<<"Free RAM "<<freeRam()<<endl;
}

void testflat()
{
  flatScene(10,100);
}

void testparaboloid()
{
  Paraboloid p1(xyz(0,0,13),13);
  xyz v(0,0,13),a(5,0,12),b(5,2,12),c(13,13,0),d(-14,-12,0),e(9,16,0);
  Cube l(v,1),m(b,0.1);
  tassert(p1.in(v));
  tassert(p1.in(a));
  tassert(!p1.in(b));
  tassert(p1.in(c));
  tassert(!p1.in(d));
  tassert(p1.in(e));
  tassert(p1.intersect(l));
  tassert(!p1.intersect(m));
}

void testcylinder()
{
  Cylinder c1(xy(0,0),13);
  xyz cen(0,0,13),a(5,12,0),b(5,13,4),c(-9,-9,-3),d(-11,-7,2);
  Cube l(cen,1),m(a,0.1),n(d,1/16.),o(d,1/32.);
  tassert(c1.in(cen));
  tassert(c1.in(a));
  tassert(!c1.in(b));
  tassert(c1.in(c));
  tassert(!c1.in(d));
  tassert(c1.intersect(l));
  tassert(c1.intersect(m));
  tassert(c1.intersect(n));
  tassert(!c1.intersect(o));
}

const int linearSize[]={1,2,3,9,24,65,171,454,1200,3176,8403,22234};
const int quadraticSize[]={0,3,24,171,1200,8403,58824,411771,2882400,20176803,141237624,988663371};
const int loLim[]={0,-4,-18,-214,-900,-10504,-44118};
const int hiLim[]={0,2,30,128,1500,6302,73530};

void testflowsnake()
{
  PostScript ps;
  int i,j,n;
  int sz=6;
  double squareSize=biggestSquare(sz);
  Eisenstein e;
  complex<double> z;
  complex<double> corners[6];
  vector<complex<double> > coast,side;
  for (i=0;i<256;i++)
  {
    n=rng.usrandom()-32768;
    tassert(baseSeven(baseFlow(n))==n);
  }
  corners[0]=complex<double>(0,M_SQRT_1_3)*pow(cFlowBase,sz);
  for (i=1;i<6;i++)
    corners[i]=corners[i-1]*(complex<double>)Eisenstein(1,1);
  for (i=0;i<6;i++)
  {
    side=crinklyLine(corners[i],corners[(i+1)%6],0.5);
    for (n=0;n<side.size();n++)
      coast.push_back(side[n]);
  }
  ps.open("flowsnake.ps");
  ps.setpaper(papersizes["A4 landscape"],0);
  ps.prolog();
  ps.startpage();
  ps.setscale(-linearSize[sz],-linearSize[sz],linearSize[sz],linearSize[sz]);
  ps.startline();
  for (i=-quadraticSize[sz];i<=quadraticSize[sz];i++)
  {
    e=baseFlow(i);
    z=e;
    ps.lineto(xy(z.real(),z.imag()));
  }
  ps.endline();
  ps.endpage();
  ps.startpage();
  ps.setscale(-linearSize[sz],-linearSize[sz],linearSize[sz],linearSize[sz]);
  ps.setcolor(1,1,0);
  ps.startline();
  ps.lineto(xy(squareSize,squareSize));
  ps.lineto(xy(-squareSize,squareSize));
  ps.lineto(xy(-squareSize,-squareSize));
  ps.lineto(xy(squareSize,-squareSize));
  ps.endline(false,true);
  ps.setcolor(0,0,0);
  ps.startline();
  for (i=loLim[sz];i<=hiLim[sz];i++)
  {
    e=toFlowsnake(i);
    z=e;
    ps.lineto(xy(z.real(),z.imag()));
  }
  ps.endline();
  ps.setcolor(0,0,1);
  ps.startline();
  for (i=0;i<coast.size();i++)
    ps.lineto(xy(coast[i].real(),coast[i].imag()));
  ps.endline(true);
  ps.endpage();
  for (n=0;n<12;n++)
  {
    ps.startpage();
    ps.setscale(-linearSize[n],-linearSize[n],linearSize[n],linearSize[n]);
    corners[0]=complex<double>(0,M_SQRT_1_3)*pow(cFlowBase,n);
    squareSize=biggestSquare(n);
    for (i=1;i<6;i++)
      corners[i]=corners[i-1]*(complex<double>)Eisenstein(1,1);
    coast.clear();
    for (i=0;i<6;i++)
    {
      side=crinklyLine(corners[i],corners[(i+1)%6],squareSize/1000);
      for (j=0;j<side.size();j++)
	coast.push_back(side[j]);
    }
    ps.setcolor(1,1,0);
    ps.startline();
    ps.lineto(xy(squareSize,squareSize));
    ps.lineto(xy(-squareSize,squareSize));
    ps.lineto(xy(-squareSize,-squareSize));
    ps.lineto(xy(squareSize,-squareSize));
    ps.endline(false,true);
    if (n==0)
    {
      ps.setcolor(1,0.5,0.5);
      ps.circle(xy(0.574918626350463586,0.07052452345448619644),0.01);
    }
    ps.setcolor(0,0,1);
    ps.startline();
    for (i=0;i<coast.size();i++)
      ps.lineto(xy(coast[i].real(),coast[i].imag()));
    ps.endline(true);
    ps.endpage();
    cout<<n<<"  "<<ldecimal(2*squareSize)<<endl;
  }
}

/* Files for testing block splitting:
 * 719: 1, 710, 81
 * 727: 1, 10, 100
 * 5749: 1, 18, 324
 * 5779: 1, 5762, 289
 */
void testsplitfile()
{
  int i;
  LasHeader lasHeader;
  lasHeader.openWrite("719.las");
  lasHeader.setVersion(1,2);
  lasHeader.setPointFormat(0);
  lasHeader.setScale(xyz(0,0,0),xyz(1,1,1),xyz(1/719.,1/719.,1/719.));
}

bool shoulddo(string testname)
{
  int i;
  bool ret,listTests=false;
  if (testfail)
  {
    cout<<"failed before "<<testname<<endl;
    //sleep(2);
  }
  ret=args.size()==0;
  for (i=0;i<args.size();i++)
  {
    if (testname==args[i])
      ret=true;
    if (args[i]=="-l")
      listTests=true;
  }
  if (listTests)
    cout<<testname<<endl;
  return ret;
}

int main(int argc, char *argv[])
{
  int i;
  for (i=1;i<argc;i++)
    args.push_back(argv[i]);
  octStore.open("store.oct");
  if (shoulddo("sizeof"))
    testsizeof();
  if (shoulddo("freeram"))
    testfreeram();
  if (shoulddo("complex"))
    testcomplex();
  if (shoulddo("pageinx"))
    testpageinx();
  if (shoulddo("paraboloid"))
    testparaboloid();
  if (shoulddo("cylinder"))
    testcylinder();
  if (shoulddo("flat"))
    testflat();
  if (shoulddo("flowsnake"))
    testflowsnake();
  if (shoulddo("splitfile"))
    testsplitfile();
  cout<<"\nTest "<<(testfail?"failed":"passed")<<endl;
  return testfail;
}
