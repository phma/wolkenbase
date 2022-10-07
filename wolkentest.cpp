/******************************************************/
/*                                                    */
/* wolkentest.cpp - test program                      */
/*                                                    */
/******************************************************/
/* Copyright 2019-2022 Pierre Abbat.
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
#include <cassert>
#include <cstring>
#include <vector>
#include <string>
#include "config.h"
#include "angle.h"
#include "octree.h"
#include "shape.h"
#include "testpattern.h"
#include "eisenstein.h"
#include "random.h"
#include "ldecimal.h"
#include "ps.h"
#include "freeram.h"
#include "flowsnake.h"
#include "peano.h"
#include "relprime.h"
#include "manysum.h"
#include "matrix.h"
#include "leastsquares.h"

#define tassert(x) testfail|=(!(x))
//#define tassert(x) if (!(x)) {testfail=true; sleep(10);}
// so that tests still work in non-debug builds

using namespace std;

char hexdig[16]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
bool slowmanysum=false;
bool testfail=false;
vector<string> args;

xy randomInCircle()
{
  int n=rng.usrandom();
  double r=sqrt(n+0.5)/256;
  return cossin((int)(n*PHITURN))*r;
}

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

void testintegertrig()
{
  double sinerror,coserror,ciserror,totsinerror,totcoserror,totciserror;
  int i;
  char bs=8;
  for (totsinerror=totcoserror=totciserror=i=0;i<128;i++)
  {
    sinerror=sin(i<<24)+sin((i+64)<<24);
    coserror=cos(i<<24)+cos((i+64)<<24);
    ciserror=hypot(cos(i<<24),sin(i<<24))-1;
    if (sinerror>0.04 || coserror>0.04 || ciserror>0.04)
    {
      printf("sin(%8x)=%a sin(%8x)=%a\n",i<<24,sin(i<<24),(i+64)<<24,sin((i+64)<<24));
      printf("cos(%8x)=%a cos(%8x)=%a\n",i<<24,cos(i<<24),(i+64)<<24,cos((i+64)<<24));
      printf("abs(cis(%8x))=%a\n",i<<24,hypot(cos(i<<24),sin(i<<24)));
    }
    totsinerror+=sinerror*sinerror;
    totcoserror+=coserror*coserror;
    totciserror+=ciserror*ciserror;
  }
  printf("total sine error=%e\n",totsinerror);
  printf("total cosine error=%e\n",totcoserror);
  printf("total cis error=%e\n",totciserror);
  tassert(totsinerror+totcoserror+totciserror<2e-29);
  //On Linux, the total error is 2e-38 and the M_PIl makes a big difference.
  //On DragonFly BSD, the total error is 1.7e-29 and M_PIl is absent.
  tassert(bintodeg(0)==0);
  tassert(fabs(bintodeg(0x15555555)-60)<0.0000001);
  tassert(fabs(bintomin(0x08000000)==1350));
  tassert(fabs(bintosec(0x12345678)-184320)<0.001);
  tassert(fabs(bintogon(0x1999999a)-80)<0.0000001);
  tassert(fabs(bintorad(0x4f1bbcdd)-3.88322208)<0.00000001);
  for (i=-2147400000;i<2147400000;i+=rng.usrandom()+18000)
  {
    cout<<setw(11)<<i<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs;
    cout.flush();
    tassert(degtobin(bintodeg(i))==i);
    tassert(mintobin(bintomin(i))==i);
    tassert(sectobin(bintosec(i))==i);
    tassert(gontobin(bintogon(i))==i);
    tassert(radtobin(bintorad(i))==i);
    tassert(foldangle(atan2i(cossin(i)))==foldangle(i));
  }
  tassert(sectobin(1295999.9999)==-2147483648);
  tassert(sectobin(1296000.0001)==-2147483648);
  tassert(sectobin(-1295999.9999)==-2147483648);
  tassert(sectobin(-1296000.0001)==-2147483648);
  cout<<"           "<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs;
  cout<<"atan2a "<<0x40000<<' '<<ldecimal(atan2a(sin(0x40000),cos(0x40000)))<<' '<<ldecimal(tan(0x40000))<<endl;
  cout<<"atan2a "<<0x80000<<' '<<ldecimal(atan2a(sin(0x80000),cos(0x80000)))<<' '<<ldecimal(tan(0x80000))<<endl;
  cout<<"atan2i(0,0)="<<hex<<atan2i(0,0)<<dec<<endl;
}

void testflat()
{
  ofstream dumpFile("flat.dump");
  PostScript ps;
  xy center=9*randomInCircle();
  vector<long long> blocks;
  vector<LasPoint> points;
  int i;
  Cylinder cyl(center,1);
  setStorePoints(true);
  flatScene(10,100);
  blocks=octRoot.findBlocks(cyl);
  cout<<"Blocks:";
  for (i=0;i<blocks.size();i++)
    cout<<' '<<blocks[i];
  cout<<endl;
  points=octStore.pointsIn(cyl);
  cout<<points.size()<<" points in cylinder\n";
  tassert(abs((int)points.size()-314)<8);
  ps.open("flat.ps");
  ps.setpaper(papersizes["A4 landscape"],0);
  ps.prolog();
  octStore.plot(ps);
  octStore.dump(dumpFile);
}

xyz findIntersection(Shape &shape,xyz a,xyz b)
{
  bool ain=shape.in(a),bin=shape.in(b),min;
  xyz m;
  assert(ain^bin);
  while (true)
  {
    m=(a+b)/2;
    if (a==m || m==b)
      break;
    min=shape.in(m);
    if (min==ain)
      a=m;
    else
      b=m;
  }
  return m;
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

void testsphere()
{
  Sphere s1(xyz(100,200,300),89);
  /* 89²=7921 is a square that can be expressed as the sum of three different
   * positive squares in more than one way.
   * 89²=15²+36²+80²=39²+48²+64²
   */
  xyz cen(100,200,300),a(115,164,220),b(115,163,220),c(36,239,252),d(36,240,252);
  xyz e(64,120,315),f(63,120,315),g(164,248,339),h(164,248,340);
  tassert(s1.in(cen));
  tassert(s1.in(a));
  tassert(!s1.in(b));
  tassert(s1.in(c));
  tassert(!s1.in(d));
  tassert(s1.in(e));
  tassert(!s1.in(f));
  tassert(s1.in(g));
  tassert(!s1.in(h));
}

void testhyperboloid()
{
  xyz ver(100,200,300),ocen1(100,200,228),ocen2(100,200,203);
  xyz topEndCur(100.6,200.8,300),bottomEndCur(100.6,200.8,228);
  xyz topEndAsy1(3000100,4000200,300),bottomEndAsy1(3000100,4000200,-4e9);
  xyz topEndAsy2(6000100,8000200,300),bottomEndAsy2(6000100,8000200,-4e9);
  double sint,hint,aint1,aint2;
  Hyperboloid h1(ver,72,1);
  /* 72²=5184 is a square that can be expressed as the difference of three different
   * positive squares in more than one way.
   * 72²=78²-18²-24²=97²-25²-60²
   */
  Sphere s1(ocen1,72);
  Hyperboloid h2(ver,97,2);
  Sphere s2(ocen2,97);
  xyz a(118,176,294),b(75,260,275);
  xyz c(118,176,295),d(75,260,276);
  xyz e(118,176,293),f(75,260,274);
  tassert(h1.in(a));
  tassert(h1.in(b));
  tassert(!h1.in(c));
  tassert(!h1.in(d));
  tassert(h1.in(e));
  tassert(h1.in(f));
  sint=findIntersection(s1,topEndCur,bottomEndCur).getz();
  hint=findIntersection(h1,topEndCur,bottomEndCur).getz();
  cout<<"Sphere intersection "<<ldecimal(sint)<<" Hyperboloid intersection "<<ldecimal(hint)<<endl;
  tassert(fabs(sint-hint)<1/sqr(72.)); // Check curvatures are equal. The actual difference is 1/(72**3*4).
  aint1=findIntersection(h1,topEndAsy1,bottomEndAsy1).getz();
  aint2=findIntersection(h1,topEndAsy2,bottomEndAsy2).getz();
  cout<<"Asymptotic slope "<<ldecimal((aint1-aint2)/5e6)<<endl;
  tassert(fabs(1-(aint1-aint2)/5e6)<1e-6);
  sint=findIntersection(s2,topEndCur,bottomEndCur).getz();
  hint=findIntersection(h2,topEndCur,bottomEndCur).getz();
  cout<<"Sphere intersection "<<ldecimal(sint)<<" Hyperboloid intersection "<<ldecimal(hint)<<endl;
  tassert(fabs(sint-hint)<1/sqr(97.));
  aint1=findIntersection(h2,topEndAsy1,bottomEndAsy1).getz();
  aint2=findIntersection(h2,topEndAsy2,bottomEndAsy2).getz();
  cout<<"Asymptotic slope "<<ldecimal((aint1-aint2)/5e6)<<endl;
  tassert(fabs(2-(aint1-aint2)/5e6)<1e-6);
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

void knowndet(matrix &mat)
/* Sets mat to a triangular matrix with ones on the diagonal, which is known
 * to have determinant 1, then permutes the rows and columns so that Gaussian
 * elimination will mess up the entries.
 *
 * The number of rows should be 40 at most. More than that, and it will not
 * be shuffled well.
 */
{
  int i,j,ran,rr,rc,cc,flipcount,size,giveup;
  mat.setidentity();
  size=mat.getrows();
  for (i=0;i<size;i++)
    for (j=0;j<i;j++)
      mat[i][j]=(rng.ucrandom()*2-255)/BYTERMS;
  for (flipcount=giveup=0;(flipcount<2*size || (flipcount&1)) && giveup<10000;giveup++)
  { // If flipcount were odd, the determinant would be -1 instead of 1.
    ran=rng.usrandom();
    rr=ran%size;
    ran/=size;
    rc=ran%size;
    ran/=size;
    cc=ran%size;
    if (rr!=rc)
    {
      flipcount++;
      mat.swaprows(rr,rc);
    }
    if (rc!=cc)
    {
      flipcount++;
      mat.swapcolumns(rc,cc);
    }
  }
}

void dumpknowndet(matrix &mat)
{
  int i,j,byte;
  for (i=0;i<mat.getrows();i++)
    for (j=0;j<mat.getcolumns();j++)
    {
      if (mat[i][j]==0)
	cout<<"z0";
      else if (mat[i][j]==1)
	cout<<"z1";
      else
      {
	byte=rint(mat[i][j]*BYTERMS/2+127.5);
	cout<<hexdig[byte>>4]<<hexdig[byte&15];
      }
    }
  cout<<endl;
}

void loadknowndet(matrix &mat,string dump)
{
  int i,j,byte;
  string item;
  for (i=0;i<mat.getrows();i++)
    for (j=0;j<mat.getcolumns();j++)
    {
      item=dump.substr(0,2);
      dump.erase(0,2);
      if (item[0]=='z')
	mat[i][j]=item[1]-'0';
      else
      {
	byte=stoi(item,0,16);
	mat[i][j]=(byte*2-255)/BYTERMS;
      }
    }
  cout<<endl;
}

void testmatrix()
{
  int i,j,chk2,chk3,chk4;
  matrix m1(3,4),m2(4,3),m3(4,3),m4(4,3);
  matrix t1(37,41),t2(41,43),t3(43,37),p1,p2,p3;
  matrix t1t,t2t,t3t,p1t,p2t,p3t;
  matrix hil(8,8),lih(8,8),hilprod;
  matrix kd(7,7);
  matrix r0,c0,p11;
  matrix rs1(3,4),rs2,rs3,rs4;
  vector<double> rv,cv;
  double tr1,tr2,tr3,de1,de2,de3,tr1t,tr2t,tr3t;
  double toler=1.2e-12;
  double kde;
  double lo,hi;
  manysum lihsum;
  m1[2][0]=5;
  m1[1][3]=7;
  tassert(m1[2][0]==5);
  m2[2][0]=9;
  m2[1][4]=6;
  tassert(m2[2][0]==9);
  for (i=0;i<4;i++)
    for (j=0;j<3;j++)
    {
      m2[i][j]=rng.ucrandom();
      m3[i][j]=rng.ucrandom();
    }
  for (chk2=chk3=i=0;i<4;i++)
    for (j=0;j<3;j++)
    {
      chk2=(50*chk2+(int)m2[i][j])%83;
      chk3=(50*chk3+(int)m3[i][j])%83;
    }
  m4=m2+m3;
  for (chk4=i=0;i<4;i++)
    for (j=0;j<3;j++)
      chk4=(50*chk4+(int)m4[i][j])%83;
  tassert(chk4==(chk2+chk3)%83);
  m4=m2-m3;
  for (chk4=i=0;i<4;i++)
    for (j=0;j<3;j++)
      chk4=(50*chk4+(int)m4[i][j]+332)%83;
  tassert(chk4==(chk2-chk3+83)%83);
  lo=INFINITY;
  hi=0;
  for (i=0;i<1;i++)
  {
    t1.randomize_c();
    t2.randomize_c();
    t3.randomize_c();
    t1t=t1.transpose();
    t2t=t2.transpose();
    t3t=t3.transpose();
    p1=t1*t2*t3;
    p2=t2*t3*t1;
    p3=t3*t1*t2;
    p1t=t3t*t2t*t1t;
    p2t=t1t*t3t*t2t;
    p3t=t2t*t1t*t3t;
    tr1=p1.trace();
    tr2=p2.trace();
    tr3=p3.trace();
    tr1t=p1t.trace();
    tr2t=p2t.trace();
    tr3t=p3t.trace();
    de1=p1.determinant();
    de2=p2.determinant();
    de3=p3.determinant();
    cout<<"trace1 "<<ldecimal(tr1)
	<<" trace2 "<<ldecimal(tr2)
	<<" trace3 "<<ldecimal(tr3)<<endl;
    tassert(fabs(tr1-tr2)<toler && fabs(tr2-tr3)<toler && fabs(tr3-tr1)<toler);
    tassert(fabs(tr1-tr1t)<toler && fabs(tr2-tr2t)<toler && fabs(tr3-tr3t)<toler);
    tassert(tr1!=0);
    cout<<"det1 "<<de1
	<<" det2 "<<de2
	<<" det3 "<<de3<<endl;
    tassert(fabs(de1)>1e80 && fabs(de2)<1e60 && fabs(de3)<1e52);
    // de2 and de3 would actually be 0 with exact arithmetic.
    if (fabs(de2)>hi)
      hi=fabs(de2);
    if (fabs(de2)<lo)
      lo=fabs(de2);
  }
  cout<<"Lowest det2 "<<lo<<" Highest det2 "<<hi<<endl;
  for (i=0;i<8;i++)
    for (j=0;j<8;j++)
      hil[i][j]=1./(i+j+1);
  lih=invert(hil);
  for (i=0;i<8;i++)
    for (j=0;j<i;j++)
      lihsum+=fabs(lih[i][j]-lih[j][i]);
  cout<<"Total asymmetry of inverse of Hilbert matrix is "<<lihsum.total()<<endl;
  hilprod=hil*lih;
  lihsum.clear();
  for (i=0;i<8;i++)
    for (j=0;j<8;j++)
      lihsum+=fabs(hilprod[i][j]-(i==j));
  cout<<"Total error of Hilbert matrix * inverse is "<<lihsum.total()<<endl;
  tassert(lihsum.total()<2e-5 && lihsum.total()>1e-15);
  for (i=0;i<1;i++)
  {
    knowndet(kd);
    //loadknowndet(kd,"z0z0z0z1c9dd28z0z1z03c46c35cz0z0z0z0z1z0z0aa74z169f635e3z0z0z0z003z0z1z0z0z0z0fcz146z160z000f50091");
    //loadknowndet(kd,"a6z1fc7ce056d6d1z0z0z0z0z1b49ez0z0z1z02f53z1z0z0z0z0z0z0e2z0z097z1230488z0z19b25484e1ez0z0z0z0z0z1");
    kd.dump();
    dumpknowndet(kd);
    kde=kd.determinant();
    cout<<"Determinant of shuffled matrix is "<<ldecimal(kde)<<" diff "<<kde-1<<endl;
    tassert(fabs(kde-1)<4e-12);
  }
  for (i=0;i<11;i++)
  {
    rv.push_back((i*i*i)%11);
    cv.push_back((i*3+7)%11);
  }
  r0=rowvector(rv);
  c0=columnvector(cv);
  p1=r0*c0;
  p11=c0*r0;
  tassert(p1.trace()==253);
  tassert(p11.trace()==253);
  tassert(p1.determinant()==253);
  tassert(p11.determinant()==0);
  for (i=0;i<3;i++)
    for (j=0;j<4;j++)
      rs1[i][j]=(j+1.)/(i+1)-(i^j);
  rs2=rs1;
  rs2.resize(4,3);
  rs3=rs1*rs2;
  rs4=rs2*rs1;
  for (i=0;i<3;i++)
    for (j=0;j<3;j++)
      tassert(rs1[i][j]==rs2[i][j]);
  cout<<"det rs3="<<ldecimal(rs3.determinant())<<endl;
  tassert(fabs(rs3.determinant()*9-100)<1e-12);
  tassert(rs4.determinant()==0);
  rs4[3][3]=1;
  tassert(fabs(rs4.determinant()*9-100)<1e-12);
}

void testrelprime()
{
  tassert(relprime(987)==610);
  tassert(relprime(610)==377);
  tassert(relprime(377)==233);
  tassert(relprime(233)==144);
  tassert(relprime(144)==89);
  tassert(relprime(89)==55);
  tassert(relprime(55)==34);
  tassert(relprime(100000)==61803);
  tassert(relprime(100)==61);
  tassert(relprime(0)==1);
  tassert(relprime(1)==1);
  tassert(relprime(2)==1);
  tassert(relprime(3)==2);
  tassert(relprime(4)==3);
  tassert(relprime(5)==3);
  tassert(relprime(6)==5);
}

void testmanysum()
{
  manysum ms,negms;
  int i,j,h;
  double x,naiveforwardsum,forwardsum,pairforwardsum,naivebackwardsum,backwardsum,pairbackwardsum;
  vector<double> summands;
  double odd[32];
  long double oddl[32];
  int pairtime=0;
  //QTime starttime;
  cout<<"manysum"<<endl;
  for (i=0;i<32;i++)
    oddl[i]=odd[i]=2*i+1;
  for (i=0;i<32;i++)
  {
    tassert(pairwisesum(odd,i)==i*i);
    tassert(pairwisesum(oddl,i)==i*i);
  }
  ms.clear();
  summands.clear();
  tassert(pairwisesum(summands)==0);
  for (naiveforwardsum=i=0;i>-7;i--)
  {
    x=pow(1000,i);
    for (j=0;j<(slowmanysum?1000000:100000);j++)
    {
      naiveforwardsum+=x;
      ms+=x;
      negms-=x;
      summands.push_back(x);
    }
  }
  ms.prune();
  forwardsum=ms.total();
  tassert(forwardsum==-negms.total());
  //starttime.start();
  pairforwardsum=pairwisesum(summands);
  //pairtime+=starttime.elapsed();
  ms.clear();
  summands.clear();
  for (naivebackwardsum=0,i=-6;i<1;i++)
  {
    x=pow(1000,i);
    for (j=0;j<(slowmanysum?1000000:100000);j++)
    {
      naivebackwardsum+=x;
      ms+=x;
      summands.push_back(x);
    }
  }
  ms.prune();
  backwardsum=ms.total();
  //starttime.start();
  pairbackwardsum=pairwisesum(summands);
  //pairtime+=starttime.elapsed();
  cout<<"Forward: "<<ldecimal(naiveforwardsum)<<' '<<ldecimal(forwardsum)<<' '<<ldecimal(pairforwardsum)<<endl;
  cout<<"Backward: "<<ldecimal(naivebackwardsum)<<' '<<ldecimal(backwardsum)<<' '<<ldecimal(pairbackwardsum)<<endl;
  tassert(fabs((forwardsum-backwardsum)/(forwardsum+backwardsum))<DBL_EPSILON);
  tassert(fabs((pairforwardsum-pairbackwardsum)/(pairforwardsum+pairbackwardsum))<DBL_EPSILON);
  tassert(fabs((forwardsum-pairforwardsum)/(forwardsum+pairforwardsum))<DBL_EPSILON);
  tassert(fabs((backwardsum-pairbackwardsum)/(backwardsum+pairbackwardsum))<DBL_EPSILON);
  tassert(fabs((forwardsum-naiveforwardsum)/(forwardsum+naiveforwardsum))<1000000*DBL_EPSILON);
  tassert(fabs((backwardsum-naivebackwardsum)/(backwardsum+naivebackwardsum))<1000*DBL_EPSILON);
  tassert(fabs((naiveforwardsum-naivebackwardsum)/(naiveforwardsum+naivebackwardsum))>30*DBL_EPSILON);
  ms.clear();
  summands.clear();
  h=slowmanysum?1:16;
  for (naiveforwardsum=i=0;i>-0x360000;i-=h)
  {
    x=exp(i/65536.);
    naiveforwardsum+=x;
    ms+=x;
    summands.push_back(x);
  }
  ms.prune();
  forwardsum=ms.total();
  //starttime.start();
  pairforwardsum=pairwisesum(summands);
  //pairtime+=starttime.elapsed();
  ms.clear();
  summands.clear();
  for (naivebackwardsum=0,i=-0x35ffff&-h;i<1;i+=h)
  {
    x=exp(i/65536.);
    naivebackwardsum+=x;
    ms+=x;
    summands.push_back(x);
  }
  ms.prune();
  backwardsum=ms.total();
  //starttime.start();
  pairbackwardsum=pairwisesum(summands);
  //pairtime+=starttime.elapsed();
  cout<<"Forward: "<<ldecimal(naiveforwardsum)<<' '<<ldecimal(forwardsum)<<' '<<ldecimal(pairforwardsum)<<endl;
  cout<<"Backward: "<<ldecimal(naivebackwardsum)<<' '<<ldecimal(backwardsum)<<' '<<ldecimal(pairbackwardsum)<<endl;
  tassert(fabs((forwardsum-backwardsum)/(forwardsum+backwardsum))<DBL_EPSILON);
  tassert(fabs((pairforwardsum-pairbackwardsum)/(pairforwardsum+pairbackwardsum))<DBL_EPSILON);
  tassert(fabs((forwardsum-pairforwardsum)/(forwardsum+pairforwardsum))<DBL_EPSILON);
  tassert(fabs((backwardsum-pairbackwardsum)/(backwardsum+pairbackwardsum))<DBL_EPSILON);
  tassert(fabs((forwardsum-naiveforwardsum)/(forwardsum+naiveforwardsum))<1000000*DBL_EPSILON);
  tassert(fabs((backwardsum-naivebackwardsum)/(backwardsum+naivebackwardsum))<1000*DBL_EPSILON);
  tassert(fabs((naiveforwardsum-naivebackwardsum)/(naiveforwardsum+naivebackwardsum))>30*DBL_EPSILON);
  //cout<<"Time in pairwisesum: "<<pairtime<<endl;
}

void testldecimal()
{
  double d;
  bool looptests=false;
  cout<<ldecimal(1/3.)<<endl;
  cout<<ldecimal(M_PI)<<endl;
  cout<<ldecimal(-64664./65536,1./131072)<<endl;
  /* This is a number from the Alaska geoid file, -0.9867, which was output
   * as -.98669 when converting to GSF.
   */
  if (looptests)
  {
    for (d=M_SQRT_3-20*DBL_EPSILON;d<=M_SQRT_3+20*DBL_EPSILON;d+=DBL_EPSILON)
      cout<<ldecimal(d)<<endl;
    for (d=1.25-20*DBL_EPSILON;d<=1.25+20*DBL_EPSILON;d+=DBL_EPSILON)
      cout<<ldecimal(d)<<endl;
    for (d=95367431640625;d>1e-14;d/=5)
      cout<<ldecimal(d)<<endl;
    for (d=123400000000000;d>3e-15;d/=10)
      cout<<ldecimal(d)<<endl;
    for (d=DBL_EPSILON;d<=1;d*=2)
      cout<<ldecimal(M_SQRT_3,d)<<endl;
  }
  cout<<ldecimal(0)<<' '<<ldecimal(INFINITY)<<' '<<ldecimal(NAN)<<' '<<ldecimal(-5.67)<<endl;
  cout<<ldecimal(3628800)<<' '<<ldecimal(1296000)<<' '<<ldecimal(0.000016387064)<<endl;
  tassert(ldecimal(0)=="0");
  tassert(ldecimal(1)=="1");
  tassert(ldecimal(-1)=="-1");
  tassert(isalpha(ldecimal(INFINITY)[0]));
  tassert(isalpha(ldecimal(NAN)[1])); // on MSVC is negative, skip the minus sign
  tassert(ldecimal(1.7320508)=="1.7320508");
  tassert(ldecimal(1.7320508,0.0005)=="1.732");
  tassert(ldecimal(-0.00064516)=="-.00064516");
  tassert(ldecimal(3628800)=="3628800");
  tassert(ldecimal(1296000)=="1296e3");
  tassert(ldecimal(0.000016387064)=="1.6387064e-5");
  tassert(ldecimal(-64664./65536,1./131072)=="-.9867");
}

void testleastsquares()
{
  matrix a(3,2);
  vector<double> b,x;
  b.push_back(4);
  b.push_back(1);
  b.push_back(3);
  a[0][0]=1;
  a[0][1]=3; // http://ltcconline.net/greenl/courses/203/MatrixOnVectors/leastSquares.htm
  a[1][0]=2;
  a[1][1]=4;
  a[2][0]=1;
  a[2][1]=6;
  x=linearLeastSquares(a,b);
  cout<<"Least squares ("<<ldecimal(x[0])<<','<<ldecimal(x[1])<<")\n";
  tassert(dist(xy(x[0],x[1]),xy(-29/77.,51/77.))<1e-9);
  a.resize(2,3);
  b.clear();
  b.push_back(1);
  b.push_back(0);
  a[0][0]=1;
  a[0][1]=1; // https://www.math.usm.edu/lambers/mat419/lecture15.pdf
  a[0][2]=1;
  a[1][0]=-1;
  a[1][1]=-1;
  a[1][2]=1;
  x=minimumNorm(a,b);
  cout<<"Minimum norm ("<<ldecimal(x[0])<<','<<ldecimal(x[1])<<','<<ldecimal(x[2])<<")\n";
  tassert(dist(xyz(x[0],x[1],x[2]),xyz(0.25,0.25,0.5))<1e-9);
}

const int linearSize[]={1,2,3,9,24,65,171,454,1200,3176,8403,22234};
const int quadraticSize[]={0,3,24,171,1200,8403,58824,411771,2882400,20176803,141237624,988663371};

void testflowsnake()
{
  PostScript ps;
  Flowsnake fs;
  double prog;
  Cube cube(xyz(314159,271828,0),512);
  Cylinder cyl;
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
  fs.setSize(cube,10);
  ps.startpage();
  ps.setscale(cube.minX(),cube.minY(),cube.maxX(),cube.maxY());
  while (true)
  {
    prog=fs.progress();
    cyl=fs.cyl(fs.next());
    if (cyl.getRadius()==0)
      break;
    ps.setcolor(prog,prog,1-prog);
    ps.circle(cyl.getCenter(),cyl.getRadius());
  }
  ps.endpage();
  cout<<' ';
}

void testpeano()
{
  unsigned i;
  array<int,3> rec;
  array<int,2> pnt;
  char histo[36];
  Peano pe;
  PostScript ps;
  memset(histo,0,36);
  for (i=0;i<THREE20;i+=rec[2])
  {
    rec=peanoPoint(2,2,i);
    cout<<setw(11)<<i<<' '<<rec[0]<<' '<<rec[1]<<endl;
  }
  for (i=0;i<THREE20;i+=rec[2])
  {
    rec=peanoPoint(5,7,i);
    cout<<setw(11)<<i<<' '<<rec[0]<<' '<<rec[1]<<endl;
    if (rec[0]<0)
      histo[35]++;
    else
      histo[rec[0]*7+rec[1]]++;
  }
  for (i=0;i<35;i++)
    tassert(histo[i]==1);
  tassert(histo[i]==20);
  ps.open("peano.ps");
  ps.setpaper(papersizes["A4 landscape"],0);
  ps.prolog();
  ps.startpage();
  ps.setscale(0,0,71,41);
  ps.startline();
  pe.resize(71,41);
  for (i=0;i<71*41;i++)
  {
    pnt=pe.step();
    ps.lineto(xy(pnt[0],pnt[1]));
  }
  ps.endline();
  ps.endpage();
}

/* Files for testing block splitting:
 * 719: 1, 710, 81
 * 727: 1, 10, 100
 * 5749: 1, 18, 324
 * 5779: 1, 5762, 289
 * The number of points in one octree block is 720, and in eight blocks is 5760.
 * These numbers are the next primes above and below them. The numbers 710, 10,
 * 18, and 5762 are primitive roots of the respective primes close to their
 * positive or negative cube roots, for even distribution of points in the cube.
 * The test showed that points were being lost when splitting a block.
 */
void test1splitfile(int p,int proot)
{
  int i;
  int zstep=(proot*proot)%p;
  LasHeader lasHeader;
  LasPoint pnt;
  lasHeader.openWrite(to_string(p)+".las",SI_TEST);
  lasHeader.setVersion(1,2);
  lasHeader.setPointFormat(1);
  lasHeader.setScale(xyz(0,0,0),xyz(1,1,1),xyz(1./p,1./p,1./p));
  for (i=0;i<p;i++)
  {
    pnt.location=xyz((i+0.5)/p,((i*proot)%p+0.5)/p,((i*zstep)%p+0.5)/p);
    pnt.returnNum=1;
    pnt.gpsTime=i;
    lasHeader.writePoint(pnt);
  }
  lasHeader.writeHeader();
}

void testsplitfile()
/* The two numbers are a prime near 1 or 8 times the number of records per
 * block (see octree.h) and a primitive root near ± its cube root.
 */
{
  test1splitfile(523,516);
  test1splitfile(541,10);
  test1splitfile(719,710);
  test1splitfile(727,10);
  test1splitfile(4289,17);
  test1splitfile(4297,15);
  test1splitfile(5749,18);
  test1splitfile(5779,5762);
}

void testbigcloud()
/* Creates a big point cloud in seven files to test loading of big point clouds.
 * The density is set so that, except at the edges, each cube containing
 * points is a 1 meter cube.
 * The total number of points is 7 million. This normally fits in RAM, but
 * the program can be forced to swap by setting low RAM to all RAM.
 */
{
  vector<long long> blocks;
  vector<LasPoint> points;
  int i,j;
#ifdef WAVEFORM
  const double density=700; //  720 points per block
#else
  const double density=500; // 537 points per block
#endif
  map<int,LasHeader> headers;
  const int totalPoints=7000000;
  double radius=sqrt(totalPoints/M_PI/density);
  setStorePoints(false);
  wavyScene(radius,density,0.33,0.30,0.1);
  for (i=0;i<7;i++)
  {
    headers[i].openWrite("big"+to_string(i)+".las",SI_TEST);
    headers[i].setVersion(1,4);
    headers[i].setPointFormat(6);
    headers[i].setScale(xyz(-radius,-radius,0.03),xyz(radius,radius,0.63),xyz(0,0,0));
    for (j=0;j<totalPoints/7;j++)
      headers[i].writePoint(testCloud[i*(totalPoints/7)+j]);
    headers[i].writeHeader();
    headers[i].close();
#ifdef LASzip_FOUND
    laszipCompex("big"+to_string(i)+".las","big"+to_string(i)+".laz",true);
#endif
  }
}

void testwkt()
{
  char wvawkt[]= // Well-known text from square of West Virginia terrain
  "COMPD_CS[\"NAD83 / UTM zone 17N + NAVD88 height\",PROJCS[\"NAD83 / UTM zone 17N\","
  "GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\","
  "6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],TOWGS84[0,0,0,0,0,0,0],"
  "AUTHORITY[\"EPSG\",\"6269\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],"
  "UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY"
  "[\"EPSG\",\"4269\"]],PROJECTION[\"Transverse_Mercator\"],PARAMETER"
  "[\"latitude_of_origin\",0],PARAMETER[\"central_meridian\",-81],PARAMETER"
  "[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER"
  "[\"false_northing\",0],UNIT[\"meter\",1,AUTHORITY[\"EPSG\",\"9001\"]],"
  "AXIS[\"X\",EAST],AXIS[\"Y\",NORTH],AUTHORITY[\"EPSG\",\"26917\"]],VERT_CS"
  "[\"NAVD88 height\",VERT_DATUM[\"North American Vertical Datum 1988\",2005,"
  "AUTHORITY[\"EPSG\",\"5103\"]],UNIT[\"meter\",1,AUTHORITY[\"EPSG\",\"9001\"]],"
  "AXIS[\"Up\",UP],AUTHORITY[\"EPSG\",\"5703\"]]]";
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
  initPhases();
  fillTanTables();
  startThreads(1); // so that lockCube doesn't divide by 0
  waitForThreads(TH_STOP);
  joinThreads();
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
  if (shoulddo("sphere"))
    testsphere();
  if (shoulddo("hyperboloid"))
    testhyperboloid();
  if (shoulddo("cylinder"))
    testcylinder();
  if (shoulddo("flat"))
    testflat();
  if (shoulddo("matrix"))
    testmatrix();
  if (shoulddo("relprime"))
    testrelprime();
  if (shoulddo("manysum"))
    testmanysum(); // >2 s
  if (shoulddo("ldecimal"))
    testldecimal();
  if (shoulddo("integertrig"))
    testintegertrig();
  if (shoulddo("leastsquares"))
    testleastsquares();
  if (shoulddo("flowsnake"))
    testflowsnake();
  if (shoulddo("peano"))
    testpeano();
  if (shoulddo("splitfile"))
    testsplitfile();
  if (shoulddo("bigcloud"))
    testbigcloud();
  if (shoulddo("wkt"))
    testwkt();
  cout<<"\nTest "<<(testfail?"failed":"passed")<<endl;
  return testfail;
}
