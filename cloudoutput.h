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

#include <QObject>
#include <map>
#include "las.h"

class CloudOutput: public QObject
{
  Q_OBJECT
public:
  xyz minCor,maxCor,scale;
  int nInputFiles;
  int pointsPerFile; // 0 means no limit
  bool separateClasses;
  std::string className(int n);
  void writeFiles(std::string name);
private:
  std::map<unsigned int,LasHeader> headers;
};
