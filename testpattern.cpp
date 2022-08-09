/******************************************************/
/*                                                    */
/* testpattern.cpp - test patterns                    */
/*                                                    */
/******************************************************/
/* Copyright 2019,2021,2022 Pierre Abbat.
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
#include <cmath>
#include <cassert>
#include "config.h"
#include "testpattern.h"
#include "angle.h"
#include "random.h"
#include "octree.h"

using namespace std;

unsigned int areaPhase;
int anglePhase;
vector<LasPoint> testCloud;
deque<uint64_t> pointCensus;
bool storePoints=true;

void setStorePoints(bool s)
{
  storePoints=s;
}

void reputPoints()
// When splitting a block, points are put in the buffer, and must be reput.
{
  LasPoint pnt;
  while (true)
  {
    pnt=debufferPoint(0);
    if (pnt.isEmpty())
      break;
    octStore.put(pnt);
  }
}

int censusPoints(vector<LasPoint> points)
/* Sets a bit in pointCensus for each point, if they are test data.
 * Returns 1 if a bit was already set, -1 if GPS time is not an integer
 * (which means not test data), and 0 if all points are test points
 * not previously counted.
 */
{
  int i,n,ret=0;
  uint64_t mask;
  //cout<<points.size()<<'+';
  for (i=0;i<points.size() && ret>=0;i++)
  {
    n=lrint(points[i].gpsTime);
    if (n!=points[i].gpsTime || n<0)
      ret=-1;
    else
    {
      if (pointCensus.size()<n/64+1)
	pointCensus.resize(n/64+1);
      mask=((uint64_t)1)<<(n%64);
      if (pointCensus[n/64]&mask)
	ret=1;
      pointCensus[n/64]|=mask;
    }
  }
  return ret;
}

void censusPoints()
{
  int64_t i,maxPoint;
  int err=0;
  vector<int64_t> missing;
  pointCensus.clear();
  for (i=0;i<octStore.getNumBlocks() && err>=0;i++)
    err=censusPoints(octStore.getAll(i));
  if (err>0 && pointCensus.size()>1)
    // If all gpsTimes are 0, it may be because the point format has no gpsTime.
    cout<<"Duplicate point\n";
  if (err>=0)
  {
    for (i=0;i<64 && pointCensus.size() && (pointCensus.back()>>i);i++)
      ;
    if (!pointCensus.size())
      maxPoint=0;
    else
      maxPoint=(pointCensus.size()-1)*64+i;
    cout<<"Max point "<<maxPoint<<endl;
    for (i=0;i<maxPoint;i++)
    {
      if (pointCensus[i/64]==0xffffffffffffffff)
	i+=63;
      if ((pointCensus[i/64]&(1<<(i&63)))==0)
	missing.push_back(i);
    }
  }
  if (missing.size())
  {
    cout<<"Missing points: ";
    for (i=0;i<missing.size();i++)
    {
      if (i)
	cout<<',';
      cout<<missing[i];
    }
    cout<<endl;
  }
}

void initPhases()
{
  areaPhase=rng.uirandom();
  anglePhase=rng.uirandom();
}

/* Terrain with street intersection:
 * 100 m diameter, with two 15 m wide streets intersecting at right angles.
 * Power poles with wires between them. Eventually, forbs and trees.
 */

double street(double x)
// Cross section of a street 15 m wide
{
  double elev;
  x=fabs(x);
  if (x<7.2)
    elev=-x/50;
  else if (x<7.4)
    elev=-0.15;
  else
    elev=0;
  return elev;
}

Quaternion rotateTo(xyz normal)
/* Returns a quaternion which rotates a vertical line to normal.
 * If normal points straight down, it returns the identity.
 */
{
  double len=normal.length();
  xyz midway;
  assert(len);
  normal/=len;
  midway=normal+xyz(0,0,1);
  if (midway.length())
    return versor(midway,DEG180);
  else
    return Quaternion(1);
}

LasPoint laserize(xyz pnt,int n)
{
  LasPoint ret;
  ret.location=pnt;
  ret.intensity=1024;
  ret.returnNum=ret.nReturns=1;
  ret.scanDirection=ret.edgeLine=false;
  ret.classification=ret.classificationFlags=0;
  ret.scannerChannel=0;
  ret.userData=0;
#ifdef WAVEFORM
  ret.waveIndex=0;
#endif
  ret.pointSource=0;
  ret.scanAngle=0;
  ret.gpsTime=n;
  ret.nir=ret.red=ret.green=ret.blue=33000; // maybe change depending on type of point
#ifdef WAVEFORM
  ret.waveformOffset=ret.waveformSize=0;
  ret.waveformTime=0;
  ret.xDir=ret.yDir=0;
  ret.zDir=-1;
#endif
  return ret;
}

vector<xyz> diskSpatter(xyz center,xyz normal,double radius,double density)
/* Places dots evenly in a disk. If the area times the density is less than one,
 * may return an empty vector. Used for terrain and leaves.
 */
{
  vector<xyz> ret;
  xyz dot;
  double dotRadius;
  Quaternion ro=rotateTo(normal);
  double nDots=M_PI*sqr(radius)*density;
  long long nDotsFixed=llrintl(4294967296e0*nDots);
  while (nDotsFixed>=4294967296 || (nDotsFixed>=areaPhase && areaPhase>0))
  {
    if (nDotsFixed>=areaPhase && areaPhase>0)
    {
      nDotsFixed-=areaPhase;
      areaPhase=0;
    }
    else if (nDotsFixed>=4294967296)
      nDotsFixed-=4294967296;
    dotRadius=sqrt(nDotsFixed/4294967296e0/M_PI/density);
    dot=ro.rotate(xyz(cossin(anglePhase)*dotRadius,0))+center;
    anglePhase+=PHITURN;
    ret.push_back(dot);
    if ((ret.size()&16383)==0)
    {
      cout<<ret.size()<<'\r';
      cout.flush();
    }
  }
  areaPhase-=nDotsFixed;
  return ret;
}

vector<xyz> cylinderSpatter(xyz begin,xyz end,double radius,double density)
/* Places dots evenly on a cylinder. Used for tree stems and power poles.
 */
{
  vector<xyz> ret;
  return ret;
}

vector<xyz> catenarySpatter(xyz vertex,double scale,int bearing,double radius,double density)
/* Places dots evenly on a catenary, except that they're slightly more dense
 * on the inside of the bend. Used for power lines.
 */
{
  vector<xyz> ret;
  return ret;
}

void flatScene(double rad,double den)
/* A rad m radius circle covered with den points per square meter,
 * all at elevation 0. Used to test the functions that find all
 * points in a sphere, a paraboloid, or the like. Should have
 * 785398±1 points if rad is 50 and den is 100.
 */
{
  vector<xyz> limits,dots;
  int i;
  LasPoint lPoint;
  limits.push_back(xyz(-rad,-rad,-1));
  limits.push_back(xyz(rad,rad,1));
  octRoot.sizeFit(limits);
  dots=diskSpatter(xyz(0,0,0),xyz(0,0,1),rad,den);
  cout<<dots.size()<<" points\n";
  for (i=0;i<dots.size();i++)
  {
    lPoint=laserize(dots[i],i);
    if (storePoints)
      octStore.put(lPoint);
    else
      testCloud.push_back(lPoint);
  }
  reputPoints();
}

void wavyScene(double rad,double den,double avg,double amp,double freq)
/* Same as flatScene, except that the elevation varies in a sine wave.
 */
{
  vector<xyz> limits,dots;
  int i;
  LasPoint lPoint;
  limits.push_back(xyz(-rad,-rad,-1));
  limits.push_back(xyz(rad,rad,1));
  octRoot.sizeFit(limits);
  dots=diskSpatter(xyz(0,0,0),xyz(0,0,1),rad,den);
  cout<<dots.size()<<" points\n";
  for (i=0;i<dots.size();i++)
  {
    dots[i]=xyz(xy(dots[i]),avg+amp*sin(dots[i].getx()*2*M_PI*freq));
    lPoint=laserize(dots[i],i);
    if (storePoints)
      octStore.put(lPoint);
    else
      testCloud.push_back(lPoint);
  }
  reputPoints();
}
