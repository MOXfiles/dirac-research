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


void MyDialog::createVideoBox()
{
    int i;
    char txt[100];
    
    videoGroupBox = new QGroupBox(tr("Video Sequence Parameters"));
    videoGroupBox->setFixedHeight(180);
    
    QGridLayout *video_box = new QGridLayout;  

    videoGroupBox->setLayout(video_box);

    sprintf(txt, "Frame Width (%d-%d pixels)", MIN_WIDTH, MAX_WIDTH);
    QLabel *label1 = new QLabel(tr(txt));
    sprintf(txt, "Frame Height (%d-%d pixels)", MIN_HEIGHT, MAX_HEIGHT);
    QLabel *label2 = new QLabel(tr(txt));
    sprintf(txt, "Frame rate (%d-%d frame/s)", MIN_FPS, MAX_FPS);
    QLabel *label3 = new QLabel(tr(txt));
    QLabel *label4 = new QLabel(tr("CFormat"));
    
    width_val = new QSpinBox;
    width_val->setRange(MIN_WIDTH, MAX_WIDTH);
    height_val = new QSpinBox;
    height_val->setRange(MIN_HEIGHT, MAX_HEIGHT);
    frame_rate_val = new QLineEdit;
    //frame_rate_val->setRange(MIN_FPS, MAX_FPS);
   
    cformats = new QComboBox;
    for (i = 0; i < NUM_CFS; i++)
    {    
        cformats->insertItem(i, cformat_names[i]);
    }
        
    video_box->addWidget(label1, 0, 0);
    video_box->addWidget(width_val, 0, 1);
    video_box->addWidget(label2, 1, 0);
    video_box->addWidget(height_val, 1, 1);
    video_box->addWidget(label3, 2, 0);
    video_box->addWidget(frame_rate_val, 2, 1);
    video_box->addWidget(label4, 3, 0);
    video_box->addWidget(cformats, 3, 1);
        
    videoGroupBox->setLayout(video_box); 
    
    connect(width_val, SIGNAL(valueChanged(int)),
            this, SLOT(checkVideoBoxW(int)));       
    connect(height_val, SIGNAL(valueChanged(int)),
            this, SLOT(checkVideoBoxH(int)));  
    connect(frame_rate_val, SIGNAL(editingFinished()),
            this, SLOT(checkVideoBoxFR()));  
    connect(cformats, SIGNAL(activated(int)),
            this, SLOT(checkVideoBoxCF(int)));
}

void MyDialog::checkVideoBoxW(int val)
{
    dirac_data.frame_width = val;
}

void MyDialog::checkVideoBoxH(int val)
{
    dirac_data.frame_height = val;
}

void MyDialog::checkVideoBoxFR(void)
{
    QString str = frame_rate_val->text();
    float val;
    char txt[100];
    
    val = str.toFloat();
    if (val < MIN_FPS)
        val = MIN_FPS;
    if (val > MAX_FPS)
        val = MAX_FPS;
    sprintf(txt, "%f", val);
    frame_rate_val->setText(txt);
    dirac_data.frame_rate = val; 
}

void MyDialog::checkVideoBoxCF(int val)
{
    dirac_data.cformat = val;
}
