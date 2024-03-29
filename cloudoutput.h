/******************************************************/
/*                                                    */
/* cloudoutput.h - write LAS files after classifying  */
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
#ifndef CLOUDOUTPUT_H
#define CLOUDOUTPUT_H

#include <QObject>
#include <map>
#include <deque>
#include "las.h"

class CloudOutput: public QObject
{
  Q_OBJECT // Needs to be a QObject because it has translated strings.
public:
  xyz minCor,maxCor,scale;
  int nInputFiles;
  size_t grandTotal;
  int pointsPerFile; // 0 means no limit
  int pointFormat;
  bool separateClasses,writeLaz;
  double unit;
  std::string className(int n);
  void openFiles(std::string name,std::map<int,size_t> classTotals);
  void writeFiles();
  void closeFiles();
private:
  std::map<int,std::deque<LasHeader> > headers;
};

#endif
