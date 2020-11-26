/******************************************************/
/*                                                    */
/* wolkencanvas.h - canvas for drawing point cloud    */
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
#ifndef WOLKENCANVAS_H
#define WOLKENCANVAS_H
#include <QWidget>
#include <deque>
#include "lissajous.h"
#include "point.h"
#include "las.h"

#define SPLASH_TIME 60

class WolkenCanvas: public QWidget
{
  Q_OBJECT
public:
  WolkenCanvas(QWidget *parent=0);
  QPointF worldToWindow(xy pnt);
  xy windowToWorld(QPointF pnt);
  int state;
signals:
  void splashScreenStarted();
  void splashScreenFinished();
public slots:
  void sizeToFit();
  void setSize();
  void readFileHeader(std::string name);
  void setLengthUnit(double unit);
  void setScalePos();
  void tick(); // 50 ms
protected:
  void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
  void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
private:
  QPixmap frameBuffer;
  Lissajous lis;
  xy windowCenter,worldCenter;
  double scale;
  double lengthUnit;
  double maxScaleSize,scaleSize;
  std::deque<LasHeader> fileHeaders;
  xy ballPos;
  xy leftScaleEnd,rightScaleEnd,scaleEnd;
  int penPos;
  int fileCountdown;
  int lastOpcount;
  int ballAngle,dartAngle;
  int lastntri;
  int splashScreenTime; // in ticks
  std::vector<int> fileHitTest(xy pnt);
};
#endif
