/******************************************************/
/*                                                    */
/* eisenstein.h - Eisenstein integers                 */
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
/* Hexagonal vector (Eisenstein or Euler integers) and array of bytes subscripted by hexagonal vector
 * The largest Eisenstein used in this program has a norm of about
 * TBD
 */
#ifndef EISENSTEIN_H
#define EISENSTEIN_H

#include <cmath>
#include <cstdlib>
#include <map>
#include <complex>
#define M_SQRT_3_4 0.86602540378443864676372317
// The continued fraction expansion is 0;1,6,2,6,2,6,2,...
#define M_SQRT_3 1.73205080756887729352744634
#define M_SQRT_1_3 0.5773502691896257645091487805

//PAGERAD should be 1 or 6 mod 8, which makes PAGESIZE 7 mod 8.
// The maximum is 147 because of the file format.
#define PAGERAD 6
#define PAGESIZE (PAGERAD*(PAGERAD+1)*3+1)
extern const std::complex<double> omega,ZLETTERMOD;

inline int sqr(int a)
{
  return a*a;
}

class Eisenstein
{
private:
  int x,y; // x is the real part, y is at 120°
  static int numx,numy,denx,deny,quox,quoy,remx,remy;
  void divmod(Eisenstein b);
public:
  Eisenstein()
  {
    x=y=0;
  }
  Eisenstein(int xa)
  {
    x=xa;
    y=0;
  }
  Eisenstein(int xa,int ya)
  {
    x=xa;
    y=ya;
  }
  Eisenstein(std::complex<double> z);
  Eisenstein operator+(Eisenstein b);
  Eisenstein operator-();
  Eisenstein operator-(Eisenstein b);
  Eisenstein operator*(const Eisenstein b) const;
  Eisenstein& operator*=(Eisenstein b);
  Eisenstein& operator+=(Eisenstein b);
  Eisenstein operator/(Eisenstein b);
  Eisenstein operator%(Eisenstein b);
  bool operator==(Eisenstein b);
  bool operator!=(Eisenstein b);
  friend bool operator<(const Eisenstein a,const Eisenstein b); // only for the map
  unsigned long norm();
  int pageinx(int size,int nelts);
  int pageinx();
  int letterinx();
  int getx()
  {
    return x;
  }
  int gety()
  {
    return y;
  }
  operator std::complex<double>() const
  {
    return std::complex<double>(x-y/2.,y*M_SQRT_3_4);
  }
  void inc(int n);
  bool cont(int n);
};

Eisenstein start(int n);
Eisenstein nthEisenstein(int n,int size,int nelts);
extern const Eisenstein LETTERMOD,PAGEMOD;
extern const Eisenstein root1[6];
extern int debugEisenstein;

template <typename T> class harray
{
  std::map<Eisenstein,T *> index;
public:
  T& operator[](Eisenstein i);
  int count(Eisenstein i);
  void clear();
};

template <typename T> T& harray<T>::operator[](Eisenstein i)
{
  Eisenstein q,r;
  q=i/PAGEMOD;
  r=i%PAGEMOD;
  if (!index[q])
    index[q]=(T*)calloc(PAGESIZE,sizeof(T));
  return index[q][r.pageinx()];
}

template <typename T> int harray<T>::count(Eisenstein i)
// Returns 1 if the page is allocated, even if the item hasn't been written to.
{
  Eisenstein q;
  q=i/PAGEMOD;
  return index[q]!=nullptr;
}

template <typename T> void harray<T>::clear()
{
  typename std::map<Eisenstein,T *>::iterator i;
  for (i=index.start();i!=index.end();i++)
  {
    free(i->second);
    i->second=NULL;
  }
}

int region(std::complex<double> z);
void testcomplex();
void testpageinx();

/* Files for storing hexagonal arrays:
 * Byte 0: number of bits per element
 * Byte 1: PAGERAD
 * Bytes 2 and 3: PAGESIZE, native endianness
 * Bytes 4 and 5 additional information about an element format, currently 0000
 * This is followed by strips:
 * Bytes 0 and 1: x-coordinate of start of strip divided by PAGEMOD
 * Bytes 2 and 3: y-coordinate of start of strip divided by PAGEMOD
 * Bytes 4 and 5: number of pages in strip
 * Example (little-endian):
 * 01 06 7f 00 00 00 fa ff fa ff 07 00 <7×16 bytes of data>
 * Numbers of bits are expected to be 1, 2 (for art masks), 8, and 16. If 16, the data
 * will be stored in two-byte words, with the same endianness as PAGESIZE.
 * At first the program will read only files with its native endianness and PAGERAD;
 * later it will be able to convert them.
 */

extern harray<char> hletters,hbits;
#endif
