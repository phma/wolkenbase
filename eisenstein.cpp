/******************************************************/
/*                                                    */
/* eisenstein.cpp - Eisenstein integers               */
/*                                                    */
/******************************************************/
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

//Page size for storing arrays subscripted by hvec
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
const hvec PAGEMOD(PAGERAD+1,2*PAGERAD+1);
const hvec LETTERMOD(-2,-4);
const complex<double> ZLETTERMOD(0,-2*M_SQRT_3);
const complex<double> omega(-0.5,M_SQRT_3_4); // this is hvec(0,1)
int debughvec;

int hvec::numx,hvec::numy,hvec::denx=0,hvec::deny=0,hvec::quox,hvec::quoy,hvec::remx,hvec::remy;

unsigned long _norm(int x,int y)
{return sqr(x)+sqr(y)-x*y;
 }

hvec::hvec(complex<double> z)
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

void hvec::divmod(hvec b)
/* Division and remainder, done together to save time
 * 1     denx       deny
 * 1+ω   denx-deny  denx
 * ω     -deny      denx-deny
 * -1    -denx      -deny
 * -1-ω  deny-denx  -denx
 * -ω    deny       deny-denx
 */
{int cont;
 if (this->x!=numx || this->y!=numy || b.x!=denx || b.y!=deny)
    {int nrm,nrm1;
     numx=this->x;
     numy=this->y;
     denx=b.x;
     deny=b.y;
     nrm=b.norm();
     if (debughvec)
        printf("%d+%dω/%d+%dω\n",numx,numy,denx,deny);
     // Do a rough division.
     quox=round((numx*denx+numy*deny-numx*deny)/(double)nrm);
     quoy=round((numy*denx-numx*deny)/(double)nrm);
     remx=numx-denx*quox+deny*quoy;
     remy=numy-denx*quoy-deny*quox+deny*quoy;
     // Adjust division so that remainder has least norm.
     // Ties are broken by < or <= for a symmetrical, but eccentric,
     // shape when dividing by LETTERMOD.
     do {cont=false; // FIXME this loop may need to be optimized
         nrm=_norm(remx,remy);
         nrm1=_norm(remx+denx-deny,remy+denx);
         if (debughvec)
            printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
         if (nrm1<nrm)
            {remx=remx+denx-deny;
             remy=remy+denx;
             quox--;
	     quoy--;
	     cont-=13;
	     }
         nrm=_norm(remx,remy);
         nrm1=_norm(remx+deny,remy+deny-denx);
         if (debughvec)
            printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
         if (nrm1<nrm)
            {remx=remx+deny;
             remy=remy+deny-denx;
             quoy++;
	     cont+=8;
	     }
         nrm=_norm(remx,remy);
         nrm1=_norm(remx-denx,remy-deny);
         if (debughvec)
            printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
         if (nrm1<nrm)
            {remx=remx-denx;
             remy=remy-deny;
             quox++;
	     cont+=5;
	     }
         nrm=_norm(remx,remy);
         nrm1=_norm(remx+deny-denx,remy-denx);
         if (debughvec)
            printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
         if (nrm1<nrm)
            {remx=remx+deny-denx;
             remy=remy-denx;
             quox++;
	     quoy++;
	     cont+=13;
	     }
         nrm=_norm(remx,remy);
         nrm1=_norm(remx-deny,remy+denx-deny);
         if (debughvec)
            printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
         if (nrm1<nrm)
            {remx=remx-deny;
             remy=remy+denx-deny;
             quoy--;
	     cont-=8;
	     }
         nrm=_norm(remx,remy);
         nrm1=_norm(remx+denx,remy+deny);
         if (debughvec)
            printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
         if (nrm1<nrm)
            {remx=remx+denx;
             remy=remy+deny;
             quox--;
	     cont-=5;
	     }
	 if (debughvec)
            printf("loop\n");
	 } while (0);
     nrm=_norm(remx,remy);
     nrm1=_norm(remx+denx,remy+deny);
     if (debughvec)
        printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
     if (nrm1<=nrm)
        {remx=remx+denx;
         remy=remy+deny;
         quox--;
         }
     if (debughvec)
        printf("quo=%d+%dω rem=%d+%dω \n",quox,quoy,remx,remy);
     nrm=_norm(remx,remy);
     nrm1=_norm(remx-deny+denx,remy+denx);
     if (debughvec)
        printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
     if (nrm1<=nrm)
        {remx=remx-deny+denx;
         remy=remy+denx;
         quox--;
	 quoy--;
	 }
     nrm=_norm(remx,remy);
     nrm1=_norm(remx+deny,remy-denx+deny);
     if (debughvec)
        printf("quo=%d+%dω rem=%d+%dω nrm=%d nrm1=%d\n",quox,quoy,remx,remy,nrm,nrm1);
     if (nrm1<=nrm)
        {remx=remx+deny;
         remy=remy-denx+deny;
         quoy++;
	 }
     }
 }

hvec hvec::operator+(hvec b)
{return hvec(this->x+b.x,this->y+b.y);
 }

hvec& hvec::operator+=(hvec b)
{this->x+=b.x,this->y+=b.y;
 return *this;
 }

hvec hvec::operator-()
{return hvec(-this->x,-this->y);
 }

hvec hvec::operator-(hvec b)
{return hvec(this->x-b.x,this->y-b.y);
 }

bool operator<(const hvec a,const hvec b)
// These numbers are complex, so there is no consistent < operator on them.
// This operator is used only to give some order to the map.
{if (a.y!=b.y)
    return a.y<b.y;
 else
    return a.x<b.x;
 }

hvec hvec::operator*(hvec b)
{return hvec(x*b.x-y*b.y,x*b.y+y*b.x-y*b.y);
 }

hvec& hvec::operator*=(hvec b)
{int tmp;
 tmp=x*b.x-y*b.y;
 y=x*b.y+y*b.x-y*b.y;
 x=tmp;
 return *this;
 }

hvec hvec::operator/(hvec b)
{if (b==0)
    throw(domain_error("Divide by zero Eisenstein integer"));
 divmod(b);
 return hvec(quox,quoy);
 }

hvec hvec::operator%(hvec b)
{if (b==0)
    return (*this); // Dividing by zero is an error, but modding by zero is not.
 else
    {divmod(b);
     return hvec(remx,remy);
     }
 }

bool hvec::operator==(hvec b)
{return this->x==b.x && this->y==b.y;
 }

bool hvec::operator!=(hvec b)
{return this->x!=b.x || this->y!=b.y;
 }

unsigned long hvec::norm()
{return sqr(this->x)+sqr(this->y)-this->x*this->y;
 }

hvec nthhvec(int n,int size,int nelts)
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
  hvec a(x,y);
  return a;
}

int hvec::pageinx(int size,int nelts)
// Index to a byte within a page of specified size. Used in the inverse
// letter table as well as the paging of harray.
{
  if (y<0)
    return (-y-size)*(-y-3*size-3)/2+x-y;
  else
    return x-y+nelts-(size-y)*(3*size+3-y)/2-1;
}

int hvec::pageinx()
// Index to a byte within a page. Meaningful only if the number
// is a remainder of division by PAGEMOD.
{
  return pageinx(PAGERAD,PAGESIZE);
}

// Iteration: start, inc, cont. Iterates over a hexagon.

hvec start(int n)
{if (n<0)
    throw(out_of_range("hvec start: n<0"));
 return hvec(-n,-n);
 }

void hvec::inc(int n)
{if (n<0)
    throw(out_of_range("hvec::inc: n<0"));
 x++;
 if (y<0)
    if (x-y>n)
       {y++;
        x=-n;
        }
    else;
 else
    if (x>n)
       {y++;
        x=y-n;
        }
 }

bool hvec::cont(int n)
{return y<=n;
 }

int hvec::letterinx()
{switch (y)
   {case 1:
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
  hvec h,g;
  for (y=PAGERAD;y>=-PAGERAD;y--)
  {
    if (y&1)
      printf("  ");
    for (x=-PAGERAD+((y+1)&-2)/2;x<-PAGERAD || x-y<-PAGERAD;x++)
      printf("    ");
    for (;x<=PAGERAD && x-y<=PAGERAD;x++)
    {
      h=hvec(x,y);
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
      h=hvec(x,y);
      g=nthhvec(h.pageinx(),PAGERAD,PAGESIZE);
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
  hvec h;
  memset(histo,0,sizeof(histo));
  psopen("testcomplex.ps");
  psprolog();
  startpage();
  for (i=0;i<32768;i++)
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
	setcolor(0,0,0);
	break;
      case 1:
	setcolor(0,0,1);
	break;
      case 2:
	setcolor(0,.5,.5);
	break;
      case 3:
	setcolor(0,1,0);
	break;
      case 4:
	setcolor(.5,.5,0);
	break;
      case 5:
	setcolor(1,0,0);
	break;
      case 6:
	setcolor(.5,0,.5);
	break;
      case 7:
	setcolor(1,1,0);
	break;
      case 8:
	setcolor(1,.5,.5);
	break;
      case 9:
	setcolor(1,0,1);
	break;
      case 10:
	setcolor(.5,.5,1);
	break;
      case 11:
	setcolor(0,1,1);
	break;
      case 12:
	setcolor(.5,1,.5);
	break;
    }
    plotpoint(diff.real()*100,diff.imag()*100);
  }
  for (i=0;i<13;i++)
    printf("%2d %5d\n",i,histo[i]);
  h=hvec(0,1);
  z=h;
  cout<<z<<endl;
  endpage();
  pstrailer();
  psclose();
}

/* Six-dimensional vectors. A two-dimensional torus lies in a four-dimensional plane
 * in six-dimensional space. Each point in the unit hexagon is mapped to a point
 * on the torus so that edges are identified. They are used to compute an average
 * of the frames that produce a framing error reading.
 */
sixvec::sixvec()
{
  memset(v,0,sizeof(v));
}

sixvec::sixvec(complex<double> z)
{
  v[0]=cos(z.imag()*2*M_PI/M_SQRT_3_4);
  v[1]=sin(z.imag()*2*M_PI/M_SQRT_3_4);
  z*=omega;
  v[2]=cos(z.imag()*2*M_PI/M_SQRT_3_4);
  v[3]=sin(z.imag()*2*M_PI/M_SQRT_3_4);
  z*=omega;
  v[4]=cos(z.imag()*2*M_PI/M_SQRT_3_4);
  v[5]=sin(z.imag()*2*M_PI/M_SQRT_3_4);
}

sixvec sixvec::operator+(const sixvec b)
{
  int i;
  sixvec a;
  for (i=0;i<6;i++)
    a.v[i]=this->v[i]+b.v[i];
  return a;
}

sixvec sixvec::operator-(const sixvec b)
{
  int i;
  sixvec a;
  for (i=0;i<6;i++)
    a.v[i]=this->v[i]-b.v[i];
  return a;
}

sixvec sixvec::operator*(const double b)
{
  int i;
  sixvec a;
  for (i=0;i<6;i++)
    a.v[i]=this->v[i]*b;
  return a;
}

sixvec sixvec::operator/(const double b)
{
  int i;
  sixvec a;
  for (i=0;i<6;i++)
    a.v[i]=this->v[i]/b;
  return a;
}

double sixvec::norm()
{
  int i;
  double sum;
  for (sum=i=0;i<6;i++)
    sum+=this->v[i]*this->v[i];
  return sqrt(sum);
}

sixvec sixvec::operator+=(const sixvec b)
{
  int i;
  for (i=0;i<6;i++)
    this->v[i]+=b.v[i];
  return *this;
}

sixvec sixvec::operator/=(const double b)
{
  int i;
  for (i=0;i<6;i++)
    this->v[i]/=b;
  return *this;
}

bool sixvec::operator!=(const sixvec b)
{
  int i;
  bool a;
  for (i=0,a=false;i<6;i++)
    a|=this->v[i]!=b.v[i];
  return a;
}

void testsixvec()
{
  complex<double> z=8191,r(0.8,0.6),z2,diff;
  int i,reg;
  hvec h;
  sixvec s,a(complex<double>(0,0)),b(complex<double>(0,M_SQRT_3_4/2)),c(complex<double>(0,-M_SQRT_3_4/2));
  psopen("testsixvec.ps");
  psprolog();
  startpage();
  for (i=0;i<32768;i++)
  {
    z*=r;
    h=z;
    z2=h;
    diff=z-z2;
    s=sixvec(z);
    setcolor((s-a).norm()/2/M_SQRT_3,(s-b).norm()/2/M_SQRT_3,(s-c).norm()/2/M_SQRT_3);
    //cout<<diff<<endl;
    plotpoint(diff.real()*100,diff.imag()*100);
  }
  endpage();
  pstrailer();
  psclose();
}
