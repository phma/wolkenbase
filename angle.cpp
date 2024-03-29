/******************************************************/
/*                                                    */
/* angle.cpp - angles as binary fractions of rotation */
/*                                                    */
/******************************************************/
/* Copyright 2019,2021 Pierre Abbat.
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

#define LIBRARY_ATAN2 0

#include <cstring>
#include <cassert>
#include <cstdio>
#include <vector>
#include "angle.h"
using namespace std;

#if !LIBRARY_ATAN2
double tanTable[511],cosTable[512],sinTable[512];
#endif

double sqr(double x)
{
  return x*x;
}

double cub(double x)
{
  return x*x*x;
}

double frac(double x)
{
  return x-floor(x);
}

double sin(int angle)
{
  return sinl(angle*M_PIl/1073741824.);
}

double cos(int angle)
{
  return cosl(angle*M_PIl/1073741824.);
}

double tan(int angle)
{
  return tanl(angle*M_PIl/1073741824.);
}

double cot(int angle)
{
  return 1/tanl(angle*M_PIl/1073741824.);
}

double sinhalf(int angle)
{
  return sinl(angle*M_PIl/2147483648.);
}

double coshalf(int angle)
{
  return cosl(angle*M_PIl/2147483648.);
}

double tanhalf(int angle)
{
  return tanl(angle*M_PIl/2147483648.);
}

double cosquarter(int angle)
{
  return cosl(angle*M_PIl/4294967296.);
}

double tanquarter(int angle)
{
  return tanl(angle*M_PIl/4294967296.);
}

double atan2a(double y,double x)
{
  //return 3.417823697056007e8*y/x;
  return 0x40000000/M_PIl*y/x-1.1392738508503886e8*cub(y/x);
}

/* I ran Wolkenbase on a big and tall point cloud, and it kept segfaulting
 * in the atan2 library routine, in a place where segfaulting doesn't make sense.
 * Apparently there is a bug in the math library, which may involve failing to
 * restore some floating-point register when a thread is stopped on one core
 * and resumed on another. To get around this bug, I wrote this function.
 * The timings are from running testintegertrig under Valgrind (Callgrind).
 */

#if LIBRARY_ATAN2
int atan2i(double y,double x)
// This version takes 11.4% as much time as cossin.
{
  return rint(atan2(y,x)/M_PIl*1073741824.);
}
#else
int atan2i(double y,double x)
// This version takes 17.4% as much time as cossin.
{
  int ret=0;
  int h;
  double temp;
  if (x<0)
  {
    ret+=(y>0)?DEG180:-DEG180;
    y=-y;
    x=-x;
  }
  if (y>x)
  {
    ret+=DEG90;
    swap(x,y);
    y=-y;
  }
  if (-y>x)
  {
    ret-=DEG90;
    swap(x,y);
    x=-x;
  }
  temp=y/x;
  for (h=DEG45/2;h>DEG45/1024;h/=2)
    if (temp>tanTable[(((ret+DEG45)&0x1ff00000)>>20)-1])
      ret+=h;
    else
      ret-=h;
  h=511-(((ret+DEG45)&0x1ff00000)>>20);
  temp=x*cosTable[h]-y*sinTable[h];
  y=y*cosTable[h]+x*sinTable[h];
  x=temp;
  ret+=lrint(0x40000000/M_PIl*y/x-1.1392738508503886e8*cub(y/x));
  if (x==0 && y==0)
    ret=0;
  return ret;
}
#endif

int atan2i(xy vect)
{
  return atan2i(vect.north(),vect.east());
}

int twiceasini(double x)
{
  return rint(asin(x)/M_PIl*2147483648.);
}

xy cossin(double angle)
{
  return xy(cos(angle),sin(angle));
}

xy cossin(int angle)
{
  return xy(cos(angle),sin(angle));
}

xy cossinhalf(int angle)
{
  return xy(coshalf(angle),sinhalf(angle));
}

int foldangle(int angle)
{
  if (((unsigned)angle>>30)%3)
    angle^=0x80000000;
  return angle;
}

bool isinsector(int angle,int sectors)
/* Quick check for ranges of angles. angle is in [0°,720°), so sectors
 * is normally an unsigned multiple of 65537. Bit 0 is [0,22.5°), bit 1
 * is [22.5°,45°), etc.
 */
{
  return ((sectors>>((unsigned)angle>>27))&1);
}

double bintorot(int angle)
{
  return angle/2147483648.;
}

double bintogon(int angle)
{
  return bintorot(angle)*400;
}

double bintodeg(int angle)
{
  return bintorot(angle)*360;
}

double bintomin(int angle)
{
  return bintorot(angle)*21600;
}

double bintosec(int angle)
{
  return bintorot(angle)*1296000;
}

double bintorad(int angle)
{
  return bintorot(angle)*M_PIl*2;
}

int rottobin(double angle)
{
  double iprt=0,fprt;
  fprt=2*modf(angle/2,&iprt);
  if (fprt>=1)
    fprt-=2;
  if (fprt<-1)
    fprt+=2;
  return lrint(2147483648.*fprt);
}

int degtobin(double angle)
{
  return rottobin(angle/360);
}

int mintobin(double angle)
{
  return rottobin(angle/21600);
}

int sectobin(double angle)
{
  return rottobin(angle/1296000);
}

int gontobin(double angle)
{
  return rottobin(angle/400);
}

int radtobin(double angle)
{
  return rottobin(angle/M_PIl/2);
}

double radtodeg(double angle)
{
  return angle*180/M_PIl;
}

double degtorad(double angle)
{
  return angle/180*M_PIl;
}

double radtomin(double angle)
{
  return angle*10800/M_PIl;
}

double mintorad(double angle)
{
  return angle/10800*M_PIl;
}

double radtosec(double angle)
{
  return angle*648000/M_PIl;
}

double sectorad(double angle)
{
  return angle/648000*M_PIl;
}

double radtogon(double angle)
{
  return angle*200/M_PIl;
}

double gontorad(double angle)
{
  return angle/200*M_PIl;
}

void fillTanTables()
/* Fills tables used by the atan2i function. The tables cover the quadrant from
 * -45° to 45° in 512 steps of 0x100000.
 */
{
#if !LIBRARY_ATAN2
  int i;
  for (i=0;i<511;i++)
    tanTable[i]=tan(i*0x100000-0xff00000);
  for (i=0;i<512;i++)
  {
    sinTable[i]=sin(i*0x100000-0xff80000);
    cosTable[i]=cos(i*0x100000-0xff80000);
  }
#endif
}
