/******************************************************/
/*                                                    */
/* wolkentest.cpp - test program                      */
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
  tassert(p1.in(v));
  tassert(p1.in(a));
  tassert(!p1.in(b));
  tassert(p1.in(c));
  tassert(!p1.in(d));
  tassert(p1.in(e));
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
  cout<<"\nTest "<<(testfail?"failed":"passed")<<endl;
  return testfail;
}
