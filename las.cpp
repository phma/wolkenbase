/******************************************************/
/*                                                    */
/* las.cpp - laser point cloud files                  */
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

#include <cassert>
#include <cstring>
#include <ctime>
#include "config.h"
#include "las.h"
#include "binio.h"
#include "angle.h"

const int MASK_GPSTIME=0x7fa; // GPS time takes 8 bytes
const int MASK_RGB=    0x5ac; // RGB takes 6 bytes
const int MASK_NIR=    0x500; // NIR takes 2 bytes
const int MASK_WAVE=   0x630; // Wave data take 29 bytes
const short pointLengths[]={20,28,26,34,57,63,30,36,38,59,67};
const short pointFeatures[]={0x0,0x1,0x2,0x3,0x9,0xb,0x101,0x103,0x107,0x109,0x10f};

using namespace std;

#ifdef LASzip_FOUND
#include <laszip_api.h>

void laszipInit()
{
  int rc;
  rc=laszip_load_dll();
}

void laszipError(laszip_POINTER a)
{
  if (a)
  {
    laszip_CHAR *error;
    if (laszip_get_error(a,&error))
      cout<<"LASzip error: "<<error<<endl;
    else
      cout<<"unknown LASzip error\n";
  }
  else
    cout<<"null LASzip pointer\n";
}

void laszipCompex(string inputFileName,string outputFileName,bool compress)
{
  laszip_POINTER reader,writer;
  laszip_BOOL readCompress=0;
  laszip_header *header;
  laszip_I64 npoints,i;
  int rc;
  laszip_point* point;
  rc=laszip_create(&reader);
  rc=laszip_open_reader(reader,inputFileName.c_str(),&readCompress);
  if (rc)
    laszipError(reader);
  rc=laszip_get_header_pointer(reader,&header);
  npoints=(header->number_of_point_records ? header->number_of_point_records : header->extended_number_of_point_records);
  rc=laszip_get_point_pointer(reader,&point);
  rc=laszip_create(&writer);
  rc=laszip_set_header(writer,header);
  rc=laszip_preserve_generating_software(writer,true);
  rc=laszip_open_writer(writer,outputFileName.c_str(),compress);
  for (i=0;i<npoints;i++)
  {
    laszip_read_point(reader);
    laszip_set_point(writer,point);
    laszip_write_point(writer);
  }
  laszip_close_writer(writer);
  laszip_destroy(writer);
  laszip_close_reader(reader);
  laszip_destroy(reader);
}

#endif

int joinPointFormat(vector<int> formats)
/* Computes the join (supremum) of the formats, according to their features.
 * See https://en.wikipedia.org/wiki/Lattice_(order) .
 */
{
  int i,allFeatures=0,ret;
  for (i=0;i<formats.size();i++)
    if (formats[i]>=0 && formats[i]<sizeof(pointFeatures)/sizeof(pointFeatures[0]))
      allFeatures|=pointFeatures[formats[i]];
  for (i=0;i<sizeof(pointFeatures)/sizeof(pointFeatures[0]);i++)
    if ((allFeatures|pointFeatures[i])==pointFeatures[i])
    {
      ret=i;
      break;
    }
  return ret;
}

LasPoint::LasPoint()
{
  location=nanxyz;
  intensity=returnNum=nReturns=classification=classificationFlags=0;
  scannerChannel=userData=pointSource=nir=red=green=blue=0;
  scanDirection=edgeLine=false;
  scanAngle=0;
  gpsTime=0;
#ifdef WAVEFORM
  waveformTime=xDir=yDir=zDir=waveformSize=waveformOffset=waveIndex=0;
#endif
}

bool LasPoint::isEmpty()
{
  return location.isnan();
}

void LasPoint::read(istream &file)
// Reads from the temporary file. To read from a LAS file, see LasHeader::readPoint.
{
  double x,y,z;
  int flags;
  x=readledouble(file);
  y=readledouble(file);
  z=readledouble(file);
  location=xyz(x,y,z);
  intensity=readleshort(file);
  returnNum=readleshort(file);
  nReturns=readleshort(file);
  flags=file.get();
  scanDirection=flags&1;
  edgeLine=(flags>>1)&1;
  classification=readleshort(file);
  classificationFlags=readleshort(file);
  scannerChannel=readleshort(file);
  userData=readleshort(file);
#ifdef WAVEFORM
  waveIndex=readleshort(file);
#endif
  pointSource=readleshort(file);
  scanAngle=readleint(file);
  gpsTime=readledouble(file);
  nir=readleshort(file);
  red=readleshort(file);
  green=readleshort(file);
  blue=readleshort(file);
#ifdef WAVEFORM
  waveformOffset=readlelong(file);
  waveformSize=readleint(file);
  waveformTime=readlefloat(file);
  xDir=readlefloat(file);
  yDir=readlefloat(file);
  zDir=readlefloat(file);
#endif
}

void LasPoint::write(ostream &file) const
{
  writeledouble(file,location.getx());
  writeledouble(file,location.gety());
  writeledouble(file,location.getz());
  writeleshort(file,intensity);
  writeleshort(file,returnNum);
  writeleshort(file,nReturns);
  file.put(edgeLine*2+scanDirection);
  writeleshort(file,classification);
  writeleshort(file,classificationFlags);
  writeleshort(file,scannerChannel);
  writeleshort(file,userData);
#ifdef WAVEFORM
  writeleshort(file,waveIndex);
#endif
  writeleshort(file,pointSource);
  writeleint(file,scanAngle);
  writeledouble(file,gpsTime);
  writeleshort(file,nir);
  writeleshort(file,red);
  writeleshort(file,green);
  writeleshort(file,blue);
#ifdef WAVEFORM
  writelelong(file,waveformOffset);
  writeleint(file,waveformSize);
  writelefloat(file,waveformTime);
  writelefloat(file,xDir);
  writelefloat(file,yDir);
  writelefloat(file,zDir);
#endif
}

const LasPoint noPoint;

string read16(istream &file)
{
  char buf[24];
  memset(buf,0,24);
  file.read(buf,16);
  return string(buf);
}

string read32(istream &file)
{
  char buf[40];
  memset(buf,0,40);
  file.read(buf,32);
  return string(buf);
}

void write32(ostream &file,string str)
{
  str.resize(32);
  file<<str;
}

VariableLengthRecord::VariableLengthRecord()
{
  reserved=recordId=0;
}

void VariableLengthRecord::setUserId(string uid)
{
  userId=uid;
}

void VariableLengthRecord::setRecordId(int rid)
{
  recordId=rid;
}

void VariableLengthRecord::setDescription(string desc)
{
  description=desc;
}

void VariableLengthRecord::setData(string dat)
{
  data=dat;
}

string VariableLengthRecord::getUserId()
{
  return userId;
}

int VariableLengthRecord::getRecordId()
{
  return recordId;
}

string VariableLengthRecord::getDescription()
{
  return description;
}

string VariableLengthRecord::getData()
{
  return data;
}

LasHeader::LasHeader()
{
  lasfile=nullptr;
  versionMajor=versionMinor=0;
  unit=1;
}

LasHeader::~LasHeader()
{
  close();
}

void LasHeader::openRead(string fileName)
{
  int magicBytes;
  int64_t lazMagic;
  int i;
  size_t total;
  unsigned int legacyNPoints[6];
  int whichNPoints=15;
  if (lasfile)
    close();
  filename=fileName;
  lasfile=new fstream(fileName,ios::binary|ios::in);
  reading=true;
  nReadPoints=0;
  magicBytes=readbeint(*lasfile);
  if (magicBytes==0x4c415346)
  {
    sourceId=readleshort(*lasfile);
    globalEncoding=readleshort(*lasfile);
    guid1=readleint(*lasfile);
    guid2=readleshort(*lasfile);
    guid3=readleshort(*lasfile);
    lasfile->read(guid4,8);
    versionMajor=lasfile->get();
    versionMinor=lasfile->get();
    systemId=read32(*lasfile);
    softwareName=read32(*lasfile);
    creationDay=readleshort(*lasfile);
    creationYear=readleshort(*lasfile);
    headerSize=readleshort(*lasfile);
    pointOffset=readleint(*lasfile);
    nVariableLength=readleint(*lasfile);
    pointFormat=lasfile->get();
    pointLength=readleshort(*lasfile);
    for (i=0;i<6;i++)
      legacyNPoints[i]=readleint(*lasfile);
    xScale=readledouble(*lasfile);
    yScale=readledouble(*lasfile);
    zScale=readledouble(*lasfile);
    xOffset=readledouble(*lasfile);
    yOffset=readledouble(*lasfile);
    zOffset=readledouble(*lasfile);
    maxX=readledouble(*lasfile);
    minX=readledouble(*lasfile);
    maxY=readledouble(*lasfile);
    minY=readledouble(*lasfile);
    maxZ=readledouble(*lasfile);
    minZ=readledouble(*lasfile);
    // Here ends the LAS 1.2 header (length 0xe3). The LAS 1.4 header (length 0x177) continues.
    if (headerSize>0xe3)
    {
      startWaveform=readlelong(*lasfile);
      startExtendedVariableLength=readlelong(*lasfile);
      nExtendedVariableLength=readleint(*lasfile);
      for (i=0;i<16;i++)
	nPoints[i]=readlelong(*lasfile);
    }
    else
    {
      startWaveform=startExtendedVariableLength=nExtendedVariableLength=0;
      for (i=0;i<16;i++)
	nPoints[i]=0;
    }
    /* Check the numbers of points by return. They should add up to the total:
     * 986 377 233 144 89 55 34 21 13 8 5 3 2 1 1 0
     * In some files, the total (nPoints[0], legacyNPoints[0]) is nonzero,
     * but the numbers of points by return are all 0. This is invalid, but
     * seen in the wild in files produced by photogrammetry.
     */
    for (i=1,total=0;i<6;i++)
    {
      total+=legacyNPoints[i];
      if (legacyNPoints[i]>legacyNPoints[0])
	whichNPoints&=~5;
    }
    if (total!=legacyNPoints[0])
      whichNPoints&=~1;
    if (total!=0 || legacyNPoints[0]==0)
      whichNPoints&=~4;
    for (i=1,total=0;i<16;i++)
    {
      total+=nPoints[i];
      if (nPoints[i]>nPoints[0])
	whichNPoints&=~10;
    }
    if (total!=nPoints[0])
      whichNPoints&=~2;
    if (total!=0 || nPoints[0]==0)
      whichNPoints&=~8;
    for (i=0;i<6;i++)
    {
      if (legacyNPoints[i]!=nPoints[i] && legacyNPoints[i]!=0)
	whichNPoints&=~10;
      if (nPoints[i]!=legacyNPoints[i] && nPoints[i]!=0)
	whichNPoints&=~5;
    }
    if (whichNPoints>0 && (whichNPoints&3)==0)
    {
      cerr<<"Number of points by return are all 0. Setting first return to all points.\n";
      nPoints[1]=nPoints[0];
      legacyNPoints[1]=legacyNPoints[0];
    }
    if (whichNPoints==1 || whichNPoints==4)
      for (i=0;i<6;i++)
	nPoints[i]=legacyNPoints[i];
    if (whichNPoints==0)
      for (i=0;i<6;i++)
	nPoints[i]=0;
    if (pointLength==0)
      versionMajor=versionMinor=nPoints[0]=0;
    /* Recognize a LASzip file. There's an extra header between the header
     * (headerSize=375, in format 1.4) and the start of points (pointOffset=469),
     * starting "\0\0laszip". If so, the file must be decompressed and the
     * decompressed file read and deleted.
     */
    zipFlag=false;
    if (pointOffset>=headerSize+8)
    {
      lasfile->seekg(headerSize,ios_base::beg);
      lazMagic=readbelong(*lasfile);
      if ((lazMagic&0xffffffffffff)==0x6c61737a6970)
      {
	zipFlag=true;
	cout<<filename<<" is laszipped\n";
      }
    }
  }
  else // file does not begin with "LASF"
    versionMajor=versionMinor=nPoints[0]=0;
}

void LasHeader::openFake(string fileName)
/* The file is an XYZ or PLY file being converted to LAS. LASify needs an LAS
 * header to show the bounding box on the screen.
 */
{
  int i;
  time_t now=time(nullptr);
  tm *ptm;
  if (lasfile)
    close();
  filename=fileName;
  reading=false;
  ptm=gmtime(&now);
  nReadPoints=0;
  versionMajor=1;
  versionMinor=4;
  xScale=yScale=zScale=0;
  xOffset=yOffset=zOffset=NAN;
  setPointFormat(6); // could be 0 if <4G points
  maxX=maxY=maxZ=-INFINITY;
  minX=minY=minZ=INFINITY;
  nVariableLength=nExtendedVariableLength=0;
  for (i=0;i<16;i++)
    nPoints[i]=0;
}

void LasHeader::openWrite(string fileName,int sysId)
{
  int i;
  time_t now=time(nullptr);
  tm *ptm;
  if (lasfile)
    close();
  filename=fileName;
  lasfile=new fstream(fileName,ios::binary|ios::out);
  reading=false;
  switch (sysId)
  {
    case SI_MERGE:
      systemId="MERGE";
      break;
    case SI_MODIFY:
      systemId="MODIFICATION";
      break;
    case SI_EXTRACT:
      systemId="EXTRACTION";
      break;
    case SI_TEST:
      systemId="TEST";
      break;
    default:
      systemId="OTHER";
  }
  ptm=gmtime(&now);
  creationDay=ptm->tm_yday+1; // tm Jan 1 is 0, LAS Jan 1 is 1
  creationYear=ptm->tm_year+1900;
  softwareName="Wolkenbase ";
  softwareName+=VERSION;
  nReadPoints=0;
  versionMajor=1;
  versionMinor=4;
  xScale=yScale=zScale=0;
  xOffset=yOffset=zOffset=NAN;
  pointFormat=pointLength=0;
  maxX=maxY=maxZ=-INFINITY;
  minX=minY=minZ=INFINITY;
  nVariableLength=nExtendedVariableLength=0;
  for (i=0;i<16;i++)
    nPoints[i]=0;
}

void LasHeader::writeHeader()
{
  int i;
  bool legacyValid=true;
  unsigned int legacyNPoints[6];
  for (i=0;i<6;i++)
    if (nPoints[i]>4294967295)
      legacyValid=false;
  for (;i<16;i++)
    if (nPoints[i]>0)
      legacyValid=false;
  for (i=0;i<6;i++)
    legacyNPoints[i]=legacyValid*nPoints[i];
  lasfile->seekp(0,ios_base::beg);
  writebeint(*lasfile,0x4c415346);
  writeleshort(*lasfile,sourceId);
  writeleshort(*lasfile,globalEncoding);
  writeleint(*lasfile,guid1);
  writeleshort(*lasfile,guid2);
  writeleshort(*lasfile,guid3);
  lasfile->write(guid4,8);
  lasfile->put(versionMajor);
  lasfile->put(versionMinor);
  write32(*lasfile,systemId);
  write32(*lasfile,softwareName);
  writeleshort(*lasfile,creationDay);
  writeleshort(*lasfile,creationYear);
  writeleshort(*lasfile,headerSize);
  writeleint(*lasfile,pointOffset);
  writeleint(*lasfile,nVariableLength);
  lasfile->put(pointFormat);
  writeleshort(*lasfile,pointLength);
  for (i=0;i<6;i++)
    writeleint(*lasfile,legacyNPoints[i]);
  writeledouble(*lasfile,xScale);
  writeledouble(*lasfile,yScale);
  writeledouble(*lasfile,zScale);
  writeledouble(*lasfile,xOffset);
  writeledouble(*lasfile,yOffset);
  writeledouble(*lasfile,zOffset);
  writeledouble(*lasfile,maxX);
  writeledouble(*lasfile,minX);
  writeledouble(*lasfile,maxY);
  writeledouble(*lasfile,minY);
  writeledouble(*lasfile,maxZ);
  writeledouble(*lasfile,minZ);
  // Here ends the LAS 1.2 header (length 0xe3). The LAS 1.4 header (length 0x177) continues.
  if (headerSize>0xe3)
  {
    writelelong(*lasfile,startWaveform);
    writelelong(*lasfile,startExtendedVariableLength);
    writeleint(*lasfile,nExtendedVariableLength);
    for (i=0;i<16;i++)
      writelelong(*lasfile,nPoints[i]);
  }
}

bool LasHeader::isValid()
{
  return versionMajor>0 && versionMinor>0 && headerSize>0 && pointLength>0 &&
	 (headerSize>0xe3 || pointFormat<6);
}

void LasHeader::setVersion(int major,int minor)
{
  versionMajor=major;
  versionMinor=minor;
  if (major==1)
    if (minor<4)
      headerSize=0xe3;
    else
      headerSize=0x177;
  else
    headerSize=0;
  writePos=pointOffset=headerSize;
}

void LasHeader::setPointFormat(int format)
{
  pointFormat=format;
  if (format>=0 && format<=sizeof(pointLengths)/sizeof(short))
    pointLength=pointLengths[format];
  else
    pointLength=0;
}

void LasHeader::setScale(xyz minCor,xyz maxCor,xyz scale)
/* If scale is (0,0,0), sets (xScale,yScale,zScale) so that any point in the box
 * can be expressed with a four-byte integer. If scale components are nonzero,
 * sets (xScale,yScale,zScale) to scale. Sets (xOffset,yOffset,zOffset) to
 * the middle of the box. Does not set (minX,minY,minZ) or (maxX,maxY,maxZ);
 * those are computed as the points are written.
 * minCor, maxCor, and scale are in meters. Set unit before setting scale.
 */
{
  xOffset=(minCor.getx()+maxCor.getx())/2/unit;
  if (scale.getx()!=0 && isfinite(scale.getx()))
    xScale=scale.getx()/unit;
  else
    xScale=(maxCor.getx()-minCor.getx())/4132485216./unit;
  yOffset=(minCor.gety()+maxCor.gety())/2/unit;
  if (scale.gety()!=0 && isfinite(scale.gety()))
    yScale=scale.gety()/unit;
  else
    yScale=(maxCor.gety()-minCor.gety())/4132485216./unit;
  zOffset=(minCor.getz()+maxCor.getz())/2/unit;
  if (scale.getz()!=0 && isfinite(scale.getz()))
    zScale=scale.getz()/unit;
  else
    zScale=(maxCor.getz()-minCor.getz())/4132485216./unit;
}

void LasHeader::setMinMax(xyz minCor,xyz maxCor)
{
  minX=minCor.getx();
  minY=minCor.gety();
  minZ=minCor.getz();
  maxX=maxCor.getx();
  maxY=maxCor.gety();
  maxZ=maxCor.getz();
}

void LasHeader::close()
{
  delete(lasfile);
  lasfile=nullptr;
}

size_t LasHeader::numberPoints(int r)
{
  return nPoints[r];
}

xyz LasHeader::minCorner()
{
  return xyz(minX*unit,minY*unit,minZ*unit);
}

xyz LasHeader::maxCorner()
{
  return xyz(maxX*unit,maxY*unit,maxZ*unit);
}

bool LasHeader::inBox(xyz pnt)
{
  xyz min=minCorner();
  xyz max=maxCorner();
  if (std::isnan(pnt.getz())) // Hit testing uses only xy
    pnt=xyz(xy(pnt),(min.getz()+max.getz())/2);
  return pnt.getx()>=min.getx() && pnt.getx()<=max.getx() &&
	 pnt.gety()>=min.gety() && pnt.gety()<=max.gety() &&
	 pnt.getz()>=min.getz() && pnt.getz()<=max.getz();
}

int LasHeader::getVersion()
{
  return (versionMajor<<8)+versionMinor;
}

int LasHeader::getPointFormat()
{
  return pointFormat;
}

LasPoint LasHeader::readPoint(size_t num)
{
  LasPoint ret;
  int xInt,yInt,zInt;
  int temp;
  lasfile->seekg(num*pointLength+pointOffset,ios_base::beg);
  xInt=readleint(*lasfile);
  yInt=readleint(*lasfile);
  zInt=readleint(*lasfile);
  ret.intensity=readleshort(*lasfile);
  //if (xInt==0 && yInt==0 && zInt==0)
    //cout<<"xyz0\n";
  if (pointFormat<6)
  {
    temp=lasfile->get();
    ret.returnNum=temp&7;
    ret.nReturns=(temp>>3)&7;
    ret.scanDirection=(temp>>6)&1;
    ret.edgeLine=(temp>>7)&1;
    ret.classification=(unsigned char)lasfile->get();
    ret.classificationFlags=(ret.classification>>5)&7;
    ret.classification&=31;
    ret.scanAngle=degtobin((signed char)lasfile->get());
    ret.userData=(unsigned char)lasfile->get();
    ret.pointSource=readleshort(*lasfile);
  }
  else // formats 6 through 10
  {
    temp=lasfile->get();
    ret.returnNum=temp&15;
    ret.nReturns=(temp>>4)&15;
    temp=lasfile->get();
    ret.classificationFlags=temp&15;
    ret.scannerChannel=(temp>>4)&3;
    ret.scanDirection=(temp>>6)&1;
    ret.edgeLine=(temp>>7)&1;
    ret.classification=(unsigned char)lasfile->get();
    ret.userData=(unsigned char)lasfile->get();
    ret.scanAngle=degtobin(readleshort(*lasfile)*0.006);
    ret.pointSource=readleshort(*lasfile);
  }
  if ((1<<pointFormat)&MASK_GPSTIME) // 10-5, 4, 3, or 1
    ret.gpsTime=readledouble(*lasfile);
  if ((1<<pointFormat)&MASK_RGB) // 10, 8, 7, 5, 3, or 2
  {
    ret.red=readleshort(*lasfile);
    ret.green=readleshort(*lasfile);
    ret.blue=readleshort(*lasfile);
  }
  if ((1<<pointFormat)&MASK_NIR) // 10 or 8
    ret.nir=readleshort(*lasfile);
  if ((1<<pointFormat)&MASK_WAVE) // 10, 9, 5 or 4
#ifdef WAVEFORM
  {
    ret.waveIndex=(unsigned char)lasfile->get();
    ret.waveformOffset=readlelong(*lasfile);
    ret.waveformSize=readleint(*lasfile);
    ret.waveformTime=readlefloat(*lasfile);
    ret.xDir=readlefloat(*lasfile);
    ret.yDir=readlefloat(*lasfile);
    ret.zDir=readlefloat(*lasfile);
  }
#else
  {
    lasfile->get();
    readlelong(*lasfile);
    readleint(*lasfile);
    readlefloat(*lasfile);
    readlefloat(*lasfile);
    readlefloat(*lasfile);
    readlefloat(*lasfile);
  }
#endif
  ret.location=xyz(xOffset+xScale*xInt,yOffset+yScale*yInt,zOffset+zScale*zInt)*unit;
  if (ret.location.getx()>maxX*unit || ret.location.getx()<minX*unit ||
      ret.location.gety()>maxY*unit || ret.location.gety()<minY*unit ||
      ret.location.getz()>maxZ*unit || ret.location.getz()<minZ*unit)
  {
    cerr<<"Point out of range\n";
    //ret.location=nanxyz;
  }
  if (!lasfile->good())
    throw -1;
  nReadPoints++;
  return ret;
}

void LasHeader::writePoint(const LasPoint &pnt)
{
  int xInt,yInt,zInt;
  double writtenX,writtenY,writtenZ;
  lasfile->seekp(writePos,ios::beg);
  xInt=lrint((pnt.location.getx()/unit-xOffset)/xScale);
  yInt=lrint((pnt.location.gety()/unit-yOffset)/yScale);
  zInt=lrint((pnt.location.getz()/unit-zOffset)/zScale);
  writeleint(*lasfile,xInt);
  writeleint(*lasfile,yInt);
  writeleint(*lasfile,zInt);
  writtenX=xInt*xScale+xOffset;
  writtenY=yInt*yScale+yOffset;
  writtenZ=zInt*zScale+zOffset;
  if (writtenX>maxX)
    maxX=writtenX;
  if (writtenX<minX)
    minX=writtenX;
  if (writtenY>maxY)
    maxY=writtenY;
  if (writtenY<minY)
    minY=writtenY;
  if (writtenZ>maxZ)
    maxZ=writtenZ;
  if (writtenZ<minZ)
    minZ=writtenZ;
  assert(dist(xyz(writtenX,writtenY,writtenZ),pnt.location/unit)<sqrt(sqr(xScale)+sqr(yScale)+sqr(zScale))*0.6);
  writeleshort(*lasfile,pnt.intensity);
  if (pointFormat<6)
  {
    lasfile->put((pnt.returnNum&7)+((pnt.nReturns&7)<<3)+((pnt.scanDirection&1)<<6)+((pnt.edgeLine&1)<<7));
    lasfile->put((pnt.classification&31)+((pnt.classificationFlags&7)<<5));
    lasfile->put(lrint(bintodeg(pnt.scanAngle)));
    lasfile->put(pnt.userData);
    writeleshort(*lasfile,pnt.pointSource);
  }
  else
  {
    lasfile->put((pnt.returnNum&15)+((pnt.nReturns&15)<<4));
    lasfile->put((pnt.classificationFlags&15)+((pnt.scannerChannel&3)<<4)+((pnt.scanDirection&1)<<6)+((pnt.edgeLine&1)<<7));
    lasfile->put(pnt.classification);
    lasfile->put(pnt.userData);
    writeleshort(*lasfile,lrint(bintodeg(pnt.scanAngle)/0.006));
    writeleshort(*lasfile,pnt.pointSource);
  }
  if ((1<<pointFormat)&MASK_GPSTIME) // 10-5, 4, 3, or 1
    writeledouble(*lasfile,pnt.gpsTime);
  if ((1<<pointFormat)&MASK_RGB) // 10, 8, 7, 5, 3, or 2
  {
    writeleshort(*lasfile,pnt.red);
    writeleshort(*lasfile,pnt.green);
    writeleshort(*lasfile,pnt.blue);
  }
  if ((1<<pointFormat)&MASK_NIR) // 10 or 8
    writeleshort(*lasfile,pnt.nir);
  if ((1<<pointFormat)&MASK_WAVE) // 10, 9, 5 or 4
#ifdef WAVEFORM
  {
    lasfile->put(pnt.waveIndex);
    writelelong(*lasfile,pnt.waveformOffset);
    writeleint(*lasfile,pnt.waveformSize);
    writelefloat(*lasfile,pnt.waveformTime);
    writelefloat(*lasfile,pnt.xDir);
    writelefloat(*lasfile,pnt.yDir);
    writelefloat(*lasfile,pnt.zDir);
  }
#else
  {
    lasfile->put('\0');
    writelelong(*lasfile,0);
    writeleint(*lasfile,0);
    writelefloat(*lasfile,0);
    writelefloat(*lasfile,0);
    writelefloat(*lasfile,0);
    writelefloat(*lasfile,0);
  }
#endif
  nPoints[0]++;
  if (pnt.returnNum>0 && pnt.returnNum<16)
    nPoints[pnt.returnNum]++;
  writePos+=pointLength;
  startExtendedVariableLength=writePos;
}
