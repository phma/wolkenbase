/******************************************************/
/*                                                    */
/* cloudoutput.cpp - write LAS files after classifying*/
/*                                                    */
/******************************************************/
/* Copyright 2021 Pierre Abbat.
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
#include "cloudoutput.h"
using namespace std;

string CloudOutput::className(int n)
// The output should have no spaces. It will be part of a file name.
{
  QString qret;
  switch (n)
  {
    case 0:
      qret=tr("raw");
      break;
    case 1:
      qret=tr("nonground");
      break;
    case 2:
      qret=tr("ground");
      break;
    case 7:
      qret=tr("lownoise");
      break;
  }
  return qret.toStdString();
}

void CloudOutput::openFiles(string name,map<int,size_t> classTotals)
{
}
