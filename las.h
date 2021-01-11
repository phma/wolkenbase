/******************************************************/
/*                                                    */
/* las.h - laser point cloud files                    */
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

#ifndef LAS_H
#define LAS_H
#include <string>
#include <iostream>
#include "config.h"
#include "point.h"

/* These tell what to put in the system identifier field of the header.
 * Merge	More than one input, one output
 * Modify	One input, one output, classify points
 * Extract	At least one input, more than one output
 * Test		No input, at least one output (test file generated by Wolkenbase)
 */
#define SI_MERGE 0
#define SI_MODIFY 1
#define SI_EXTRACT 2
#define SI_TEST 3

class OctBlock;

int joinPointFormat(std::vector<int> formats);

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
#ifdef WAVEFORM
  unsigned short waveIndex;
#endif
  unsigned short pointSource;
  int scanAngle;
  double gpsTime;
  unsigned short nir,red,green,blue;
#ifdef WAVEFORM
  size_t waveformOffset;
  unsigned int waveformSize;
  float waveformTime,xDir,yDir,zDir;
#endif
  LasPoint();
  bool isEmpty();
//private:
  void read(std::istream &file);
  void write(std::ostream &file) const;
  friend class OctBlock;
};

extern const LasPoint noPoint;

class VariableLengthRecord
// Used for extended variable length records as well.
{
private:
  unsigned short reserved;
  std::string userId;
  unsigned short recordId;
  std::string description;
  std::string data;
public:
  VariableLengthRecord();
  void setUserId(std::string uid);
  void setRecordId(int rid);
  void setDescription(std::string desc);
  void setData(std::string dat);
  std::string getUserId();
  int getRecordId();
  std::string getDescription();
  std::string getData();
  friend class LasHeader;
};

class LasHeader
{
private:
  std::fstream *lasfile;
  std::string filename;
  unsigned short sourceId,globalEncoding;
  unsigned int guid1;
  unsigned short guid2,guid3;
  char guid4[8];
  char versionMajor,versionMinor;
  std::string systemId,softwareName;
  unsigned short creationDay,creationYear;
  unsigned int headerSize;
  unsigned int pointOffset;
  unsigned int nVariableLength;
  unsigned short pointFormat,pointLength;
  double xScale,yScale,zScale,xOffset,yOffset,zOffset;
  double maxX,minX,maxY,minY,maxZ,minZ;
  double unit;
  size_t startWaveform,startExtendedVariableLength;
  unsigned int nExtendedVariableLength;
  size_t nPoints[16]; // [0] is total; [1]..[15] are each return
  size_t nReadPoints; // for progress bar
  bool reading;
  size_t writePos;
public:
  LasHeader();
  ~LasHeader();
  void openRead(std::string fileName);
  void openWrite(std::string fileName,int sysId);
  void writeHeader();
  bool isValid();
  void setVersion(int major,int minor);
  void setPointFormat(int format);
  void setScale(xyz minCor,xyz maxCor,xyz scale);
  xyz getScale()
  {
    return xyz(xScale*unit,yScale*unit,zScale*unit);
  }
  xyz getOffset()
  {
    return xyz(xOffset*unit,yOffset*unit,zOffset*unit);
  }
  std::string getFileName()
  {
    return filename;
  }
  void setUnit(double u)
  {
    unit=u;
  }
  double getUnit()
  {
    return unit;
  }
  void close();
  size_t numberPoints(int r=0);
  void clearNumberReadPoints()
  {
    nReadPoints=0;
  }
  size_t numberReadPoints()
  {
    return nReadPoints;
  }
  xyz minCorner();
  xyz maxCorner();
  bool inBox(xyz pnt);
  int getVersion();
  int getPointFormat();
  LasPoint readPoint(size_t num);
  void writePoint(const LasPoint &pnt);
};

#endif
