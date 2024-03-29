/******************************************************/
/*                                                    */
/* cloudoutput.cpp - write LAS files after classifying*/
/*                                                    */
/******************************************************/
/* Copyright 2021,2022 Pierre Abbat.
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
#include "octree.h"
#include "cloud.h"
using namespace std;

string ndecimal(size_t n,int dig)
{
  char buf[24];
  char fmt[8];
  if (dig)
  {
    sprintf(fmt,"%%0%dld",dig);
    sprintf(buf,fmt,n);
  }
  else
    buf[0]=0;
  return buf;
}

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
    case 3:
      qret=tr("lowveg");
      break;
    case 4:
      qret=tr("medveg");
      break;
    case 5:
      qret=tr("highveg");
      break;
    case 6:
      qret=tr("building");
      break;
    case 7:
      qret=tr("lownoise");
      break;
    case 9:
      qret=tr("water");
      break;
    case 10:
      qret=tr("rail");
      break;
    case 11:
      qret=tr("road");
      break;
    case 12:
      qret=tr("overlap"); // in formats 0-5, reserved in 6-10
      break;
    case 13:
      qret=tr("wireguard");
      break;
    case 14:
      qret=tr("conductor");
      break;
    case 15:
      qret=tr("tower");
      break;
    case 16:
      qret=tr("insulator");
      break;
    case 17:
      qret=tr("bridge");
      break;
    case 18:
      qret=tr("highnoise");
      break;
    case 19:
      qret=tr("overhead");
      break;
    case 20:
      qret=tr("ignground");
      break;
    case 21:
      qret=tr("snow");
      break;
    case 22:
      qret=tr("temporal");
      break;
    default:
      qret=tr("reserved")+QString::number(n);
  }
  return qret.toStdString();
}

void CloudOutput::openFiles(string name,map<int,size_t> classTotals)
{
  int i,sysId;
  size_t quot;
  map<int,size_t>::iterator j;
  map<int,deque<LasHeader> >::iterator k;
  int nDigits=0; // number of digits appended to filenames
  grandTotal=0;
  if (separateClasses)
    sysId=SI_EXTRACT;
  else if (nInputFiles>1)
    sysId=SI_MERGE;
  else
    sysId=SI_MODIFY;
  for (j=classTotals.begin();j!=classTotals.end();++j)
    grandTotal+=j->second;
  if (pointsPerFile)
  {
    quot=(grandTotal+pointsPerFile-1)/pointsPerFile;
    if (quot)
      quot--; // If quot is 10, only one digit is needed.
    if (!quot)
      quot++; // If quot is 0, still append a digit.
    while (quot)
    {
      quot/=10;
      nDigits++;
    }
  }
  if (separateClasses)
    for (j=classTotals.begin();j!=classTotals.end();++j)
    {
      if (pointsPerFile)
	quot=(j->second+pointsPerFile-1)/pointsPerFile;
      else
	quot=1;
      for (i=0;i<quot;i++)
      {
	headers[j->first].push_back(LasHeader());
	headers[j->first][i].setZipped(writeLaz);
	headers[j->first][i].openWrite(name+'-'+className(j->first)+
				       (pointsPerFile?"-":"")+ndecimal(i,nDigits)+
				       (writeLaz?".laz":".las"),sysId);
	headers[j->first][i].setUnit(unit);
	headers[j->first][i].setScale(minCor,maxCor,scale);
	headers[j->first][i].setVersion(1,4);
	headers[j->first][i].setPointFormat(pointFormat);
      }
    }
  else
  {
    if (pointsPerFile)
      quot=(grandTotal+pointsPerFile-1)/pointsPerFile;
    else
      quot=1;
    for (i=0;i<quot;i++)
    {
      headers[0].push_back(LasHeader());
      headers[0][i].openWrite(name+(pointsPerFile?"-":"")+
			      ndecimal(i,nDigits)+".las",sysId);
      headers[0][i].setUnit(unit);
      headers[0][i].setScale(minCor,maxCor,scale);
      headers[0][i].setVersion(1,4);
      headers[0][i].setPointFormat(pointFormat);
    }
  }
}

void CloudOutput::writeFiles()
{
  long long i;
  int j;
  long long min;
  int nextBlocks[256];
  vector<LasPoint> blockPoints;
  map<int,deque<LasHeader> >::iterator k;
  for (i=0;i<octStore.getNumBlocks();i++)
  {
    for (k=headers.begin();k!=headers.end();++k)
    {
      min=grandTotal;
      for (j=0;j<k->second.size();j++)
	if (k->second[j].numberPoints()<min)
	{
	  nextBlocks[k->first]=j;
	  min=k->second[j].numberPoints();
	}
    }
    blockPoints=octStore.getAll(i);
    for (j=0;j<blockPoints.size();j++)
      headers[blockPoints[j].classification]
	     [nextBlocks[blockPoints[j].classification]].
	     writePoint(blockPoints[j]);
  }
  for (i=0;i<getNumCloudBlocks();i++)
  {
    for (k=headers.begin();k!=headers.end();++k)
    {
      min=grandTotal;
      for (j=0;j<k->second.size();j++)
	if (k->second[j].numberPoints()<min)
	{
	  nextBlocks[k->first]=j;
	  min=k->second[j].numberPoints();
	}
    }
    blockPoints=getCloudBlock(i);
    for (j=0;j<blockPoints.size();j++)
      headers[blockPoints[j].classification]
	     [nextBlocks[blockPoints[j].classification]].
	     writePoint(blockPoints[j]);
  }
}

void CloudOutput::closeFiles()
{
  int i;
  map<int,deque<LasHeader> >::iterator k;
  for (k=headers.begin();k!=headers.end();++k)
  {
    for (i=0;i<k->second.size();i++)
    {
      k->second[i].writeHeader();
      k->second[i].close();
    }
    k->second.clear();
  }
}
