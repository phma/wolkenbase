/******************************************************/
/*                                                    */
/* mainwindow.h - main window                         */
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
#include "threads.h"
#include <QMainWindow>
#include <QTimer>
#include <QtWidgets>
#include <QPixmap>
#include <string>
#include <array>
//#include "configdialog.h"
//#include "unitbutton.h"
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
public slots:
  void tick();
  void setSettings();
  void setUnit(double lu);
  void openFile();
  void loadFile();
  void disableMenuSplash();
  void enableMenuSplash();
  void startConversion();
  void stopConversion();
  void resumeConversion();
  void handleResult(ThreadAction ta);
  void aboutProgram();
  void aboutQt();
protected:
  void closeEvent(QCloseEvent *event) override;
private:
  int lastNumDots,lastNumTriangles,lastNumEdges;
  double lastTolerance,lastStageTolerance,writtenTolerance,lastDensity,rmsadj;
  int numberThreads;
  int lastState; // state is in TinCanvas
  bool conversionStopped,showingResult,exportEmpty;
  double tolerance,density,lengthUnit;
  double lpfBusyFraction;
  std::string fileNames,saveFileName,lastFileName;
  QTimer *timer;
  QFileDialog *fileDialog;
  QMessageBox *msgBox;
  QToolBar *toolbar;
  //ConfigurationDialog *configDialog;
  QMenu *fileMenu,*viewMenu,*settingsMenu,*helpMenu,*exportMenu,*colorMenu;
  QLabel *fileMsg,*dotTriangleMsg,*toleranceMsg,*densityMsg;
  QProgressBar *doneBar,*busyBar;
  QAction *openAction,*loadAction,*convertAction,*clearAction;
  QAction *exportAction,*stopAction,*resumeAction,*exitAction;
  QAction *exportDxfTxtAction,*exportDxfBinAction,*exportTinTxtAction;
  QAction *exportCarlsonTinAction,*exportLandXmlAction;
  QAction *exportPlyTxtAction,*exportPlyBinAction;
  QAction *exportStlTxtAction,*exportStlBinAction;
  QAction *configureAction;
  QAction *aboutProgramAction,*aboutQtAction;
  //UnitButton *unitButtons[4];
  WolkenCanvas *canvas;
};
