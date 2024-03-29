/******************************************************/
/*                                                    */
/* configdialog.cpp - configuration dialog            */
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
#include <cmath>
#include "configdialog.h"
#include "ldecimal.h"
#include "threads.h"

using namespace std;

const double conversionFactors[4]={1,0.3048,12e2/3937,0.3047996};
const char unitNames[4][12]=
{
  QT_TRANSLATE_NOOP("ConfigurationDialog","Meter"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","Int'l foot"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","US foot"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","Indian foot")
};
const int ppf[]=
{
  1000000,2000000,5000000,10000000,20000000,50000000,
  100000000,200000000,500000000,1000000000,2000000000,0
};
const char ppfStr[12][6]=
{
  QT_TRANSLATE_NOOP("ConfigurationDialog","1 M"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","2 M"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","5 M"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","10 M"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","20 M"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","50 M"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","100 M"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","200 M"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","500 M"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","1 G"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","2 G"),
  QT_TRANSLATE_NOOP("ConfigurationDialog","nolmt")
};
const double ts[]={0.1,0.2,0.5,1,2,5,10};
const char tsStr[7][8]=
{
  "100 mm","200 mm","500 mm","1 m","2 m","5 m","10 m"
};
const double minsm[11]=
{
  0.01,0.016,0.025,0.04,0.063,0.1,0.16,0.25,0.4,0.63,1
};
const char minsmStr[11][7]=
{
  "10 mm","16 mm","25 mm","40 mm","63 mm","100 mm","160 mm","250 mm","400 mm","630 mm","1 m"
};
const double maxsl[9]=
{
  0.4663076581549986,
  M_SQRT_1_3,
  0.7002075382097097,
  0.8390996311772799,
  1,
  1.19175359259421,
  1.4281480067421144,
  M_SQRT_3,
  2.1445069205095586
};
const char maxslStr[9][12]=
{
  "0.47 (25°)","0.58 (30°)","0.70 (35°)",
  "0.84 (40°)","1 (45°)","1.2 (50°)",
  "1.4 (55°)","1.7 (60°)","2.1 (65°)"
};
const double thick[8]=
{
  0,0.001,0.002,0.005,0.01,0.02,0.05,0.1
};
const char thickStr[8][7]=
{
  "0 mm","1 mm","2 mm","5 mm","10 mm","20 mm","50 mm","100 mm"
};

/* Minimum smoothness is what the config dialog calls minimum paraboloid size.
 * It is the size of the smallest hump of ground that the algorithm will call
 * ground, even with an infinitely dense point cloud.
 */

GeneralTab::GeneralTab(QWidget *parent):QWidget(parent)
{
  lengthUnitLabel=new QLabel(this);
  threadLabel=new QLabel(this);
  threadDefault=new QLabel(this);
  pointsPerFileLabel=new QLabel(this);
  lengthUnitBox=new QComboBox(this);
  pointsPerFileBox=new QComboBox(this);
  threadInput=new QLineEdit(this);
  gridLayout=new QGridLayout(this);
  separateClassesCheck=new QCheckBox(tr("Separate classes"),this);
  setLayout(gridLayout);
  gridLayout->addWidget(lengthUnitLabel,0,0);
  gridLayout->addWidget(lengthUnitBox,0,1);
  gridLayout->addWidget(threadLabel,1,0);
  gridLayout->addWidget(threadInput,1,1);
  gridLayout->addWidget(threadDefault,1,2);
  gridLayout->addWidget(pointsPerFileLabel,2,0);
  gridLayout->addWidget(pointsPerFileBox,2,1);
  gridLayout->addWidget(separateClassesCheck,3,1);
}

ClassifyTab::ClassifyTab(QWidget *parent):QWidget(parent)
{
  tileSizeLabel=new QLabel(this);
  minimumSmoothnessLabel=new QLabel(this);
  maximumSlopeLabel=new QLabel(this);
  thicknessLabel=new QLabel(this);
  tileSizeBox=new QComboBox(this);
  minimumSmoothnessBox=new QComboBox(this);
  maximumSlopeBox=new QComboBox(this);
  thicknessBox=new QComboBox(this);
  gridLayout=new QGridLayout(this);
  gridLayout->addWidget(tileSizeLabel,0,0);
  gridLayout->addWidget(tileSizeBox,0,1);
  gridLayout->addWidget(minimumSmoothnessLabel,1,0);
  gridLayout->addWidget(minimumSmoothnessBox,1,1);
  gridLayout->addWidget(maximumSlopeLabel,2,0);
  gridLayout->addWidget(maximumSlopeBox,2,1);
  gridLayout->addWidget(thicknessLabel,3,0);
  gridLayout->addWidget(thicknessBox,3,1);
}

ConfigurationDialog::ConfigurationDialog(QWidget *parent):QDialog(parent)
{
  boxLayout=new QVBoxLayout(this);
  tabWidget=new QTabWidget(this);
  general=new GeneralTab(this);
  classify=new ClassifyTab(this);
  buttonBox=new QDialogButtonBox(this);
  okButton=new QPushButton(tr("OK"),this);
  cancelButton=new QPushButton(tr("Cancel"),this);
  buttonBox->addButton(okButton,QDialogButtonBox::AcceptRole);
  buttonBox->addButton(cancelButton,QDialogButtonBox::RejectRole);
  boxLayout->addWidget(tabWidget);
  boxLayout->addWidget(buttonBox);
  tabWidget->addTab(general,tr("General"));
  tabWidget->addTab(classify,tr("Classify"));
  connect(okButton,SIGNAL(clicked()),this,SLOT(accept()));
  connect(cancelButton,SIGNAL(clicked()),this,SLOT(reject()));
}

void ConfigurationDialog::set(double lengthUnit,int threads,int pointsPerFile,bool separateClasses,double tileSize,double maximumSlope,double thickness,double minimumSmoothness)
{
  int i;
  general->lengthUnitBox->clear();
  general->lengthUnitLabel->setText(tr("Length unit"));
  general->threadLabel->setText(tr("Threads:"));
  general->pointsPerFileLabel->setText(tr("Points per file:"));
  general->threadDefault->setText(tr("default is %n","",thread::hardware_concurrency()));
  classify->tileSizeLabel->setText(tr("Tile size"));
  classify->minimumSmoothnessLabel->setText(tr("Minimum smoothness"));
  classify->maximumSlopeLabel->setText(tr("Maximum slope"));
  classify->thicknessLabel->setText(tr("Thickness"));
  for (i=0;i<sizeof(conversionFactors)/sizeof(conversionFactors[1]);i++)
  {
    general->lengthUnitBox->addItem(tr(unitNames[i]));
  }
  for (i=0;i<sizeof(conversionFactors)/sizeof(conversionFactors[1]);i++)
  {
    if (lengthUnit==conversionFactors[i])
      general->lengthUnitBox->setCurrentIndex(i);
  }
  for (i=0;i<sizeof(ppf)/sizeof(ppf[1]);i++)
  {
    general->pointsPerFileBox->addItem(tr(ppfStr[i]));
  }
  for (i=0;i<sizeof(ppf)/sizeof(ppf[1]);i++)
  {
    if (pointsPerFile==ppf[i])
      general->pointsPerFileBox->setCurrentIndex(i);
  }
  for (i=0;i<sizeof(ts)/sizeof(ts[1]);i++)
  {
    classify->tileSizeBox->addItem(QString::fromStdString(tsStr[i]));
  }
  for (i=0;i<sizeof(ts)/sizeof(ts[1]);i++)
  {
    if (tileSize==ts[i])
      classify->tileSizeBox->setCurrentIndex(i);
  }
  for (i=0;i<sizeof(minsm)/sizeof(minsm[1]);i++)
  {
    classify->minimumSmoothnessBox->addItem(QString::fromStdString(minsmStr[i]));
  }
  for (i=0;i<sizeof(minsm)/sizeof(minsm[1]);i++)
  {
    if (minimumSmoothness==minsm[i])
      classify->minimumSmoothnessBox->setCurrentIndex(i);
  }
  for (i=0;i<sizeof(maxsl)/sizeof(maxsl[1]);i++)
  {
    classify->maximumSlopeBox->addItem(QString::fromStdString(maxslStr[i]));
  }
  for (i=0;i<sizeof(maxsl)/sizeof(maxsl[1]);i++)
  {
    if (maximumSlope==maxsl[i])
      classify->maximumSlopeBox->setCurrentIndex(i);
  }
  for (i=0;i<sizeof(thick)/sizeof(thick[1]);i++)
  {
    classify->thicknessBox->addItem(QString::fromStdString(thickStr[i]));
  }
  for (i=0;i<sizeof(thick)/sizeof(thick[1]);i++)
  {
    if (thickness==thick[i])
      classify->thicknessBox->setCurrentIndex(i);
  }
  general->threadInput->setText(QString::number(threads));
  general->separateClassesCheck->setCheckState(separateClasses?Qt::Checked:Qt::Unchecked);
}

void ConfigurationDialog::checkValid()
{
}

void ConfigurationDialog::accept()
{
  settingsChanged(conversionFactors[general->lengthUnitBox->currentIndex()],
		  general->threadInput->text().toInt(),
		  ppf[general->pointsPerFileBox->currentIndex()],
		  general->separateClassesCheck->checkState()>0,
		  ts[classify->tileSizeBox->currentIndex()],
		  maxsl[classify->maximumSlopeBox->currentIndex()],
		  thick[classify->thicknessBox->currentIndex()],
		  minsm[classify->minimumSmoothnessBox->currentIndex()]);
  QDialog::accept();
}
