/*
    Dirac encoder GUI, for use with the Dirac video encoder 
    (http://dirac.sourceforge.net)
    Copyright (C) 2005 BBC Research and Development
    Author: David Marston

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>

#include "diracgui.h"
#include "tables.h"

/***********************************************/

void MyDialog::createReportBox()
{
    reportGroupBox = new QGroupBox(tr("Information"));
    reportGroupBox->setFixedHeight(240);
    reportGroupBox->setMinimumWidth(400);
           
    reportdoc = new QListWidget;
    
    report_vbox = new QVBoxLayout;
    report_vbox->addWidget(reportdoc);
    
    report_widget = new QWidget;    
    report_widget->setLayout(report_vbox);
    report_widget->setVisible(true);

    reportrun = new QLabel;
    reportrun->setText("Inactive"); 
    
    QVBoxLayout *report_lay = new QVBoxLayout;
    report_lay->addWidget(reportrun);
    report_lay->addWidget(report_widget);
        
    reportGroupBox->setLayout(report_lay);
}    
  
void MyDialog::ReportSetMessage(QString mess)
{
    reportrun->setText("Running");
    reportdoc->addItem(mess);
}

void MyDialog::ReportFinished(void)
{
    reportrun->setText("Finished");
    reportdoc->addItem("Finished");
    setReset();
}
