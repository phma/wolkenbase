/******************************************************/
/*                                                    */
/* mainwindow.h - main window                         */
/*                                                    */
/******************************************************/
/* Copyright 2020,2021 Pierre Abbat.
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
#include <QMainWindow>
#include <QTimer>
#include <QtWidgets>
#include <QPixmap>
#include <string>
#include <array>
#include "configdialog.h"
#include "unitbutton.h"
#include "wolkencanvas.h"

class MainWindow: public QMainWindow
{
  Q_OBJECT
public:
  MainWindow(QWidget *parent=0);
  ~MainWindow();
  void makeActions();
  void makeStatusBar();
  void readSettings();
  void writeSettings();
  int getNumberThreads()
  {
    return numberThreads;
  }
  bool conversionBusy();
signals:
  void tinSizeChanged();
  void lengthUnitChanged(double unit);
  void colorSchemeChanged(int scheme);
  void noCloudArea();
  void gotResult(ThreadAction ta);
  void fileOpened(std::string name);
  void allPointsCounted();
public slots:
  void tick();
  void setSettings(double lu,int thr,int ppf,bool sc,double ts,double maxsl,double minhs);
  void setUnit(double lu);
  void openFile();
  void disableMenuSplash();
  void enableMenuSplash();
  void stopConversion();
  void resumeConversion();
  void readFileProgress(size_t sofar,size_t total);
  void configure();
  void handleResult(ThreadAction ta);
  void aboutProgram();
  void aboutQt();
protected:
  void closeEvent(QCloseEvent *event) override;
private:
  int threadsCountedPoints;
  double lastTolerance,lastStageTolerance,writtenTolerance,lastDensity,rmsadj;
  int numberThreads;
  int lastState; // state is in WolkenCanvas
  size_t readFileSoFar,readFileTotal;
  bool conversionStopped,showingResult;
  double density,lengthUnit;
  double lpfBusyFraction;
  std::string fileNames,saveFileName,lastFileName;
  QTimer *timer;
  QFileDialog *fileDialog;
  QMessageBox *msgBox;
  QToolBar *toolbar;
  ConfigurationDialog *configDialog;
  QMenu *fileMenu,*viewMenu,*settingsMenu,*helpMenu,*colorMenu;
  QLabel *fileMsg,*dotTriangleMsg,*memoryMsg,*densityMsg;
  QProgressBar *doneBar,*busyBar;
  QAction *openAction,*loadAction,*processAction,*clearAction;
  QAction *exportAction,*stopAction,*resumeAction,*exitAction;
  QAction *configureAction;
  QAction *aboutProgramAction,*aboutQtAction;
  UnitButton *unitButtons[4];
  WolkenCanvas *canvas;
};
