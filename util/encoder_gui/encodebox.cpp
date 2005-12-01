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

void MyDialog::createEncodeBox()
{
    encodeGroupBox = new QGroupBox(tr("Encoding Parameters"));
    encodeGroupBox->setFixedWidth(400);
    
    encode_more = new QPushButton(tr("Show"));
    encode_more->setFixedSize(60, 26);
        
    encode_box = new QGridLayout;
    encode_box2 = new QGridLayout;      
    QVBoxLayout *encode_all = new QVBoxLayout;
    encode_widget = new QWidget;
    //encode_widget2 = new QWidget;
        
    encode_widget->setVisible(false);
    //encode_widget2->setVisible(true);
    
    QLabel *label1 = new QLabel(tr("Prefered viewing distance (CPD)"));
    QLabel *label2 = new QLabel(tr("Motion compensation block width (xblen)"));
    QLabel *label3 = new QLabel(tr("Motion compensation block height (yblen)"));
    QLabel *label4 = new QLabel(tr("Motion compensation horizontal separation (xbsep)"));
    QLabel *label5 = new QLabel(tr("Motion compensation vertical separation (ybsep)"));
    QLabel *label6 = new QLabel(tr("Start frame number"));
    QLabel *label7 = new QLabel(tr("Stop frame number"));
    QLabel *label8 = new QLabel(tr("No. of ref. frames between key frames (num_L1)"));
    QLabel *label9 = new QLabel(tr("Separation between reference frames (sep_L1)"));
        
    cpd_val = new QSpinBox;
    cpd_val->setRange(MIN_CPD, MAX_CPD);
    xblen_val = new QSpinBox;
    xblen_val->setRange(MIN_LEN, MAX_LEN);
    yblen_val = new QSpinBox;
    yblen_val->setRange(MIN_LEN, MAX_LEN); 
    xbsep_val = new QSpinBox;
    xbsep_val->setRange(MIN_SEP, MAX_SEP);
    ybsep_val = new QSpinBox;
    ybsep_val->setRange(MIN_SEP, MAX_SEP);
    num_l1_val = new QSpinBox;
    num_l1_val->setRange(MIN_NUML1, MAX_NUML1);
    sep_l1_val = new QSpinBox;
    sep_l1_val->setRange(MIN_SEPL1, MAX_SEPL1);
    start_val = new QSpinBox;
    start_val->setRange(MIN_START, MAX_START);
    stop_val = new QSpinBox;
    stop_val->setRange(MIN_STOP, MAX_STOP);
    
    num_l1_val->setValue(DEF_NUML1);
    dirac_data.num_l1 = DEF_NUML1;
    sep_l1_val->setValue(DEF_SEPL1);
    dirac_data.sep_l1 = DEF_SEPL1;
    start_val->setValue(MIN_START);
    dirac_data.start = MIN_START;
    stop_val->setValue(MAX_STOP);   
    dirac_data.stop = MAX_START;
                    
    encode_box->addWidget(label1, 0, 0);
    encode_box->addWidget(cpd_val, 0, 1);
    encode_box->addWidget(label2, 1, 0);
    encode_box->addWidget(xblen_val, 1, 1);
    encode_box->addWidget(label3, 2, 0);
    encode_box->addWidget(yblen_val, 2, 1);
    encode_box->addWidget(label4, 3, 0);    
    encode_box->addWidget(xbsep_val, 3, 1);
    encode_box->addWidget(label5, 4, 0);
    encode_box->addWidget(ybsep_val, 4, 1);
    encode_box->addWidget(label8, 5, 0);
    encode_box->addWidget(num_l1_val, 5, 1);
    encode_box->addWidget(label9, 6, 0);
    encode_box->addWidget(sep_l1_val, 6, 1);
        
    encode_box2->addWidget(label6, 0, 0);           
    encode_box2->addWidget(start_val, 0, 1);           
    encode_box2->addWidget(label7, 1, 0);           
    encode_box2->addWidget(stop_val, 1, 1);    
         
    encode_widget->setLayout(encode_box);
    //encode_widget2->setLayout(encode_box2);   
    encode_all->addLayout(encode_box2); 
    encode_all->addWidget(encode_widget);
    encode_all->addWidget(encode_more);
        
    encodeGroupBox->setLayout(encode_all);
    
    connect(cpd_val, SIGNAL(valueChanged(int)),
            this, SLOT(checkEncodeBoxCPD(int)));       
    connect(xblen_val, SIGNAL(valueChanged(int)),
            this, SLOT(checkEncodeBoxXL(int)));  
    connect(yblen_val, SIGNAL(valueChanged(int)),
            this, SLOT(checkEncodeBoxYL(int)));  
    connect(xbsep_val, SIGNAL(valueChanged(int)),
            this, SLOT(checkEncodeBoxXS(int)));  
    connect(ybsep_val, SIGNAL(valueChanged(int)),
            this, SLOT(checkEncodeBoxYS(int)));  
    connect(num_l1_val, SIGNAL(valueChanged(int)),
            this, SLOT(checkEncodeBoxN1(int)));  
    connect(sep_l1_val, SIGNAL(valueChanged(int)),
            this, SLOT(checkEncodeBoxS1(int)));  
   
    connect(start_val, SIGNAL(valueChanged(int)),
            this, SLOT(checkEncodeBoxStart(int))); 
    connect(stop_val, SIGNAL(valueChanged(int)),
            this, SLOT(checkEncodeBoxStop(int)));  
  
    connect(encode_more, SIGNAL(pressed()),
            this, SLOT(showEncode()));  
}

void MyDialog::showEncode(void)
{
    if (encode_widget->isVisible() == false)
    {
        encode_widget->show();
    //encode_widget2->show();
        encode_more->setText(tr("Hide"));
        encodeGroupBox->setFixedHeight(390);
        reportGroupBox->setFixedHeight(30);
        report_widget->hide();
    }
    else 
    {
        encode_widget->hide();
    //encode_widget2->show();
        encode_more->setText(tr("Show"));
        encodeGroupBox->setFixedHeight(130);
        reportGroupBox->setFixedHeight(250);
        report_widget->show();
    } 
}

void MyDialog::checkEncodeBoxCPD(int val)
{
    dirac_data.cpd = val;
}

void MyDialog::checkEncodeBoxXL(int val)
{
    dirac_data.xblen = val;
}

void MyDialog::checkEncodeBoxYL(int val)
{
    dirac_data.yblen = val;
}

void MyDialog::checkEncodeBoxXS(int val)
{
    dirac_data.xbsep = val;
}

void MyDialog::checkEncodeBoxYS(int val)
{
    dirac_data.ybsep = val;
}

void MyDialog::checkEncodeBoxStart(int val)
{
    dirac_data.start = val;
}

void MyDialog::checkEncodeBoxStop(int val)
{
    dirac_data.stop = val;
}

void MyDialog::checkEncodeBoxN1(int val)
{
    dirac_data.num_l1 = val;
}

void MyDialog::checkEncodeBoxS1(int val)
{
    dirac_data.sep_l1 = val;
}

