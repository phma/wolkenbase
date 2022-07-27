/******************************************************/
/*                                                    */
/* wolkencanvas.cpp - canvas for drawing point cloud  */
/*                                                    */
/******************************************************/
/* Copyright 2020-2022 Pierre Abbat.
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
#include <QPainter>
#include <QtWidgets>
#include <cmath>
#include <climits>
#include "wolkencanvas.h"
#include "cloud.h"
#include "fileio.h"
#include "relprime.h"
#include "angle.h"
#include "octree.h"
#include "freeram.h"
#include "threads.h"
#include "ldecimal.h"
#include "tile.h"
#include "scan.h"

using namespace std;
namespace cr=std::chrono;

Qt::GlobalColor lightColors[]=
{
  Qt::red,Qt::cyan,Qt::gray,Qt::blue,Qt::yellow,Qt::green,Qt::magenta
};
Qt::GlobalColor darkColors[]=
{
  Qt::black,Qt::darkCyan,Qt::darkGreen,Qt::darkYellow,
  Qt::darkRed,Qt::darkMagenta,Qt::darkBlue,Qt::darkGray
};

QPoint qptrnd(xy pnt)
{
  return QPoint(lrint(pnt.getx()),lrint(pnt.gety()));
}

WolkenCanvas::WolkenCanvas(QWidget *parent):QWidget(parent)
{
  setAutoFillBackground(true);
  setBackgroundRole(QPalette::Base);
  setMinimumSize(40,30);
  setMouseTracking(true);
  countedBlock=0;
  fileCountdown=splashScreenTime=dartAngle=ballAngle=0;
  lowRam=freeRam()/7;
}

QPointF WolkenCanvas::worldToWindow(xy pnt)
{
  pnt.roscat(worldCenter,0,scale,windowCenter);
  QPointF ret(pnt.getx(),height()-pnt.gety());
  return ret;
}

xy WolkenCanvas::windowToWorld(QPointF pnt)
{
  xy ret(pnt.x(),height()-pnt.y());
  ret.roscat(windowCenter,0,1/scale,worldCenter);
  return ret;
}

vector<int> WolkenCanvas::fileHitTest(xy pnt)
{
  vector<int> ret;
  xyz pntz(pnt,NAN);
  int i;
  for (i=0;i<inFileHeaders.size();i++)
    if (inFileHeaders[i].inBox(pntz))
      ret.push_back(i);
  return ret;
}

array<double,3> WolkenCanvas::pixelColorRead(int x,int y)
{
  Column col(windowToWorld(QPoint(x,y)),1/scale);
  array<double,2> hilo=octStore.hiLoPointsIn(col);
  array<double,3> ret;
  octStore.disown();
  if (hilo[0]>hilo[1])
    ret[0]=ret[1]=ret[2]=NAN;
  else
  {
    ret[0]=(hilo[0]-br.low())/(br.high()-br.low());
    ret[1]=(hilo[1]-hilo[0])/(br.high()-br.low());
    ret[2]=(br.high()-hilo[1])/(br.high()-br.low());
  }
  return ret;
}

array<double,3> WolkenCanvas::pixelColorTile(Eisenstein tileAddr)
{
  tileMutex.lock();
  Tile *thisTile=&tiles[tileAddr];
  tileMutex.unlock();
  array<double,3> ret;
  if (thisTile->nPoints==0)
    ret[0]=ret[1]=ret[2]=1;
  else
  {
    ret[0]=double(thisTile->nGround)/thisTile->nPoints;
    ret[1]=(log(thisTile->nPoints)-log(minTile.nPoints))/
	   (log(maxTile.nPoints)-log(minTile.nPoints));
    //ret[1]=thisTile->treeFlags;
    ret[2]=(log(thisTile->density)-log(minTile.density))/
	   (log(maxTile.density)-log(minTile.density));
  }
  return ret;
}

array<double,3> WolkenCanvas::pixelColorTile(int x,int y)
{
  xy pnt=windowToWorld(QPoint(x,y));
  Eisenstein tileAddr=snake.tileAddress(pnt);
  return pixelColorTile(tileAddr);
}

void WolkenCanvas::sizeToFit()
{
  int i;
  double xscale,yscale;
  br.clear();
  for (i=0;i<inFileHeaders.size();i++)
  {
    br.include(inFileHeaders[i].minCorner());
    br.include(inFileHeaders[i].maxCorner());
  }
  if (br.left()>=br.right() && br.bottom()>=br.top())
  {
    worldCenter=xy(0,0);
    scale=1;
  }
  else
  {
    worldCenter=xy((br.left()+br.right())/2,(br.bottom()+br.top())/2);
    xscale=width()/(br.right()-br.left());
    yscale=height()/(br.top()-br.bottom());
    if (xscale<yscale)
      scale=xscale;
    else
      scale=yscale;
  }
  fileCountdown=inFileHeaders.size()+10;
  if (splashScreenTime)
    scale=scale*2/3;
}

void WolkenCanvas::tick()
{
  int i,sz,timeLimit;
  size_t sofar=0,total=0;
  int tstatus=getThreadStatus();
  double splashElev,radius;
  xy gradient,A,B,C;
  xy dartCorners[4];
  xy leftTickmark,rightTickmark,tickmark;
  QPolygonF polygon;
  QPolygon swath;
  QPointF circleCenter;
  QRectF circleBox,textBox,fileBound;
  array<double,3> pcolor;
  array<int,2> pixel;
  string scaleText;
  int pixminx=width(),pixminy=height(),pixmaxx=0,pixmaxy=0;
  cr::nanoseconds elapsed=cr::milliseconds(0);
  cr::time_point<cr::steady_clock> timeStart=clk.now();
  xy oldBallPos=ballPos;
  QPainter painter(&frameBuffer);
  QBrush brush;
  QPen pen;
  painter.setRenderHint(QPainter::Antialiasing,true);
  painter.setPen(Qt::NoPen);
  brush.setStyle(Qt::SolidPattern);
  for (i=0;i<inFileHeaders.size();i++)
  {
    sofar+=inFileHeaders[i].numberReadPoints();
    total+=inFileHeaders[i].numberPoints();
  }
  readFileProgress(sofar,total);
  if ((tstatus&0x3ffbfeff)%1048577==0)
    state=(tstatus&0x3ffbfeff)/1048577;
  else
  {
    state=0;
    //cout<<"tstatus "<<((tstatus>>20)&1023)<<':'<<((tstatus>>10)&1023)<<':'<<(tstatus&1023)<<endl;
  }
  if ((state==TH_WAIT || state==TH_PAUSE) && currentAction)
    state=-currentAction;
  // Compute the new position of the ball, and update a swath containing the ball's motion.
  penPos=(penPos+5)%144;
  ballPos=lis.move()+xy(10,10);
  switch ((ballPos.gety()>oldBallPos.gety())*2+(ballPos.getx()>oldBallPos.getx()))
  {
    case 0: // Ball is moving up and left
      swath<<qptrnd(ballPos+xy(-10.5,-10.5))<<qptrnd(ballPos+xy(-10.5,10.5));
      swath<<qptrnd(oldBallPos+xy(-10.5,10.5))<<qptrnd(oldBallPos+xy(10.5,10.5));
      swath<<qptrnd(oldBallPos+xy(10.5,-10.5))<<qptrnd(ballPos+xy(10.5,-10.5));
      break;
    case 1: // Ball is moving up and right
      swath<<qptrnd(oldBallPos+xy(-10.5,-10.5))<<qptrnd(oldBallPos+xy(-10.5,10.5));
      swath<<qptrnd(oldBallPos+xy(10.5,10.5))<<qptrnd(ballPos+xy(10.5,10.5));
      swath<<qptrnd(ballPos+xy(10.5,-10.5))<<qptrnd(ballPos+xy(-10.5,-10.5));
      break;
    case 2: // Ball is moving down and left
      swath<<qptrnd(oldBallPos+xy(10.5,10.5))<<qptrnd(oldBallPos+xy(10.5,-10.5));
      swath<<qptrnd(oldBallPos+xy(-10.5,-10.5))<<qptrnd(ballPos+xy(-10.5,-10.5));
      swath<<qptrnd(ballPos+xy(-10.5,10.5))<<qptrnd(ballPos+xy(10.5,10.5));
      break;
    case 3: // Ball is moving down and right
      swath<<qptrnd(ballPos+xy(10.5,10.5))<<qptrnd(ballPos+xy(10.5,-10.5));
      swath<<qptrnd(oldBallPos+xy(10.5,-10.5))<<qptrnd(oldBallPos+xy(-10.5,-10.5));
      swath<<qptrnd(oldBallPos+xy(-10.5,10.5))<<qptrnd(ballPos+xy(-10.5,10.5));
      break;
  }
  update(QRegion(swath));
  // Draw a rectangle for each open LAS file.
  if (fileCountdown==inFileHeaders.size())
    frameBuffer.fill();
  if (fileCountdown>=0 && fileCountdown<inFileHeaders.size())
  {
    QLinearGradient grad(worldToWindow(inFileHeaders[fileCountdown].minCorner()),
			 worldToWindow(inFileHeaders[fileCountdown].maxCorner()));
    fileBound=QRectF(worldToWindow(inFileHeaders[fileCountdown].minCorner()),
		     worldToWindow(inFileHeaders[fileCountdown].maxCorner()));
    grad.setColorAt(0,lightColors[fileCountdown%7]);
    grad.setColorAt(1,darkColors[fileCountdown%8]);
    painter.setBrush(grad);
    painter.drawRect(fileBound);
  }
  if (fileCountdown==0)
    update();
  if (fileCountdown>=0)
    fileCountdown--;
  // Paint a circle white as a background for the scale.
  if (maxScaleSize>0)
  {
    circleCenter=worldToWindow(scaleEnd);
    circleBox=QRectF(circleCenter.x()-maxScaleSize*scale,circleCenter.y()-maxScaleSize*scale,
		    maxScaleSize*scale*2,maxScaleSize*scale*2);
    painter.drawEllipse(circleBox);
    // Draw the scale.
    painter.setPen(Qt::black);
    tickmark=turn90(rightScaleEnd-leftScaleEnd)/10;
    leftTickmark=leftScaleEnd+tickmark;
    rightTickmark=rightScaleEnd+tickmark;
    polygon=QPolygonF();
    polygon<<worldToWindow(leftTickmark);
    polygon<<worldToWindow(leftScaleEnd);
    polygon<<worldToWindow(rightScaleEnd);
    polygon<<worldToWindow(rightTickmark);
    painter.drawPolyline(polygon);
    // Write the length of the scale.
    textBox=QRectF(worldToWindow(leftScaleEnd+2*tickmark),worldToWindow(rightScaleEnd));
    scaleText=ldecimal(scaleSize/lengthUnit,scaleSize/lengthUnit/10)+((lengthUnit==1)?" m":" ft");
    painter.drawText(textBox,Qt::AlignCenter,QString::fromStdString(scaleText));
  }
  painter.setPen(Qt::NoPen);
  if (state==TH_WAIT || state==TH_PAUSE)
    timeLimit=45;
  else
    timeLimit=20;
  while (elapsed<cr::milliseconds(timeLimit))
  {
    Eisenstein tileAddress=dequeueTileDone();
    Cylinder tileDone=snake.cyl(tileAddress);
    if (tileDone.getRadius()==0)
      break;
    circleCenter=worldToWindow(tileDone.getCenter());
    radius=tileDone.getRadius()*scale;
    ballAngle+=DEG1;
    if (circleCenter.x()+radius>=0 && circleCenter.x()-radius<=width() &&
	circleCenter.y()+radius>=0 && circleCenter.y()-radius<=height())
    {
      pcolor=pixelColorTile(tileAddress);
      painter.setPen(Qt::NoPen);
      painter.setBrush(QColor(lrint(pcolor[0]*255),lrint(pcolor[1]*255),lrint(pcolor[2]*255)));
      circleBox=QRectF(circleCenter.x()-radius,circleCenter.y()-radius,
		      radius*2,radius*2);
      painter.drawEllipse(circleBox);
      if (ceil(circleCenter.x()+radius)>pixmaxx)
	pixmaxx=ceil(circleCenter.x()+radius);
      if (floor(circleCenter.x()-radius)<pixminx)
	pixminx=floor(circleCenter.x()-radius);
      if (ceil(circleCenter.y()+radius)>pixmaxy)
	pixmaxy=ceil(circleCenter.y()+radius);
      if (floor(circleCenter.y()-radius)<pixminy)
	pixminy=floor(circleCenter.y()-radius);
    }
    elapsed=clk.now()-timeStart;
  }
  if (pixminx<=pixmaxx)
    update(pixminx,pixminy,pixmaxx-pixminx,pixmaxy-pixminy);
  pixminx=width();
  pixminy=height();
  pixmaxx=pixmaxy=0;
  while (elapsed<cr::milliseconds(timeLimit) && pixelsToPaint)
  {
    countedBlock+=0x69969669;
    octRoot.countPoints(countedBlock);
    pixel=peano.step();
    if (state==TH_READ || state==TH_WAIT)
      pcolor=pixelColorRead(pixel[0],pixel[1]);
    else
      pcolor=pixelColorTile(pixel[0],pixel[1]);
    if (std::isfinite(pcolor[0]))
    {
      painter.setPen(QColor(lrint(pcolor[0]*255),lrint(pcolor[1]*255),lrint(pcolor[2]*255)));
      painter.drawPoint(pixel[0],pixel[1]);
      if (pixel[0]>pixmaxx)
	pixmaxx=pixel[0];
      if (pixel[0]<pixminx)
	pixminx=pixel[0];
      if (pixel[1]>pixmaxy)
	pixmaxy=pixel[1];
      if (pixel[1]<pixminy)
	pixminy=pixel[1];
    }
    elapsed=clk.now()-timeStart;
    pixelsToPaint--;
  }
  if (pixminx<=pixmaxx)
    update(pixminx,pixminy,pixmaxx-pixminx+1,pixmaxy-pixminy+1);
  if (pointBufferSize() || (snake.progress()>0 && snake.progress()<1))
    pixelsToPaint=(width()*height()*7+3)/4;
  if (state==TH_SPLIT && snake.progress()==1)
    startCount();
  if (state==TH_POSTSCAN && snake.progress()==1)
    startClassify();
  if (state==TH_SCAN && snake.progress()==1)
    startPostscan();
  if (state==TH_READ && actionQueueEmpty() && pointBufferEmpty() && sofar==total)
    if (shallClassify)
      startScan();
    else
      startCount();
}

void WolkenCanvas::setSize()
{
  double oldScale=scale;
  xy oldWindowCenter=windowCenter;
  int dx,dy,i;
  QTransform tf;
  windowCenter=xy(width(),height())/2.;
  lis.resize(width()-20,height()-20);
  peano.resize(width(),height());
  pixelPointCount.resize(width());
  columnTotal.resize(width());
  for (i=0;i<width();i++)
    pixelPointCount[i].resize(height());
  sizeToFit();
  if (frameBuffer.isNull())
  {
    frameBuffer=QPixmap(width(),height());
    frameBuffer.fill(Qt::white);
  }
  else
  {
    tf.translate(-oldWindowCenter.getx(),-oldWindowCenter.gety());
    tf.scale(scale/oldScale,scale/oldScale);
    tf.translate(windowCenter.getx(),windowCenter.gety());
    QPixmap frameTemp=frameBuffer.transformed(tf);
    frameBuffer=QPixmap(width(),height());
    frameBuffer.fill(Qt::white);
    QPainter painter(&frameBuffer);
    dx=(width()-frameTemp.width())/2;
    dy=(height()-frameTemp.height())/2;
    QRect rect0(0,0,frameTemp.width(),frameTemp.height());
    QRect rect1(dx,dy,frameTemp.width(),frameTemp.height());
    painter.drawPixmap(rect1,frameTemp,rect0);
    update();
  }
  setScalePos();
  pixelsToPaint=(width()*height()*7+3)/4;
}

void WolkenCanvas::readFileHeader(string name)
{
  inFileHeaders.resize(inFileHeaders.size()+1);
  cout<<"Opening "<<name<<endl;
  inFileHeaders.back().openRead(name);
  inFileHeaders.back().setUnit(lengthUnit);
  setSize();
}

void WolkenCanvas::saveFile()
// Save the point cloud that was read in as PLY or XYZ without classifying
{
  int i,dialogResult;
  vector<xyz> limits;
  QStringList files;
  string fileName;
  ThreadAction ta;
  BoundRect br;
  double side;
  multimap<int64_t,LasHeader *> sorter;
  multimap<int64_t,LasHeader *>::iterator j;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Save point cloud"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  if (inFileHeaders.size())
  {
    fileName=noExt(inFileHeaders[0].getFileName())+".las";
    fileDialog->selectFile(QString::fromStdString(fileName));
  }
  fileDialog->setNameFilter(tr("(*.las)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    saveFileName=files[0].toStdString();
    if (extension(saveFileName)==".las")
      saveFileName=noExt(saveFileName);
    for (i=0;i<inFileHeaders.size();i++)
    {
      limits.push_back(inFileHeaders[i].minCorner());
      limits.push_back(inFileHeaders[i].maxCorner());
      br.include(inFileHeaders[i].minCorner());
      br.include(inFileHeaders[i].maxCorner());
      if (cloud.size()<4294967295)
	inFileHeaders[i].setPointFormat(0);
      sorter.insert(pair<int64_t,LasHeader *>(-inFileHeaders[i].numberPoints(),&inFileHeaders[i]));
    }
    classTotals.clear();
    classTotals[0]=cloud.size();
    side=br.right()-br.left();
    if (br.top()-br.bottom()>side)
      side=br.top()-br.bottom();
    if (br.high()-br.low()>side)
      side=br.high()-br.low();
    cube=Cube(xyz((br.right()+br.left())/2,(br.top()+br.bottom())/2,
		  (br.high()+br.low())/2),side);
  }
  delete fileDialog;
  fileDialog=nullptr;
  if (dialogResult)
    writeFile();
}

void WolkenCanvas::startProcess(bool clfy)
{
  int i,dialogResult;
  vector<xyz> limits;
  QStringList files;
  string fileName;
  ThreadAction ta;
  BoundRect br;
  double side;
  multimap<int64_t,LasHeader *> sorter;
  multimap<int64_t,LasHeader *>::iterator j;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Save classified point cloud"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  fileDialog->setNameFilter(tr("(*.las)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    saveFileName=files[0].toStdString();
    if (extension(saveFileName)==".las")
      saveFileName=noExt(saveFileName);
    waitForThreads(TH_READ);
    for (i=0;i<inFileHeaders.size();i++)
    {
      limits.push_back(inFileHeaders[i].minCorner());
      limits.push_back(inFileHeaders[i].maxCorner());
      br.include(inFileHeaders[i].minCorner());
      br.include(inFileHeaders[i].maxCorner());
      sorter.insert(pair<int64_t,LasHeader *>(-inFileHeaders[i].numberPoints(),&inFileHeaders[i]));
    }
    octRoot.sizeFit(limits);
    side=br.right()-br.left();
    if (br.top()-br.bottom()>side)
      side=br.top()-br.bottom();
    if (br.high()-br.low()>side)
      side=br.high()-br.low();
    cube=Cube(xyz((br.right()+br.left())/2,(br.top()+br.bottom())/2,
		  (br.high()+br.low())/2),side);
    snake.setSize(cube,tileSize);
    initTiles();
    shallClassify=clfy;
    for (j=sorter.begin();j!=sorter.end();++j)
    {
      cout<<"Read file "<<baseName(j->second->getFileName())<<endl;
      ta.hdr=j->second;
      ta.opcode=ACT_READ;
      enqueueAction(ta);
    }
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void WolkenCanvas::startProcessClassify()
{
  startProcess(true);
}

void WolkenCanvas::startProcessAsIs()
{
  startProcess(false);
}

void WolkenCanvas::startScan()
{
  waitForThreads(TH_SCAN);
  cout<<"Starting scan\n";
  octStore.shrink();
  currentAction=0;
}

void WolkenCanvas::startPostscan()
{
  waitForThreads(TH_POSTSCAN);
  cout<<"Starting postscan\n";
  snake.restart();
}

void WolkenCanvas::startClassify()
{
  waitForThreads(TH_SPLIT);
  cout<<"Starting classifying\n";
  octStore.setIgnoreDupes(true);
  snake.restart();
}

void WolkenCanvas::startCount()
{
  int i;
  ThreadAction ta;
  waitForThreads(TH_PAUSE);
  cout<<"Counting points\n";
  octStore.setIgnoreDupes(false);
  for (i=0;i<nThreads();i++)
  {
    ta.opcode=ACT_COUNT;
    enqueueAction(ta);
  }
}

void WolkenCanvas::writeFile()
{
  int i;
  map<int,size_t>::iterator j;
  vector<int> formats;
  ThreadAction ta;
  waitForThreads(TH_PAUSE);
  cout<<"Classified points:\n";
  for (j=classTotals.begin();j!=classTotals.end();++j)
    cout<<j->first<<' '<<j->second<<endl;
  for (i=0;i<inFileHeaders.size();i++)
    formats.push_back(inFileHeaders[i].getPointFormat());
  cloudOutput.pointFormat=joinPointFormat(formats);
  cloudOutput.minCor=xyz(br.left(),br.bottom(),br.low());
  cloudOutput.maxCor=xyz(br.right(),br.top(),br.high());
  cloudOutput.scale=xyz(0,0,0); // TODO check the inFileHeaders for same scale and offset
  cloudOutput.nInputFiles=inFileHeaders.size();
  cloudOutput.pointsPerFile=10000000;
  cloudOutput.unit=lengthUnit;
  cloudOutput.openFiles(saveFileName,classTotals);
  ta.opcode=ACT_WRITE;
  enqueueAction(ta);
}

void WolkenCanvas::clearCloud()
{
  inFileHeaders.clear();
  cloud.clear();
  setSize();
}

void WolkenCanvas::setScalePos()
// Set the position of the scale at the lower left or right corner.
{
  xy leftScalePos,rightScalePos,disp;
  bool right=false;
  leftScalePos=windowToWorld(QPointF(width()*0.01,height()*0.99));
  rightScalePos=windowToWorld(QPointF(width()*0.99,height()*0.99));
  maxScaleSize=0; // TODO fit to rectangles of opened files
  scaleSize=lengthUnit;
  while (scaleSize>maxScaleSize)
    scaleSize/=10;
  while (scaleSize<maxScaleSize)
    scaleSize*=10;
  if (scaleSize/2<maxScaleSize)
    scaleSize/=2;
  else if (scaleSize/5<maxScaleSize)
    scaleSize/=5;
  else
    scaleSize/=10;
  if (right)
  {
    scaleEnd=rightScaleEnd=rightScalePos;
    disp=leftScalePos-rightScalePos;
    disp/=disp.length();
    leftScaleEnd=rightScaleEnd+scaleSize*disp;
  }
  else
  {
    scaleEnd=leftScaleEnd=leftScalePos;
    disp=rightScalePos-leftScalePos;
    disp/=disp.length();
    rightScaleEnd=leftScaleEnd+scaleSize*disp;
  }
}

void WolkenCanvas::setLengthUnit(double unit)
{
  lengthUnit=unit;
  setScalePos();
}

void WolkenCanvas::paintEvent(QPaintEvent *event)
{
  int i;
  QPainter painter(this);
  QRectF square(ballPos.getx()-10,ballPos.gety()-10,20,20);
  QRectF sclera(ballPos.getx()-10,ballPos.gety()-5,20,10);
  QRectF iris(ballPos.getx()-5,ballPos.gety()-5,10,10);
  QRectF pupil(ballPos.getx()-2,ballPos.gety()-2,4,4);
  QRectF paper(ballPos.getx()-7.07,ballPos.gety()-10,14.14,20);
  QPolygonF octagon;
  double x0,x1,y;
  octagon<<QPointF(ballPos.getx()-10,ballPos.gety()-4.14);
  octagon<<QPointF(ballPos.getx()-4.14,ballPos.gety()-10);
  octagon<<QPointF(ballPos.getx()+4.14,ballPos.gety()-10);
  octagon<<QPointF(ballPos.getx()+10,ballPos.gety()-4.14);
  octagon<<QPointF(ballPos.getx()+10,ballPos.gety()+4.14);
  octagon<<QPointF(ballPos.getx()+4.14,ballPos.gety()+10);
  octagon<<QPointF(ballPos.getx()-4.14,ballPos.gety()+10);
  octagon<<QPointF(ballPos.getx()-10,ballPos.gety()+4.14);
  painter.setRenderHint(QPainter::Antialiasing,true);
  painter.drawPixmap(this->rect(),frameBuffer,this->rect());
  switch (state)
  {
    case TH_SPLIT:
      painter.setBrush(Qt::yellow);
      painter.setPen(Qt::NoPen);
      painter.drawChord(square,lrint(bintodeg(ballAngle)*16),2880);
      painter.setBrush(Qt::blue);
      painter.drawChord(square,lrint(bintodeg(ballAngle+DEG180)*16),2880);
      break;
    case -ACT_WRITE:
      painter.setBrush(Qt::white);
      painter.setPen(Qt::NoPen);
      painter.drawRect(paper);
      painter.setPen(Qt::black);
      for (i=0;i<8;i++)
      {
	y=ballPos.gety()-9+i*18./7;
	x0=ballPos.getx()-6;
	if (penPos/18==i)
	  x1=x0+(i%18+0.5)*2/3;
	else
	  x1=x0+12*(i<penPos/18);
	if (x1>x0)
	  painter.drawLine(QPointF(x0,y),QPointF(x1,y));
      }
      break;
    case TH_READ:
    case -ACT_LOAD:
      if (!(actionQueueEmpty() && pointBufferEmpty()) || state<0)
      {
	painter.setPen(Qt::NoPen);
	painter.setBrush(Qt::lightGray);
	painter.drawEllipse(sclera);
	painter.setBrush(Qt::blue);
	painter.drawEllipse(iris);
	painter.setBrush(Qt::black);
	painter.drawEllipse(pupil);
      }
      break;
    case 0:
      painter.setBrush(Qt::lightGray);
      painter.setPen(Qt::NoPen);
      painter.drawEllipse(QPointF(ballPos.getx(),ballPos.gety()),10,10);
      break;
  }
}

void WolkenCanvas::resizeEvent(QResizeEvent *event)
{
  setSize();
  QWidget::resizeEvent(event);
}

void WolkenCanvas::mouseMoveEvent(QMouseEvent *event)
{
  xy eventLoc=windowToWorld(event->pos());
  string tipString;
  vector<int> filenums=fileHitTest(eventLoc);
  int i;
  for (i=0;i<filenums.size();i++)
  {
    if (i)
      tipString+=' ';
    tipString+=baseName(inFileHeaders[filenums[i]].getFileName());
  }
  if (snake.cyl(Eisenstein(0,0)).getRadius())
  {
    tileMutex.lock();
    double den=tiles[snake.tileAddress(eventLoc)].density;
    int nGround=tiles[snake.tileAddress(eventLoc)].nGround;
    int nPoints=tiles[snake.tileAddress(eventLoc)].nPoints;
    tileMutex.unlock();
    if (den)
      tipString=tipString+'\n'+ldecimal(den,den/1e3)+"/m²";
    if (nGround)
      tipString=tipString+'\n'+to_string(nGround)+"/"+to_string(nPoints);
  }
  QToolTip::showText(event->globalPos(),QString::fromStdString(tipString),this);
}
