/******************************************************/
/*                                                    */
/* las.h - laser point cloud files                    */
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

#ifndef LAS_H
#define LAS_H
#include <string>
#include <iostream>
#include "point.h"

class OctBlock;

class LasPoint
{ // 91 bytes in temporary file, 720 points per block
public:
  xyz location;
  unsigned short intensity;
  unsigned short returnNum,nReturns;
  bool scanDirection,edgeLine;
  unsigned short classification,classificationFlags;
  unsigned short scannerChannel;
  unsigned short userData;
  unsigned short waveIndex;
  unsigned short pointSource;
  int scanAngle;
  double gpsTime;
  unsigned short nir,red,green,blue;
  size_t waveformOffset;
  unsigned int waveformSize;
  float waveformTime,xDir,yDir,zDir;
  LasPoint();
  LasPoint(OctBlock *parent);
  bool isEmpty();
//private:
  void read(std::istream &file);
  void write(std::ostream &file);
  OctBlock *block;
  friend class OctBlock;
};

class LasHeader
{
private:
  std::ifstream *lasfile;
  unsigned short sourceId,globalEncoding;
  unsigned int guid1;
  unsigned short guid2,guid3;
  char guid4[8];
  char versionMajor,versionMinor;
  std::string systemId,softwareName;
  unsigned short creationDay,creationYear;
  unsigned int pointOffset;
  unsigned int nVariableLength;
  unsigned short pointFormat,pointLength;
  double xScale,yScale,zScale,xOffset,yOffset,zOffset;
  double maxX,minX,maxY,minY,maxZ,minZ;
  size_t startWaveform,startExtendedVariableLength;
  unsigned int nExtendedVariableLength;
  size_t nPoints[16]; // [0] is total; [1]..[15] are each return
public:
  LasHeader();
  ~LasHeader();
  void open(std::string fileName);
  bool isValid();
  void close();
  size_t numberPoints(int r=0);
  xyz minCorner();
  xyz maxCorner();
  LasPoint readPoint(size_t num);
};

void readLas(std::string fileName);
#endif
