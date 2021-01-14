/******************************************************/
/*                                                    */
/* configdialog.cpp - configuration dialog            */
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
  gridLayout->addWidget(lengthUnitLabel,1,0);
  gridLayout->addWidget(lengthUnitBox,1,1);
  gridLayout->addWidget(threadLabel,2,0);
  gridLayout->addWidget(threadInput,2,1);
  gridLayout->addWidget(threadDefault,2,2);
  gridLayout->addWidget(pointsPerFileLabel,3,0);
  gridLayout->addWidget(pointsPerFileBox,3,1);
  gridLayout->addWidget(separateClassesCheck,4,1);
}

ConfigurationDialog::ConfigurationDialog(QWidget *parent):QDialog(parent)
{
  boxLayout=new QVBoxLayout(this);
  tabWidget=new QTabWidget(this);
  general=new GeneralTab(this);
  buttonBox=new QDialogButtonBox(this);
  okButton=new QPushButton(tr("OK"),this);
  cancelButton=new QPushButton(tr("Cancel"),this);
  buttonBox->addButton(okButton,QDialogButtonBox::AcceptRole);
  buttonBox->addButton(cancelButton,QDialogButtonBox::RejectRole);
  boxLayout->addWidget(tabWidget);
  boxLayout->addWidget(buttonBox);
  tabWidget->addTab(general,tr("General"));
  connect(okButton,SIGNAL(clicked()),this,SLOT(accept()));
  connect(cancelButton,SIGNAL(clicked()),this,SLOT(reject()));
}

void ConfigurationDialog::set(double lengthUnit,int threads,int pointsPerFile,bool separateClasses)
{
  int i;
  general->lengthUnitBox->clear();
  general->lengthUnitLabel->setText(tr("Length unit"));
  general->threadLabel->setText(tr("Threads:"));
  general->threadDefault->setText(tr("default is %n","",thread::hardware_concurrency()));
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
		  general->separateClassesCheck->checkState()>0);
  QDialog::accept();
}
