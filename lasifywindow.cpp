/******************************************************/
/*                                                    */
/* lasifywindow.cpp - main window of lasify           */
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
#include "config.h"
#include "lasifywindow.h"
#include "ldecimal.h"
#include "angle.h"
#include "cloud.h"
#include "fileio.h"
#include "brevno.h"
#include "octree.h"
#include "freeram.h"
#include "scan.h"
using namespace std;

const char unitIconNames[4][28]=
{
  ":/meter.png",":/international-foot.png",":/us-foot.png",":/indian-foot.png"
};

const char prefixes[]="yzafpnum kMGTPEZY"; // u should be µ but that's two bytes

string threePrefix(double x)
// Converts x to three digits with a prefix.
{
  int e=0;
  int sign=1;
  char buf[8];
  string ret;
  if (x<0)
  {
    x=-x;
    sign=-1;
  }
  while (x>0 && x<1)
  {
    x*=1000;
    e--;
  }
  while (x>=1000 && x<INFINITY)
  {
    x/=1000;
    e++;
  }
  if (x>=100)
    sprintf(buf,"%3.0f",x);
  else if (x>=10)
    sprintf(buf,"%3.1f",x);
  else
    sprintf(buf,"%3.2f",x);
  ret=buf;
  if (sign<0)
    ret='-'+ret;
  switch (prefixes[e+8])
  {
    case 'u':
      ret+=" µ";
      break;
    case ' ':
      ret+=' ';
      break;
    default:
      ret=ret+' '+prefixes[e+8];
  }
  return ret;
}

LasifyWindow::LasifyWindow(QWidget *parent):QMainWindow(parent)
{
  setWindowTitle(QApplication::translate("main", "LASify"));
  setWindowIcon(QIcon(":/wolkenbase.png"));
  fileMsg=new QLabel(this);
  dotTriangleMsg=new QLabel(this);
  memoryMsg=new QLabel(this);
  densityMsg=new QLabel(this);
  canvas=new WolkenCanvas(this);
  configDialog=new ConfigurationDialog(this);
  msgBox=new QMessageBox(this);
  connect(configDialog,SIGNAL(settingsChanged(double,int,int,bool,double,double,double)),
	  this,SLOT(setSettings(double,int,int,bool,double,double,double)));
  connect(this,SIGNAL(tinSizeChanged()),canvas,SLOT(setSize()));
  connect(this,SIGNAL(lengthUnitChanged(double)),canvas,SLOT(setLengthUnit(double)));
  connect(this,SIGNAL(fileOpened(std::string)),canvas,SLOT(readFileHeader(std::string)));
  connect(this,SIGNAL(allPointsCounted()),canvas,SLOT(writeFile()));
  connect(canvas,SIGNAL(readFileProgress(size_t,size_t)),this,SLOT(readFileProgress(size_t,size_t)));
  connect(this,SIGNAL(gotResult(ThreadAction)),this,SLOT(handleResult(ThreadAction)));
  doneBar=new QProgressBar(this);
  busyBar=new QProgressBar(this);
  doneBar->setRange(0,16777216);
  busyBar->setRange(0,16777216);
  lpfBusyFraction=0;
  density=0;
  threadsCountedPoints=0;
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

LasifyWindow::~LasifyWindow()
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

void LasifyWindow::tick()
{
  double toleranceRatio;
  int tstatus=getThreadStatus();
  ThreadAction ta;
  if (!showingResult && !resultQueueEmpty())
  {
    ta=dequeueResult();
    gotResult(ta);
  }
  dotTriangleMsg->setText(tr("%n points","",cloud.size()));
  memoryMsg->setText(QString::fromStdString(threePrefix(freeRam())+'B'));
  lpfBusyFraction=(16*lpfBusyFraction+busyFraction())/17;
  busyBar->setValue(lrint(lpfBusyFraction*16777216));
  if ((tstatus&0x3ffbfeff)==1048577*TH_READ && readFileTotal>0)
    doneBar->setValue(lrint((double)readFileSoFar/readFileTotal*16777216));
  if ((tstatus&0x3ffbfeff)==1048577*TH_SCAN || (tstatus&0x3ffbfeff)==1048577*TH_SPLIT)
    doneBar->setValue(lrint(snake.progress()*16777216));
  if (tstatus==1048577*TH_PAUSE+TH_ASLEEP && actionQueueEmpty())
    currentAction=0;
  writeBufLog();
}

void LasifyWindow::openFile()
{
  int dialogResult;
  QStringList files;
  int i;
  string fileName;
  ThreadAction ta;
  fileDialog=new QFileDialog(this);
  fileDialog->setWindowTitle(tr("Open Point Cloud"));
  fileDialog->setFileMode(QFileDialog::ExistingFile);
  fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
  fileDialog->selectFile("");
#ifdef Plytapus_FOUND
  fileDialog->setNameFilter(tr("(*.ply);;(*.xyz);;(*)"));
#else
  fileDialog->setNameFilter(tr("(*.xyz);;(*)"));
#endif
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
      ta.filename=fileName;
      canvas->inFileHeaders.resize(1);
      canvas->inFileHeaders[0].openFake(fileName);
      ta.hdr=&canvas->inFileHeaders[0];
      ta.opcode=ACT_LOAD;
      enqueueAction(ta);
    }
  }
  delete fileDialog;
  fileDialog=nullptr;
}

void LasifyWindow::disableMenuSplash()
/* Disable menu during splash screen, so that the user can't mess up
 * the TIN that the splash screen shows.
 */
{
  openAction->setEnabled(false);
  loadAction->setEnabled(false);
  clearAction->setEnabled(false);
  stopAction->setEnabled(false);
}

void LasifyWindow::enableMenuSplash()
{
  openAction->setEnabled(true);
  loadAction->setEnabled(true);
  clearAction->setEnabled(false);
  stopAction->setEnabled(false);
}

void LasifyWindow::stopConversion()
{
  setThreadCommand(TH_WAIT);
  conversionStopped=true;
}

void LasifyWindow::resumeConversion()
{
  if (conversionStopped)
  {
    setThreadCommand(TH_SPLIT);
    conversionStopped=false;
  }
}

void LasifyWindow::readFileProgress(size_t sofar,size_t total)
{
  readFileSoFar=sofar;
  readFileTotal=total;
}

void LasifyWindow::configure()
{
  configDialog->set(lengthUnit,numberThreads,cloudOutput.pointsPerFile,cloudOutput.separateClasses,canvas->tileSize,maxSlope,minHyperboloidSize);
  configDialog->open();
}

void LasifyWindow::handleResult(ThreadAction ta)
/* Receives the result of counting classified points.
 */
{
  QString message;
  showingResult=true;
  switch (ta.opcode)
  {
    case ACT_COUNT:
      if (++threadsCountedPoints==nThreads())
      {
	allPointsCounted();
	threadsCountedPoints=0;
      }
      break;
    case ACT_LOAD:
      cout<<"Finished loading cloud\n";
      tinSizeChanged();
      break;
  }
  showingResult=false;
}

void LasifyWindow::aboutProgram()
{
  QString progName=tr("Wolkenbase");
  QMessageBox::about(this,tr("Wolkenbase"),
		     tr("%1\nVersion %2\nCopyright %3 Pierre Abbat\nLicense GPL 3 or later")
		     .arg(progName).arg(QString(VERSION)).arg(COPY_YEAR));
}

void LasifyWindow::aboutQt()
{
  QMessageBox::aboutQt(this,tr("Wolkenbase"));
}

void LasifyWindow::closeEvent(QCloseEvent *event)
{
  int result;
  writeSettings();
    event->accept();
}

void LasifyWindow::makeActions()
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
  clearAction=new QAction(this);
  clearAction->setIcon(QIcon::fromTheme("edit-clear"));
  clearAction->setText(tr("Clear"));
  fileMenu->addAction(clearAction);
  connect(clearAction,SIGNAL(triggered(bool)),canvas,SLOT(clearCloud()));
  processAction=new QAction(this);
  //procesAction->setIcon(QIcon::fromTheme("edit-clear"));
  processAction->setText(tr("Process"));
  fileMenu->addAction(processAction);
  connect(processAction,SIGNAL(triggered(bool)),canvas,SLOT(startProcess()));
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
  aboutProgramAction->setText(tr("About Wolkenbase"));
  helpMenu->addAction(aboutProgramAction);
  connect(aboutProgramAction,SIGNAL(triggered(bool)),this,SLOT(aboutProgram()));
  aboutQtAction=new QAction(this);
  aboutQtAction->setText(tr("About Qt"));
  helpMenu->addAction(aboutQtAction);
  connect(aboutQtAction,SIGNAL(triggered(bool)),this,SLOT(aboutQt()));
  // Toolbar
  for (i=0;i<4;i++)
  {
    unitButtons[i]=new UnitButton(this,conversionFactors[i]);
    unitButtons[i]->setText(configDialog->tr(unitNames[i])); // lupdate warns but it works
    unitButtons[i]->setIcon(QIcon(unitIconNames[i]));
    connect(this,SIGNAL(lengthUnitChanged(double)),unitButtons[i],SLOT(setUnit(double)));
    connect(unitButtons[i],SIGNAL(triggered(bool)),unitButtons[i],SLOT(selfTriggered(bool)));
    connect(unitButtons[i],SIGNAL(unitChanged(double)),this,SLOT(setUnit(double)));
    toolbar->addAction(unitButtons[i]);
  }
}

void LasifyWindow::makeStatusBar()
{
  statusBar()->addWidget(fileMsg);
  statusBar()->addWidget(dotTriangleMsg);
  statusBar()->addWidget(memoryMsg);
  statusBar()->addWidget(densityMsg);
  statusBar()->addWidget(doneBar);
  statusBar()->addWidget(busyBar);
  statusBar()->show();
}

void LasifyWindow::readSettings()
{
  QSettings settings("Bezitopo","Wolkenbase");
  resize(settings.value("size",QSize(707,500)).toSize());
  move(settings.value("pos",QPoint(0,0)).toPoint());
  numberThreads=settings.value("threads",0).toInt();
  lengthUnit=settings.value("lengthUnit",1).toDouble();
  cloudOutput.pointsPerFile=settings.value("pointsPerFile",0).toInt();
  cloudOutput.separateClasses=settings.value("separateClasses",true).toBool();
  canvas->tileSize=settings.value("tileSize",1).toDouble();
  maxSlope=settings.value("maxSlope",1).toDouble();
  minHyperboloidSize=settings.value("minimumHyperboloidSize",0.1).toDouble();
  lengthUnitChanged(lengthUnit);
}

void LasifyWindow::writeSettings()
{
  QSettings settings("Bezitopo","Wolkenbase");
  settings.setValue("size",size());
  settings.setValue("pos",pos());
  settings.setValue("threads",numberThreads);
  settings.setValue("lengthUnit",lengthUnit);
  settings.setValue("pointsPerFile",cloudOutput.pointsPerFile);
  settings.setValue("separateClasses",cloudOutput.separateClasses);
  settings.setValue("tileSize",canvas->tileSize);
  settings.setValue("maxSlope",maxSlope);
  settings.setValue("minimumHyperboloidSize",minHyperboloidSize);
}

void LasifyWindow::setSettings(double lu,int thr,int ppf,bool sc,double ts,double maxsl,double minhs)
{
  lengthUnit=lu;
  numberThreads=thr;
  cloudOutput.pointsPerFile=ppf;
  cloudOutput.separateClasses=sc;
  canvas->tileSize=ts;
  maxSlope=maxsl;
  minHyperboloidSize=minhs;
  writeSettings();
  lengthUnitChanged(lengthUnit);
}

void LasifyWindow::setUnit(double lu)
{
  lengthUnit=lu;
  lengthUnitChanged(lengthUnit);
}
