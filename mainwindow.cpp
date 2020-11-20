/******************************************************/
/*                                                    */
/* mainwindow.cpp - main window                       */
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
#include "config.h"
#include "mainwindow.h"
#include "ldecimal.h"
#include "angle.h"
#include "fileio.h"
#include "brevno.h"
using namespace std;

const char unitIconNames[4][28]=
{
  ":/meter.png",":/international-foot.png",":/us-foot.png",":/indian-foot.png"
};

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent)
{
  setWindowTitle(QApplication::translate("main", "Wolkenbase"));
  setWindowIcon(QIcon(":/wolkenbase.png"));
  fileMsg=new QLabel(this);
  dotTriangleMsg=new QLabel(this);
  toleranceMsg=new QLabel(this);
  densityMsg=new QLabel(this);
  canvas=new WolkenCanvas(this);
  //configDialog=new ConfigurationDialog(this);
  msgBox=new QMessageBox(this);
  /*connect(configDialog,SIGNAL(settingsChanged(double,double,int,bool,Printer3dSize)),
	  this,SLOT(setSettings(double,double,int,bool,Printer3dSize)));
  connect(this,SIGNAL(tinSizeChanged()),canvas,SLOT(setSize()));
  connect(this,SIGNAL(lengthUnitChanged(double)),canvas,SLOT(setLengthUnit(double)));*/
  connect(this,SIGNAL(fileOpened(std::string)),canvas,SLOT(readFileHeader(std::string)));
  doneBar=new QProgressBar(this);
  busyBar=new QProgressBar(this);
  doneBar->setRange(0,16777216);
  busyBar->setRange(0,16777216);
  lpfBusyFraction=0;
  density=0;
  conversionStopped=false;
  showingResult=false;
  toolbar=new QToolBar(this);
  addToolBar(Qt::TopToolBarArea,toolbar);
  toolbar->setIconSize(QSize(32,32));
  makeActions();
  makeStatusBar();
  setCentralWidget(canvas);
  readSettings();
  canvas->show();
  show();
  timer=new QTimer(this);
  connect(timer,SIGNAL(timeout()),this,SLOT(tick()));
  connect(timer,SIGNAL(timeout()),canvas,SLOT(tick()));
  timer->start(50);
}

MainWindow::~MainWindow()
{
}

/* Rules for enabling actions:
 * You cannot load or open a file while a conversion is in progress.
 * You can load a file while loading another file (it will be queued).
 * You can load a file while a conversion is stopped, but then you cannot resume.
 * You can open a file while a conversion is stopped. If it is a final file,
 * you cannot resume; if it is a checkpoint file, you can resume.
 * You cannot clear or export while loading a file or converting.
 * You cannot start a conversion while loading a file or converting.
 * You can start a conversion after loading a file, but not after opening one.
 * You cannot stop a conversion unless one is in progress and has
 * passed the octagon stage.
 * You cannot resume a conversion unless one has been stopped or you have
 * opened a checkpoint file and no file has been loaded and the TIN
 * has not been cleared.
 */

void MainWindow::tick()
{
  double toleranceRatio;
  int tstatus=getThreadStatus();
  ThreadAction ta;
  if (!showingResult && !resultQueueEmpty())
  {
    ta=dequeueResult();
    gotResult(ta);
  }
  lpfBusyFraction=(16*lpfBusyFraction+busyFraction())/17;
  busyBar->setValue(lrint(lpfBusyFraction*16777216));
  if (tstatus==1048577*TH_WAIT+TH_ASLEEP && actionQueueEmpty())
    currentAction=0;
  writeBufLog();
}

void MainWindow::openFile()
{
  int dialogResult;
  QStringList files;
  int i;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Open LAS File"));
  fileDialog->setFileMode(QFileDialog::ExistingFiles);
  fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
  fileDialog->selectFile("");
  fileDialog->setNameFilter(tr("(*.las);;(*)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    for (i=0;i<files.size();i++)
    {
      fileName=files[i].toStdString();
      if (fileNames.length())
	fileNames+=';';
      fileNames+=baseName(fileName);
      lastFileName=fileName;
      fileOpened(fileName);
    }
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::loadFile()
{
  int dialogResult;
  QStringList files;
  int i;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Load Point Cloud File"));
  fileDialog->setFileMode(QFileDialog::ExistingFiles);
  fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
  fileDialog->selectFile("");
#ifdef Plytapus_FOUND
  fileDialog->setNameFilter(tr("(*.las);;(*.ply);;(*)"));
#else
  fileDialog->setNameFilter(tr("(*.las);;(*)"));
#endif
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    tinSizeChanged();
    for (i=0;i<files.size();i++)
    {
      fileName=files[i].toStdString();
      ta.opcode=ACT_LOAD;
      ta.filename=fileName;
      ta.param1=lengthUnit;
      enqueueAction(ta);
      if (fileNames.length())
	fileNames+=';';
      fileNames+=baseName(fileName);
      lastFileName=fileName;
    }
    fileMsg->setText(QString::fromStdString(fileNames));
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::disableMenuSplash()
/* Disable menu during splash screen, so that the user can't mess up
 * the TIN that the splash screen shows.
 */
{
  openAction->setEnabled(false);
  loadAction->setEnabled(false);
  convertAction->setEnabled(false);
  clearAction->setEnabled(false);
  stopAction->setEnabled(false);
}

void MainWindow::enableMenuSplash()
{
  openAction->setEnabled(true);
  loadAction->setEnabled(true);
  convertAction->setEnabled(false);
  clearAction->setEnabled(false);
  stopAction->setEnabled(false);
}

void MainWindow::startConversion()
{
  int dialogResult;
  QStringList files;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Convert to TIN"));
  fileDialog->setFileMode(QFileDialog::AnyFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  fileDialog->selectFile(QString::fromStdString(noExt(lastFileName)+".ptin"));
  fileDialog->setNameFilter(tr("(*.ptin)"));
  dialogResult=fileDialog->exec();
  if (dialogResult)
  {
    files=fileDialog->selectedFiles();
    saveFileName=files[0].toStdString();
    if (extension(saveFileName)==".ptin")
      saveFileName=noExt(saveFileName);
    //ta.opcode=ACT_OCTAGON;
    enqueueAction(ta);
    writtenTolerance=INFINITY;
    fileNames=baseName(saveFileName)+".ptin";
    fileMsg->setText(QString::fromStdString(fileNames));
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void MainWindow::stopConversion()
{
  setThreadCommand(TH_WAIT);
  conversionStopped=true;
}

void MainWindow::resumeConversion()
{
  if (conversionStopped)
  {
    setThreadCommand(TH_RUN);
    conversionStopped=false;
  }
}

void MainWindow::handleResult(ThreadAction ta)
/* Receives the result of reading a file. If an error happened, pops up a message.
 * The file was read by a worker thread, which put the result in a queue.
 */
{
  QString message;
  showingResult=true;
  tinSizeChanged();
  switch (ta.opcode)
  {
    case ACT_LOAD:
      convertAction->setEnabled(true);
      break;
  }
  fileMsg->setText(QString::fromStdString(fileNames));
  showingResult=false;
}

void MainWindow::aboutProgram()
{
  QString progName=tr("PerfectTIN");
#ifdef Plytapus_FOUND
  QString rajotte=tr("\nPlytapus library version %1\nCopyright %2\nSimon Rajotte and Pierre Abbat\nMIT license")
  .arg(QString::fromStdString(plytapusVersion())).arg(plytapusYear());
#else
  QString rajotte("");
#endif
  QMessageBox::about(this,tr("PerfectTIN"),
		     tr("%1\nVersion %2\nCopyright %3 Pierre Abbat\nLicense LGPL 3 or later%4")
		     .arg(progName).arg(QString(VERSION)).arg(COPY_YEAR).arg(rajotte));
}

void MainWindow::aboutQt()
{
  QMessageBox::aboutQt(this,tr("PerfectTIN"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  int result;
  writeSettings();
  /*if (conversionBusy())
  {
    msgBox->setWindowTitle(tr("PerfectTIN"));
    msgBox->setIcon(QMessageBox::Warning);
    msgBox->setText(tr("A conversion is in progress."));
    msgBox->setInformativeText(tr("Do you want to abort it and exit?"));
    msgBox->setStandardButtons(QMessageBox::Yes|QMessageBox::No);
    msgBox->setDefaultButton(QMessageBox::No);
    result=msgBox->exec();
    if (result==QMessageBox::Yes)
      event->accept();
    else
      event->ignore();
  }
  else*/
    event->accept();
}

void MainWindow::makeActions()
{
  int i;
  fileMenu=menuBar()->addMenu(tr("&File"));
  viewMenu=menuBar()->addMenu(tr("&View"));
  settingsMenu=menuBar()->addMenu(tr("&Settings"));
  helpMenu=menuBar()->addMenu(tr("&Help"));
  // File menu
  openAction=new QAction(this);
  openAction->setIcon(QIcon::fromTheme("document-open"));
  openAction->setText(tr("Open"));
  fileMenu->addAction(openAction);
  connect(openAction,SIGNAL(triggered(bool)),this,SLOT(openFile()));
  loadAction=new QAction(this);
  loadAction->setIcon(QIcon::fromTheme("document-open"));
  loadAction->setText(tr("Load"));
  fileMenu->addAction(loadAction);
  connect(loadAction,SIGNAL(triggered(bool)),this,SLOT(loadFile()));
  convertAction=new QAction(this);
  convertAction->setIcon(QIcon::fromTheme("document-save-as"));
  convertAction->setText(tr("Convert"));
  fileMenu->addAction(convertAction);
  connect(convertAction,SIGNAL(triggered(bool)),this,SLOT(startConversion()));
  clearAction=new QAction(this);
  clearAction->setIcon(QIcon::fromTheme("edit-clear"));
  clearAction->setText(tr("Clear"));
  fileMenu->addAction(clearAction);
  connect(clearAction,SIGNAL(triggered(bool)),this,SLOT(clearCloud()));
  stopAction=new QAction(this);
  stopAction->setIcon(QIcon::fromTheme("process-stop"));
  stopAction->setText(tr("Stop"));
  stopAction->setEnabled(false);
  fileMenu->addAction(stopAction);
  connect(stopAction,SIGNAL(triggered(bool)),this,SLOT(stopConversion()));
  resumeAction=new QAction(this);
  //resumeAction->setIcon(QIcon::fromTheme("edit-clear"));
  resumeAction->setText(tr("Resume"));
  resumeAction->setEnabled(false);
  fileMenu->addAction(resumeAction);
  connect(resumeAction,SIGNAL(triggered(bool)),this,SLOT(resumeConversion()));
  exitAction=new QAction(this);
  exitAction->setIcon(QIcon::fromTheme("application-exit"));
  exitAction->setText(tr("Exit"));
  fileMenu->addAction(exitAction);
  connect(exitAction,SIGNAL(triggered(bool)),this,SLOT(close()));
  // View menu
  // Settings menu
  configureAction=new QAction(this);
  configureAction->setIcon(QIcon::fromTheme("configure"));
  configureAction->setText(tr("Configure"));
  settingsMenu->addAction(configureAction);
  connect(configureAction,SIGNAL(triggered(bool)),this,SLOT(configure()));
  // Help menu
  aboutProgramAction=new QAction(this);
  aboutProgramAction->setText(tr("About PerfectTIN"));
  helpMenu->addAction(aboutProgramAction);
  connect(aboutProgramAction,SIGNAL(triggered(bool)),this,SLOT(aboutProgram()));
  aboutQtAction=new QAction(this);
  aboutQtAction->setText(tr("About Qt"));
  helpMenu->addAction(aboutQtAction);
  connect(aboutQtAction,SIGNAL(triggered(bool)),this,SLOT(aboutQt()));
  // Toolbar
  /*for (i=0;i<4;i++)
  {
    unitButtons[i]=new UnitButton(this,conversionFactors[i]);
    unitButtons[i]->setText(configDialog->tr(unitNames[i])); // lupdate warns but it works
    unitButtons[i]->setIcon(QIcon(unitIconNames[i]));
    connect(this,SIGNAL(lengthUnitChanged(double)),unitButtons[i],SLOT(setUnit(double)));
    connect(unitButtons[i],SIGNAL(triggered(bool)),unitButtons[i],SLOT(selfTriggered(bool)));
    connect(unitButtons[i],SIGNAL(unitChanged(double)),this,SLOT(setUnit(double)));
    toolbar->addAction(unitButtons[i]);
  }*/
}

void MainWindow::makeStatusBar()
{
  statusBar()->addWidget(fileMsg);
  statusBar()->addWidget(dotTriangleMsg);
  statusBar()->addWidget(toleranceMsg);
  statusBar()->addWidget(densityMsg);
  statusBar()->addWidget(doneBar);
  statusBar()->addWidget(busyBar);
  statusBar()->show();
}

void MainWindow::readSettings()
{
  QSettings settings("Bezitopo","Wolkenbase");
  resize(settings.value("size",QSize(707,500)).toSize());
  move(settings.value("pos",QPoint(0,0)).toPoint());
  numberThreads=settings.value("threads",0).toInt();
  tolerance=settings.value("tolerance",0.1).toDouble();
  lengthUnit=settings.value("lengthUnit",1).toDouble();
  exportEmpty=settings.value("exportEmpty",false).toBool();
  lengthUnitChanged(lengthUnit);
}

void MainWindow::writeSettings()
{
  QSettings settings("Bezitopo","Wolkenbase");
  settings.setValue("size",size());
  settings.setValue("pos",pos());
  settings.setValue("threads",numberThreads);
  settings.setValue("tolerance",tolerance);
  settings.setValue("lengthUnit",lengthUnit);
  settings.setValue("exportEmpty",exportEmpty);
}

void MainWindow::setSettings()
{
  /*lengthUnit=lu;
  tolerance=tol;
  numberThreads=thr;
  exportEmpty=ee;
  printer3d=pri;
  writeSettings();
  lengthUnitChanged(lengthUnit);*/
}

void MainWindow::setUnit(double lu)
{
  lengthUnit=lu;
  lengthUnitChanged(lengthUnit);
}
