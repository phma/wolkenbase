/******************************************************/
/*                                                    */
/* eisenstein.cpp - Eisenstein integers               */
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
/* Hexagonal vector (Eisenstein or Euler integers) and array of bytes subscripted by hexagonal vector
 */

#include <cstdio>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cassert>
#include "eisenstein.h"
#include "ps.h"

using namespace std;

//Page size for storing arrays subscripted by Eisenstein
/* A page looks like this:
 *       * * * * * *
 *      * * * * * * *
 *     * * * * * * * *
 *    * * * * * * * * *
 *   * * * * * * * * * *
 *  * * * * * * * * * * *
 *   * * * * * * * * * *
 *    * * * * * * * * *
 *     * * * * * * * *
 *      * * * * * * *
 *       * * * * * *
 */
const Eisenstein PAGEMOD(PAGERAD+1,2*PAGERAD+1);
const Eisenstein LETTERMOD(-2,-4);
const complex<double> ZLETTERMOD(0,-2*M_SQRT_3);
const complex<double> omega(-0.5,M_SQRT_3_4); // this is Eisenstein(0,1)
int debugEisenstein;

int Eisenstein::numx,Eisenstein::numy,Eisenstein::denx=0,Eisenstein::deny=0,Eisenstein::quox,Eisenstein::quoy,Eisenstein::remx,Eisenstein::remy;

unsigned long _norm(int x,int y)
{
  return sqr(x)+sqr(y)-x*y;
}

Eisenstein::Eisenstein(complex<double> z)
{
  double norm0,norm1;
  y=lrint(z.imag()/M_SQRT_3_4);
  x=lrint(z.real()+y*0.5);
  norm0=::norm(z-(complex<double> (*this)));
  y++;
  norm1=::norm(z-(complex<double> (*this)));
  if (norm1>norm0)
    y--;
  else
    norm0=norm1;
  y--,x--;
  norm1=::norm(z-(complex<double> (*this)));
  if (norm1>norm0)
    y++,x++;
  else
    norm0=norm1;
  /*x++;
  norm1=::norm(z-(complex<double> (*this)));
  if (norm1>norm0)
    x--;
  else
    norm0=norm1;*/
  y--;
  norm1=::norm(z-(complex<double> (*this)));
  if (norm1>norm0)
    y++;
  else
    norm0=norm1;
  y++,x++;
  norm1=::norm(z-(complex<double> (*this)));
  if (norm1>norm0)
    y--,x--;
  else
    norm0=norm1;
  /*x--;
  norm1=::norm(z-(complex<double> (*this)));
  if (norm1>norm0)
    x++;
  else
    norm0=norm1;*/
}

void Eisenstein::divmod(Eisenstein b)
/* Division and remainder, done together to save time
 * 1     denx       deny
 * 1+ω   denx-deny  denx
 * ω     -deny      denx-deny
 * -1    -denx      -deny
 * -1-ω  deny-denx  -denx
 * -ω    deny       deny-denx
 */
{
  int cont;
  if (this->x!=numx || this->y!=numy || b.x!=denx || b.y!=deny)
  {
    int nrm,nrm1;
    numx=this->x;
    numy=this->y;
    denx=b.x;
    deny=b.y;
    nrm=b.norm();
    if (debugEisenstein)
      printf("%d+%dω/%d+%dω\n",numx,numy,denx,deny);
    // Do a rough division.
    quox=round((numx*denx+numy*deny-numx*deny)/(double)nrm);
    quoy=round((numy*denx-numx*deny)/(double)nrm);
    remx=numx-denx*quox+deny*quoy;
    remy=numy-denx*quoy-deny*quox+deny*quoy;
    // Adjust division so that remainder has least norm.
    // Ties are broken by < or <= for a symmetrical, but eccentric,
    // shape when dividing by LETTERMOD.
    do
    {
      cont=false; // FIXME this loop may need to be optimized
      nrm=_norm(remx,remy);
      nrm1=_norm(remx+denx-deny,remy+denx);
      if (debugEisenstein)
	printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
      if (nrm1<nrm)
      {
	remx=remx+denx-deny;
	remy=remy+denx;
	quox--;
	quoy--;
	cont-=13;
      }
      nrm=_norm(remx,remy);
      nrm1=_norm(remx+deny,remy+deny-denx);
      if (debugEisenstein)
	printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
      if (nrm1<nrm)
      {
	remx=remx+deny;
	remy=remy+deny-denx;
	quoy++;
	cont+=8;
      }
      nrm=_norm(remx,remy);
      nrm1=_norm(remx-denx,remy-deny);
      if (debugEisenstein)
	printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
      if (nrm1<nrm)
      {
	remx=remx-denx;
	remy=remy-deny;
	quox++;
	cont+=5;
      }
      nrm=_norm(remx,remy);
      nrm1=_norm(remx+deny-denx,remy-denx);
      if (debugEisenstein)
	printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
      if (nrm1<nrm)
      {
	remx=remx+deny-denx;
	remy=remy-denx;
	quox++;
	quoy++;
	cont+=13;
      }
      nrm=_norm(remx,remy);
      nrm1=_norm(remx-deny,remy+denx-deny);
      if (debugEisenstein)
	printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
      if (nrm1<nrm)
      {
	remx=remx-deny;
	remy=remy+denx-deny;
	quoy--;
	cont-=8;
      }
      nrm=_norm(remx,remy);
      nrm1=_norm(remx+denx,remy+deny);
      if (debugEisenstein)
	printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
      if (nrm1<nrm)
      {
	remx=remx+denx;
	remy=remy+deny;
	quox--;
	cont-=5;
      }
      if (debugEisenstein)
	printf("loop\n");
      } while (0);
    nrm=_norm(remx,remy);
    nrm1=_norm(remx+denx,remy+deny);
    if (debugEisenstein)
      printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
    if (nrm1<=nrm)
    {
      remx=remx+denx;
      remy=remy+deny;
      quox--;
    }
    if (debugEisenstein)
      printf("quo=%d+%dω rem=%d+%dω \n",quox,quoy,remx,remy);
    nrm=_norm(remx,remy);
    nrm1=_norm(remx-deny+denx,remy+denx);
    if (debugEisenstein)
      printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
    if (nrm1<=nrm)
    {
      remx=remx-deny+denx;
      remy=remy+denx;
      quox--;
      quoy--;
    }
    nrm=_norm(remx,remy);
    nrm1=_norm(remx+deny,remy-denx+deny);
    if (debugEisenstein)
      printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
    if (nrm1<=nrm)
    {
      remx=remx+deny;
      remy=remy-denx+deny;
      quoy++;
    }
  }
}

Eisenstein Eisenstein::operator+(Eisenstein b)
{
  return Eisenstein(this->x+b.x,this->y+b.y);
}

Eisenstein& Eisenstein::operator+=(Eisenstein b)
{
  this->x+=b.x,this->y+=b.y;
  return *this;
}

Eisenstein Eisenstein::operator-()
{
  return Eisenstein(-this->x,-this->y);
}

Eisenstein Eisenstein::operator-(Eisenstein b)
{
  return Eisenstein(this->x-b.x,this->y-b.y);
}

bool operator<(const Eisenstein a,const Eisenstein b)
// These numbers are complex, so there is no consistent < operator on them.
// This operator is used only to give some order to the map.
{
  if (a.y!=b.y)
    return a.y<b.y;
  else
    return a.x<b.x;
}

Eisenstein Eisenstein::operator*(Eisenstein b)
{
  return Eisenstein(x*b.x-y*b.y,x*b.y+y*b.x-y*b.y);
}

Eisenstein& Eisenstein::operator*=(Eisenstein b)
{
  int tmp;
  tmp=x*b.x-y*b.y;
  y=x*b.y+y*b.x-y*b.y;
  x=tmp;
  return *this;
}

Eisenstein Eisenstein::operator/(Eisenstein b)
{if (b==0)
    throw(domain_error("Divide by zero Eisenstein integer"));
 divmod(b);
 return Eisenstein(quox,quoy);
 }

Eisenstein Eisenstein::operator%(Eisenstein b)
{
  if (b==0)
    return (*this); // Dividing by zero is an error, but modding by zero is not.
  else
  {
    divmod(b);
    return Eisenstein(remx,remy);
  }
}

bool Eisenstein::operator==(Eisenstein b)
{
  return this->x==b.x && this->y==b.y;
}

bool Eisenstein::operator!=(Eisenstein b)
{
  return this->x!=b.x || this->y!=b.y;
}

unsigned long Eisenstein::norm()
{
  return sqr(this->x)+sqr(this->y)-this->x*this->y;
}

Eisenstein nthEisenstein(int n,int size,int nelts)
{
  int x,y,row;
  assert (n>=0 && n<nelts);
  n-=nelts/2;
  if (n<0)
  {
    for (n-=size,row=2*size+1,y=0;n<=0;n+=row--,y--)
      ;
    y++;
    n-=++row;
    x=n+y+size;
  }
  else
  {
    for (n+=size,row=2*size+1,y=0;n>=0;n-=row--,y++)
      ;
    y--;
    n+=++row;
    x=n+y-size;
  }
  Eisenstein a(x,y);
  return a;
}

int Eisenstein::pageinx(int size,int nelts)
// Index to a byte within a page of specified size. Used in the inverse
// letter table as well as the paging of harray.
{
  if (y<0)
    return (-y-size)*(-y-3*size-3)/2+x-y;
  else
    return x-y+nelts-(size-y)*(3*size+3-y)/2-1;
}

int Eisenstein::pageinx()
// Index to a byte within a page. Meaningful only if the number
// is a remainder of division by PAGEMOD.
{
  return pageinx(PAGERAD,PAGESIZE);
}

// Iteration: start, inc, cont. Iterates over a hexagon.

Eisenstein start(int n)
{
  if (n<0)
    throw(out_of_range("Eisenstein start: n<0"));
  return Eisenstein(-n,-n);
}

void Eisenstein::inc(int n)
{
  if (n<0)
    throw(out_of_range("Eisenstein::inc: n<0"));
  x++;
  if (y<0)
    if (x-y>n)
    {
      y++;
      x=-n;
    }
    else;
  else
    if (x>n)
    {
      y++;
      x=y-n;
    }
}

bool Eisenstein::cont(int n)
{
  return y<=n;
}

int Eisenstein::letterinx()
{
  switch (y)
  {
    case 1:
      return 11-x;
    case 0:
      return 8-x;
    case -1:
      return 4-x;
    case -2:
      return -x;
    default:
      return 32768;
  }
}

void testpageinx()
{
  int x,y;
  Eisenstein h,g;
  for (y=PAGERAD;y>=-PAGERAD;y--)
  {
    if (y&1)
      printf("  ");
    for (x=-PAGERAD+((y+1)&-2)/2;x<-PAGERAD || x-y<-PAGERAD;x++)
      printf("    ");
    for (;x<=PAGERAD && x-y<=PAGERAD;x++)
    {
      h=Eisenstein(x,y);
      printf("%4d",h.pageinx());
    }
    printf("\n");
  }
  for (y=PAGERAD;y>=-PAGERAD;y--)
  {
    if (y&1)
      printf("   ");
    for (x=-PAGERAD+((y+1)&-2)/2;x<-PAGERAD || x-y<-PAGERAD;x++)
      printf("      ");
    for (;x<=PAGERAD && x-y<=PAGERAD;x++)
    {
      h=Eisenstein(x,y);
      g=nthEisenstein(h.pageinx(),PAGERAD,PAGESIZE);
      printf("%2d,%-2d ",g.getx(),g.gety());
      assert(g==h);
    }
    printf("\n");
  }
}

int region(complex<double> z)
/* z is in the unit hexagon. Returns which of 13 regions z is in.
 * Examples of a point in each region, and area relative to the hexagon:
 *  0  0.000, 0.000 0.6802 1812 (9/16)π/(3/2)sqrt(3)
 *  1  0.000, 0.500 0.0417  111 1/24
 *  2  0.433, 0.250 0.0417  111
 *  3  0.433,-0.250 0.0417  111
 *  4  0.000,-0.500 0.0417  111
 *  5 -0.433,-0.250 0.0417  111
 *  6 -0.433, 0.250 0.0417  111
 *  7  0.466, 0.000 0.0116   31
 *  8  0.233,-0.407 0.0116   31
 *  9 -0.233,-0.407 0.0116   31
 * 10 -0.466, 0.000 0.0116   31
 * 11 -0.233, 0.407 0.0116   31
 * 12  0.233, 0.407 0.0116   31
 * ----------------------------
 *     0.000, 0.000 1.000  2664
 * Region 0 is a circle; the rest of the regions are formed by drawing
 * the common tangents of the circles.
 * Centroid of region 7 (needed for inverse letter table):
 * Centroid of the kite is ((2/8)*3+(10/24)*1)/4=7/24, with weight 3/4.
 * Centroid of the sector is (3/π)*(2/3)*sqrt(3/16) (0.2757), with weight -0.6802.
 * Centroid of the sliver is thus 0.4475459119.
 */
{
  int reg,outside[6],inside[6],i;
  if (norm(z)<3./16)
    reg=0;
  else
  {
    for (i=0;i<6;i+=2)
    {
      outside[(i+0)%6]=z.imag()>M_SQRT_3_4/2;
      outside[(i+3)%6]=z.imag()<-M_SQRT_3_4/2;
      inside[(i+0)%6]=z.real()>0.375;
      inside[(i+3)%6]=z.real()<-0.375;
      z*=omega;
    }
    for (reg=i=0;i<6 && reg==0;i++)
      if (outside[i])
	reg=i+1;
    for (i=0;i<6 && reg==0;i++)
      if (inside[i])
	reg=i+7;
  }
  return reg;
}

void testcomplex()
{
  complex<double> z=8191,r(0.8,0.6),z2,diff;
  int i,histo[13],reg;
  Eisenstein h;
  PostScript ps;
  memset(histo,0,sizeof(histo));
  ps.open("testcomplex.ps");
  ps.prolog();
  ps.startpage();
  for (i=0;i<26640;i++)
  {
    z*=r;
    h=z;
    z2=h;
    diff=z-z2;
    //cout<<diff<<endl;
    reg=region(diff);
    histo[reg]++;
    switch (reg)
    {
      case 0:
	ps.setcolor(0,0,0);
	break;
      case 1:
	ps.setcolor(0,0,1);
	break;
      case 2:
	ps.setcolor(0,.5,.5);
	break;
      case 3:
	ps.setcolor(0,1,0);
	break;
      case 4:
	ps.setcolor(.5,.5,0);
	break;
      case 5:
	ps.setcolor(1,0,0);
	break;
      case 6:
	ps.setcolor(.5,0,.5);
	break;
      case 7:
	ps.setcolor(1,1,0);
	break;
      case 8:
	ps.setcolor(1,.5,.5);
	break;
      case 9:
	ps.setcolor(1,0,1);
	break;
      case 10:
	ps.setcolor(.5,.5,1);
	break;
      case 11:
	ps.setcolor(0,1,1);
	break;
      case 12:
	ps.setcolor(.5,1,.5);
	break;
    }
    ps.dot(xy(diff.real()*100,diff.imag()*100));
  }
  for (i=0;i<13;i++)
    printf("%2d %5d\n",i,histo[i]);
  h=Eisenstein(0,1);
  z=h;
  cout<<z<<endl;
  ps.endpage();
  ps.trailer();
  ps.close();
}
