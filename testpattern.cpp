/******************************************************/
/*                                                    */
/* testpattern.cpp - test patterns                    */
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
#include <cmath>
#include <cassert>
#include "testpattern.h"
#include "angle.h"
#include "octree.h"

using namespace std;

unsigned int areaPhase;
int anglePhase;

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

LasPoint laserize(xyz pnt)
{
  LasPoint ret;
  ret.location=pnt;
  ret.intensity=1024;
  ret.returnNum=ret.nReturns=1;
  ret.scanDirection=ret.edgeLine=false;
  ret.classification=ret.classificationFlags=0;
  ret.scannerChannel=0;
  ret.userData=0;
  ret.waveIndex=0;
  ret.pointSource=0;
  ret.scanAngle=0;
  ret.gpsTime=0;
  ret.nir=ret.red=ret.green=ret.blue=33000; // maybe change depending on type of point
  ret.waveformOffset=ret.waveformSize=0;
  ret.waveformTime=0;
  ret.xDir=ret.yDir=0;
  ret.zDir=-1;
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
  limits.push_back(xyz(-50,-50,-1));
  limits.push_back(xyz(50,50,1));
  octRoot.sizeFit(limits);
  dots=diskSpatter(xyz(0,0,0),xyz(0,0,1),rad,den);
  cout<<dots.size()<<" points\n";
  for (i=0;i<dots.size();i++)
  {
    lPoint=laserize(dots[i]);
    octStore.put(lPoint);
  }
  reputPoints();
}
