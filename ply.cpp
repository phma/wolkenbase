/******************************************************/
/*                                                    */
/* ply.cpp - polygon files                            */
/*                                                    */
/******************************************************/
/* Copyright 2019-2021 Pierre Abbat.
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

#include "config.h"
#undef VERSION
#undef COPY_YEAR
#ifdef Plytapus_FOUND
#include <plytapus.h>
#endif
#include "ply.h"
#include "cloud.h"

using namespace std;
#ifdef Plytapus_FOUND
using namespace plytapus;

double plyUnit;
xyz plyOffset;

string plytapusVersion()
{
  return version();
}

int plytapusYear()
{
  return copyrightYear();
}

void receivePoint(ElementBuffer &buf)
{
  if (buf.size()>=3)
  {
    xyz pnt(buf[0],buf[1],buf[2]);
    cloud.push_back(pnt);
  }
}

void readPly(string fileName)
{
  try
  {
    File plyfile(fileName);
    ElementReadCallback pointCallback=receivePoint;
    plyfile.setElementReadCallback("vertex",pointCallback);
    plyfile.read();
  }
  catch (...)
  {
  }
}
#else

string plytapusVersion()
{
  return "";
}

void readPly(string fileName)
{
}
#endif
