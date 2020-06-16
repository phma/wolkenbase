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

#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <csignal>
#include <cfloat>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <string>
#include "octree.h"
#include "shape.h"
#include "testpattern.h"
#include "eisenstein.h"
#include "random.h"
#include "ps.h"
#include "flowsnake.h"

#define tassert(x) testfail|=(!(x))
//#define tassert(x) if (!(x)) {testfail=true; sleep(10);}
// so that tests still work in non-debug builds

using namespace std;

bool testfail=false;
vector<string> args;

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

const int linearSize[]={1,2,3,9,24,65,171};
const int quadraticSize[]={0,3,24,171,1200,8403,58824};
const int loLim[]={0,-4,-18,-214,-900,-10504,-44118};
const int hiLim[]={0,2,30,128,1500,6302,73530};

void testflowsnake()
{
  PostScript ps;
  int i,n;
  int sz=6;
  Eisenstein e;
  complex<double> z;
  for (i=0;i<256;i++)
  {
    n=rng.usrandom()-32768;
    tassert(baseSeven(baseFlow(n))==n);
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
  ps.startline();
  for (i=loLim[sz];i<=hiLim[sz];i++)
  {
    e=toFlowsnake(i);
    z=e;
    ps.lineto(xy(z.real(),z.imag()));
  }
  ps.endline();
  ps.endpage();
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
  if (shoulddo("complex"))
    testcomplex();
  if (shoulddo("pageinx"))
    testpageinx();
  if (shoulddo("paraboloid"))
    testparaboloid();
  if (shoulddo("flat"))
    testflat();
  if (shoulddo("flowsnake"))
    testflowsnake();
  cout<<"\nTest "<<(testfail?"failed":"passed")<<endl;
  return testfail;
}
