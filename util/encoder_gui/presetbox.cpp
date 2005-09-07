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

void MyDialog::createPresetBox()
{
    int i;
    
    presetGroupBox = new QGroupBox(tr("Presets"));
    presetGroupBox->setFixedHeight(60);
        
    QGridLayout *preset_box = new QGridLayout;  
       
    presets = new QRadioButton*[NUM_PRESETS];    
    for (i = 0; i < NUM_PRESETS; i++)
        presets[i] = new QRadioButton;

    for (i = 0; i < NUM_PRESETS; i++)
        presets[i]->setText(preset_names[i]);
        
    for (i = 0; i < NUM_PRESETS; i++)
        preset_box->addWidget(presets[i], 1, i);    

    connect(presets[0], SIGNAL(pressed()),
            this, SLOT(setPreset0()));
    connect(presets[1], SIGNAL(pressed()),
            this, SLOT(setPreset1()));
    connect(presets[2], SIGNAL(pressed()),
            this, SLOT(setPreset2()));
    connect(presets[3], SIGNAL(pressed()),
            this, SLOT(setPreset3()));

    presets[0]->setChecked(true);

    presetGroupBox->setLayout(preset_box);
}

void MyDialog::setPreset0(void)
{
    setPresetValues(0);
}

void MyDialog::setPreset1(void)
{
    setPresetValues(1);
}

void MyDialog::setPreset2(void)
{
    setPresetValues(2);
}

void MyDialog::setPreset3(void)
{
    setPresetValues(3);
}

void MyDialog::setPresetValues(int num)
{
    char txt[100];
    
    /* We only want to copy some of it over, not the filenames */
    dirac_data.frame_rate = dirac_preset[num].frame_rate;
    dirac_data.frame_height = dirac_preset[num].frame_height;
    dirac_data.frame_width = dirac_preset[num].frame_width;
    dirac_data.cformat = dirac_preset[num].cformat; 
    dirac_data.cpd = dirac_preset[num].cpd;
    dirac_data.xblen = dirac_preset[num].xblen;
    dirac_data.yblen = dirac_preset[num].yblen;
    dirac_data.xbsep = dirac_preset[num].xbsep;
    dirac_data.ybsep = dirac_preset[num].ybsep;
        
    width_val->setValue(dirac_preset[num].frame_width);
    height_val->setValue(dirac_preset[num].frame_height);
    sprintf(txt, "%f", dirac_preset[num].frame_rate);
    frame_rate_val->setText(txt);
    cformats->setCurrentIndex(dirac_preset[num].cformat);
    cpd_val->setValue(dirac_preset[num].cpd);
    xblen_val->setValue(dirac_preset[num].xblen);
    yblen_val->setValue(dirac_preset[num].yblen);
    xbsep_val->setValue(dirac_preset[num].xbsep);
    ybsep_val->setValue(dirac_preset[num].ybsep);
}
