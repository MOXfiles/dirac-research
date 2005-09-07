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

#include "diracdata.h"
#include "diracgui.h"
#include "tables.h"


//DiracData dirac_data;
//FILE *op_ptr;
//QListWidget *reportdoc;
        
/***********************************************/

MyDialog::MyDialog(QDialog *parent)
    : QDialog(parent)
{   
    createFilesBox();
    createQualityBox();   
    createPresetBox();
    createVideoBox();
    createEncodeBox();
    createReportBox();
           
    done = new QPushButton("Done");
    done->setFont(QFont("Sans Serif", 12, QFont::Bold));
    done->setEnabled(false);
    QPushButton *reset = new QPushButton("Reset");
    reset->setFont(QFont("Sans Serif", 12, QFont::Bold));
    QPushButton *help = new QPushButton("Help");
    help->setFont(QFont("Sans Serif", 12, QFont::Bold));
    QPushButton *quit = new QPushButton("Quit");
    quit->setFont(QFont("Sans Serif", 12, QFont::Bold));
                
    connect(quit, SIGNAL(clicked()), qApp, SLOT(quit()));
    connect(done, SIGNAL(clicked()), this, SLOT(doneValues()));
    connect(reset, SIGNAL(clicked()), this, SLOT(setReset()));
    connect(help, SIGNAL(clicked()), this, SLOT(help()));	    
	    
    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addWidget(done);
    buttons->addWidget(reset);
    buttons->addWidget(help);
    buttons->addWidget(quit);
   
    QLabel *title_l = new QLabel;
    title_l->setText("Dirac Video Coder");
    title_l->setAlignment(Qt::AlignCenter);
    title_l->setFont(QFont("Sans Serif", 14, QFont::Bold, false));
    title_l->setFixedHeight(40);
           
    QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *lay_H = new QHBoxLayout;
    QVBoxLayout *lay_leftV = new QVBoxLayout;
    QVBoxLayout *lay_rightV = new QVBoxLayout;    
    
    lay_leftV->addWidget(qualityGroupBox);
    lay_leftV->addWidget(presetGroupBox);
    lay_leftV->addWidget(videoGroupBox);
    lay_leftV->addStretch();
    lay_leftV->addLayout(buttons);
    
    lay_rightV->addWidget(filesGroupBox);
    lay_rightV->addWidget(encodeGroupBox);
    lay_rightV->addWidget(reportGroupBox);
    lay_rightV->addStretch();

    lay_H->addLayout(lay_leftV);
    lay_H->addSpacing(10);
    lay_H->addLayout(lay_rightV);
    
    layout->addWidget(title_l);
    layout->addLayout(lay_H);   
    
    setPresetValues(0);
    
    setLayout(layout);
}

void MyDialog::setReset(void)
{
    presets[0]->setChecked(true);
    done->setEnabled(false);
    input_file_text->setText("No file");
    output_file_text->setText("No file");    
    infile_valid = false;
    outfile_valid = false;
    setPresetValues(0);
    dirac_data.quality = 10;
    strcpy(dirac_data.fname_in, "");
    strcpy(dirac_data.fname_out, "");
    input_file_name->setText("Input file: <I>Nothing</I>");
    output_file_name->setText("Output files: <I>Nothing</I>");
}


void MyDialog::doneValues()
{   
    done->setEnabled(false);
        
    connect(&rep_thread, SIGNAL(RepMessage(QString)),
                this, SLOT(ReportSetMessage(QString)));
    connect(&rep_thread, SIGNAL(RepFinished()),
                this, SLOT(ReportFinished()));    
    connect(this, SIGNAL(SendDiracData(DiracData)),
                &rep_thread, SLOT(RepGetDiracData(DiracData)));
    
    emit SendDiracData(dirac_data);
    
    rep_thread.report();
    
}

void MyDialog::help()
{
    char help_txt[65536];
    
    sprintf(help_txt, "<P><B>Dirac Help</B></P>");
    sprintf(help_txt, "%s<P>Some helpful information here.</P>", help_txt);
    
    QMessageBox::about(this, tr("Dirac Help"),
            tr(help_txt));
}
