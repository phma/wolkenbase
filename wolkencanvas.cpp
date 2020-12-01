/******************************************************/
/*                                                    */
/* wolkencanvas.cpp - canvas for drawing point cloud  */
/*                                                    */
/******************************************************/
/* Copyright 2020 Pierre Abbat.
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
#include "wolkencanvas.h"
#include "fileio.h"
#include "boundrect.h"
#include "relprime.h"
#include "angle.h"
#include "octree.h"
#include "freeram.h"
#include "threads.h"
#include "ldecimal.h"

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
  for (i=0;i<fileHeaders.size();i++)
    if (fileHeaders[i].inBox(pntz))
      ret.push_back(i);
  return ret;
}

void WolkenCanvas::sizeToFit()
{
  int i;
  double xscale,yscale;
  BoundRect br;
  for (i=0;i<fileHeaders.size();i++)
  {
    br.include(fileHeaders[i].minCorner());
    br.include(fileHeaders[i].maxCorner());
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
  fileCountdown=fileHeaders.size()+10;
  if (splashScreenTime)
    scale=scale*2/3;
}

void WolkenCanvas::tick()
{
  int i,sz,timeLimit;
  size_t sofar=0,total=0;
  int tstatus=getThreadStatus();
  double splashElev;
  xy gradient,A,B,C;
  xy dartCorners[4];
  xy leftTickmark,rightTickmark,tickmark;
  QPolygonF polygon;
  QPolygon swath;
  QPointF circleCenter;
  QRectF circleBox,textBox,fileBound;
  string scaleText;
  cr::nanoseconds elapsed=cr::milliseconds(0);
  cr::time_point<cr::steady_clock> timeStart=clk.now();
  xy oldBallPos=ballPos;
  QPainter painter(&frameBuffer);
  QBrush brush;
  QPen pen;
  painter.setRenderHint(QPainter::Antialiasing,true);
  painter.setPen(Qt::NoPen);
  brush.setStyle(Qt::SolidPattern);
  for (i=0;i<fileHeaders.size();i++)
  {
    sofar+=fileHeaders[i].numberReadPoints();
    total+=fileHeaders[i].numberPoints();
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
  if (fileCountdown==fileHeaders.size())
    frameBuffer.fill();
  if (fileCountdown>=0 && fileCountdown<fileHeaders.size())
  {
    QLinearGradient grad(worldToWindow(fileHeaders[fileCountdown].minCorner()),
			 worldToWindow(fileHeaders[fileCountdown].maxCorner()));
    fileBound=QRectF(worldToWindow(fileHeaders[fileCountdown].minCorner()),
		     worldToWindow(fileHeaders[fileCountdown].maxCorner()));
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
}

void WolkenCanvas::setSize()
{
  double oldScale=scale;
  xy oldWindowCenter=windowCenter;
  int dx,dy;
  QTransform tf;
  windowCenter=xy(width(),height())/2.;
  lis.resize(width()-20,height()-20);
  peano.resize(width(),height());
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
}

void WolkenCanvas::readFileHeader(string name)
{
  fileHeaders.resize(fileHeaders.size()+1);
  cout<<"Opening "<<name<<endl;
  fileHeaders.back().openRead(name);
  fileHeaders.back().setUnit(lengthUnit);
  setSize();
}

void WolkenCanvas::startProcess()
{
  int i;
  vector<xyz> limits;
  ThreadAction ta;
  waitForThreads(TH_READ);
  for (i=0;i<fileHeaders.size();i++)
  {
    limits.push_back(fileHeaders[i].minCorner());
    limits.push_back(fileHeaders[i].maxCorner());
  }
  octRoot.sizeFit(limits);
  for (i=0;i<fileHeaders.size();i++)
  {
    cout<<"Read file "<<baseName(fileHeaders[i].getFileName())<<endl;
    ta.hdr=&fileHeaders[i];
    ta.opcode=ACT_READ;
    enqueueAction(ta);
  }
}

void WolkenCanvas::clearCloud()
{
  fileHeaders.clear();
  setSize();
}

void WolkenCanvas::setScalePos()
// Set the position of the scale at the lower left or right corner.
{
  xy leftScalePos,rightScalePos,disp;
  bool right=false;
  leftScalePos=windowToWorld(QPointF(width()*0.01,height()*0.99));
  rightScalePos=windowToWorld(QPointF(width()*0.99,height()*0.99));
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
    case 99: // FIXME writing file
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
     case -ACT_READ:
      painter.setPen(Qt::NoPen);
      painter.setBrush(Qt::lightGray);
      painter.drawEllipse(sclera);
      painter.setBrush(Qt::blue);
      painter.drawEllipse(iris);
      painter.setBrush(Qt::black);
      painter.drawEllipse(pupil);
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
    tipString+=baseName(fileHeaders[filenums[i]].getFileName());
  }
  QToolTip::showText(event->globalPos(),QString::fromStdString(tipString),this);
}
