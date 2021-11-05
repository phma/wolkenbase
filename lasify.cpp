/******************************************************/
/*                                                    */
/* lasify.cpp - GUI main program to convert to LAS    */
/*                                                    */
/******************************************************/
/* Copyright 2020-2021 Pierre Abbat.
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
#include "threads.h"
#include <QtGui>
#include <QtWidgets>
#include <QTranslator>
#include <iostream>
#include "point.h"
#include "config.h"
#include "octree.h"
#include "angle.h"
#include "relprime.h"
#include "lasifywindow.h"
#include "brevno.h"

using namespace std;

int main(int argc, char *argv[])
{
  int exitStatus;
  QApplication app(argc, argv);
  QTranslator translator,qtTranslator;
  int nthreads;
  string exeFileName=argv[0],exeDir;
  size_t slashPos;
  slashPos=exeFileName.find_last_of("/\\");
  if (slashPos<exeFileName.length())
    exeDir=exeFileName.substr(0,slashPos);
  else
    exeDir=".";
  if (qtTranslator.load(QLocale(),QLatin1String("qt"),QLatin1String("_"),
                        QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
    app.installTranslator(&qtTranslator);
  if (translator.load(QLocale(),QLatin1String("wolkenbase"),
                      QLatin1String("_"),QString::fromStdString(exeDir)))
  {
    //cout<<"Translations found in executable's directory"<<endl;
    app.installTranslator(&translator);
  }
  else if (translator.load(QLocale(),QLatin1String("wolkenbase"),
                      QLatin1String("_"),QLatin1String(SHARE_DIR)))
  {
    //cout<<"Translations found in share directory"<<endl;
    app.installTranslator(&translator);
  }
  LasifyWindow window;
  nthreads=window.getNumberThreads();
  fillTanTables();
  if (nthreads<1)
    nthreads=thread::hardware_concurrency();
  if (nthreads<1)
    nthreads=2;
  octStore.open("store.oct",nthreads+relprime(nthreads));
  octStore.resize(8*nthreads+1);
  startThreads(nthreads);
  window.show();
  exitStatus=app.exec();
  waitForThreads(TH_STOP);
  writeBufLog();
  joinThreads();
  return exitStatus;
}
