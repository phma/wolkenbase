/******************************************************/
/*                                                    */
/* flowsnake.cpp - flowsnake curve                    */
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

#include <cassert>
#include <climits>
#include "flowsnake.h"
using namespace std;

const Eisenstein flowBase(2,-1);
const complex<double> cFlowBase(2.5,-M_SQRT_3_4);
const double squareSides[]=
{
  0.6583539906808145, // This is the biggest square that fits inside
  1.8501627472990723, // a Gosper island of size n. It's used to size
  4.286014912881196,  // the flowsnake to the octree. The table goes
  12.6716160058597,   // to 11 because 7**11 is the largest <2**32.
  32.85016274729906,
  79.01914481623778,
  243.0343734125204,
  592.8501627472989,
  1510.850162747299,
  4399.956311577825,
  10731.850162747294,
  29198.8501627473
};

char forwardFlowsnakeTable[][7]=
{
  {0x52,0x05,0x06,0x24,0x33,0x40,0x01},
  {0x31,0x10,0x12,0x05,0x43,0x54,0x16},
  {0x46,0x24,0x21,0x10,0x53,0x35,0x22},
  {0x31,0x10,0x03,0x54,0x36,0x35,0x22},
  {0x46,0x24,0x13,0x35,0x42,0x40,0x01},
  {0x52,0x05,0x23,0x40,0x51,0x54,0x16}
};

const int loLim[]={0,-4,-18,-214,-900,-10504,-44118,-514714,-24242424,-25221004,-105928218,-1235829214};
const int hiLim[]={0,2,30,128,1500,6302,73530,308828,3603000,15132602,176547030,741497528};

int base7dig(Eisenstein d)
/* Input: d has norm 0 or 1.
 * Output: [-3,3].
 */
{
  return d.getx()+2*d.gety();
}

Eisenstein baseFlowDig(int d)
{
  int x,y;
  y=(d+4)/3-1;
  x=d-2*y;
  return Eisenstein(x,y);
}

int baseSeven(Eisenstein e)
/* Converts e, in centered base 2-ω, to an integer in centered base 7.
 */
{
  int ret=0;
  int pow7=1;
  Eisenstein edig;
  while (e.norm())
  {
    edig=e%flowBase;
    e=e/flowBase;
    ret+=base7dig(edig)*pow7;
    pow7*=7;
  }
  return ret;
}

int iToFlowsnake(int n)
{
  int i,dirori;
  int dig[11];
  n+=1235829214;
  for (i=0;i<11;i++)
  {
    dig[i]=n%7;
    n/=7;
  }
  for (i=10,dirori=n=0;i>=0;i--)
  {
    dig[i]=forwardFlowsnakeTable[dirori][dig[i]];
    dirori=dig[i]>>4;
    dig[i]&=7;
    n=7*n+dig[i];
  }
  return n-988663371;
}

Eisenstein baseFlow(int n)
/* Converts n, in centered base 7, to an Eisenstein integer in centered base 2-ω.
 */
{
  Eisenstein ret=0;
  Eisenstein powF=1;
  int dig;
  while (n)
  {
    dig=n%7;
    if (dig>3)
      dig-=7;
    if (dig<-3)
      dig+=7;
    n=(n-dig)/7;
    ret+=baseFlowDig(dig)*powF;
    powF*=flowBase;
  }
  return ret;
}

Eisenstein toFlowsnake(int n)
{
  return baseFlow(iToFlowsnake(n));
}

double squareSize(complex<double> z)
// Returns half the side of the square, centered at the origin, that z lies on.
{
  return max(fabs(z.real()),fabs(z.imag()));
}

vector<complex<double> > crinklyLine(complex<double> begin,complex<double> end,double precision)
{
  vector<complex<double> > ret,seg;
  complex<double> bend1,bend2;
  int i;
  if (abs(end-begin)<precision)
    ret.push_back(begin);
  else
  {
    bend1=begin+(end-begin)/cFlowBase;
    bend2=end+(begin-end)/cFlowBase;
    seg=crinklyLine(begin,bend1,precision);
    for (i=0;i<seg.size();i++)
      ret.push_back(seg[i]);
    seg=crinklyLine(bend1,bend2,precision);
    for (i=0;i<seg.size();i++)
      ret.push_back(seg[i]);
    seg=crinklyLine(bend2,end,precision);
    for (i=0;i<seg.size();i++)
      ret.push_back(seg[i]);
  }
  return ret;
}

double biggestSquare(complex<double> begin,complex<double> end,double sofar)
/* Returns the size of the biggest square centered at the origin
 * whose interior contains none of the crinkly line.
 */
{
  double ret=min(squareSize(begin),squareSize(end));
  complex<double> bend1=begin+(end-begin)/cFlowBase;
  complex<double> bend2=end+(begin-end)/cFlowBase;
  if ((begin==bend1)+(bend1==bend2)+(bend2==end)<2 && ret-sofar<2*abs(end-begin))
  {
    double seg[3];
    int i;
    if (ret<sofar)
      sofar=ret;
    seg[0]=biggestSquare(begin,bend1,sofar);
    seg[1]=biggestSquare(bend1,bend2,sofar);
    seg[2]=biggestSquare(bend2,end,sofar);
    for (i=0;i<3;i++)
      if (seg[i]<ret)
	ret=seg[i];
  }
  return ret;
}

double biggestSquare(int size)
{
  complex<double> corners[4];
  int i;
  double ret=INFINITY,seg;
  corners[0]=complex<double>(0,M_SQRT_1_3)*pow(cFlowBase,size);
  for (i=1;i<4;i++)
  {
    corners[i]=corners[i-1]*(complex<double>)Eisenstein(1,1);
    seg=biggestSquare(corners[i-1],corners[i],ret);
    if (seg<ret)
      ret=seg;
  }
  return ret;
}

void Flowsnake::setSize(Cube cube,double desiredSpacing)
{
  int i,bestI;
  double spacing1,diff,bestDiff=INFINITY;
  assert(desiredSpacing>0);
  assert(cube.getSide()>0);
  for (i=0;i<12;i++)
  {
    spacing1=cube.getSide()/squareSides[i];
    diff=fabs(log(spacing1/desiredSpacing));
    if (diff<bestDiff)
    {
      bestDiff=diff;
      bestI=i;
    }
  }
  center=cube.getCenter();
  spacing=cube.getSide()/squareSides[bestI];
  startnum=counter=loLim[bestI];
  stopnum=hiLim[bestI];
}

void Flowsnake::restart()
{
  counter=startnum;
}

Eisenstein Flowsnake::next()
/* Returns the coordinates of the cylinder enclosing
 * the next hexagon in the flowsnake sequence.
 * When finished, returns INT_MIN.
 */
{
  Eisenstein e;
  flowMutex.lock();
  if (counter<=stopnum)
    e=toFlowsnake(counter++);
  else
    e=Eisenstein(INT_MIN,INT_MIN);
  flowMutex.unlock();
  return e;
}

Cylinder Flowsnake::cyl(Eisenstein e)
{
  double rad=spacing*41/71;
  complex<double> z=e;
  z*=spacing;
  if (e.getx()==INT_MIN)
    rad=0;
  return Cylinder(xy(z.real(),z.imag())+center,rad);
}

double Flowsnake::progress()
{
  double ret;
  flowMutex.lock();
  ret=double(counter-startnum)/double(stopnum-startnum);
  flowMutex.unlock();
  return ret;
}
