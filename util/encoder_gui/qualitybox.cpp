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

void MyDialog::createQualityBox()
{
    char txt[100];
    
    qualityGroupBox = new QGroupBox(tr("Quality (QF)"));
    qualityGroupBox->setFixedHeight(60);
    
    QGridLayout *quality_box = new QGridLayout;  

    sprintf(txt, "Quality value (%d-%d)", MIN_QUALITY, MAX_QUALITY);
    QLabel *label = new QLabel(tr(txt));
    
    QSpinBox *quality_val = new QSpinBox;
    quality_val->setRange(MIN_QUALITY, MAX_QUALITY);
    
    quality_box->addWidget(label, 0, 0);
    quality_box->addWidget(quality_val, 0, 1);
    
    qualityGroupBox->setLayout(quality_box); 
    
    quality_val->setValue(DEF_QUALITY);
    dirac_data.quality = DEF_QUALITY;
    
    connect(quality_val, SIGNAL(valueChanged(int)),
            this, SLOT(checkQualityBox(int))); 
}

void MyDialog::checkQualityBox(int val)
{
    dirac_data.quality = val;   
}
