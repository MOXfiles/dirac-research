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

void MyDialog::createFilesBox()
{
    filesGroupBox = new QGroupBox(tr("Input and Output Files"));
    filesGroupBox->setFixedHeight(140);
       
    QPushButton *input_file = new QPushButton;
    input_file->setText("Open Input File");
    QPushButton *output_file = new QPushButton;
    output_file->setText("Set Output File");
    input_file_text = new QLineEdit;
    input_file_text->setText("No file");
    output_file_text = new QLineEdit;
    output_file_text->setText("No file");
    input_file_name = new QLabel;
    input_file_name->setText("Input file: <I>Nothing</I>");
    output_file_name = new QLabel;
    output_file_name->setText("Output files: <I>Nothing</I>");
    
    infile_valid = false;
    outfile_valid = false;
    
    QGridLayout *fileButtons = new QGridLayout;
    fileButtons->addWidget(input_file_text, 0, 1);
    fileButtons->addWidget(input_file, 0, 0);
    fileButtons->addWidget(output_file_text, 2, 1);
    fileButtons->addWidget(output_file, 2, 0);
    fileButtons->addWidget(input_file_name, 1, 0, 1, 2);
    fileButtons->addWidget(output_file_name, 3, 0, 1, 2);
    
    filesGroupBox->setLayout(fileButtons);
    
    connect(input_file, SIGNAL(clicked()),
            this, SLOT(openInputFile()));    
    connect(output_file, SIGNAL(clicked()),
            this, SLOT(openOutputFile()));
    connect(input_file_text, SIGNAL(returnPressed()),
            this, SLOT(textInputFile()));
    connect(output_file_text, SIGNAL(returnPressed()),
            this, SLOT(textOutputFile()));
}    
  
void MyDialog::textInputFile()
{
    useInputFile(input_file_text->text());
}

void MyDialog::textOutputFile()
{
    useOutputFile(output_file_text->text());
}

void MyDialog::openInputFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                    tr("Open File"), QDir::currentPath());
    useInputFile(fileName);
}

void MyDialog::useInputFile(QString fileName)
{
    char extn[6];
    bool dot;
    char txt[500];
    
    if (!fileName.isEmpty())
    {
        input_file_text->setText(fileName);	
        strcpy(dirac_data.fname_in, fileName.toAscii());
	infile_valid = true;

	dot = checkExtn(dirac_data.fname_in, extn);
	
	if (strcmp(extn, ".yuv") == 0)
	{
	    int i = strlen(dirac_data.fname_in);
	    dirac_data.fname_in[i - 4] = 0;
	}

	sprintf(txt, "Input file: %s.yuv", basename(dirac_data.fname_in)); 
	input_file_name->setText(txt);
    }
    else
    {
        QMessageBox::information(this, tr("Dirac"),
                                     tr("Cannot open %1.").arg(fileName));
    }
    
    if (infile_valid && outfile_valid)
        done->setEnabled(true);
}

void MyDialog::openOutputFile()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                    tr("Open File"), QDir::currentPath());
    useOutputFile(fileName);
}

void MyDialog::useOutputFile(QString fileName)
{
    char extn[6];
    bool dot;
    char txt[500];
    
    if (!fileName.isEmpty())
    {
        output_file_text->setText(fileName);	
        strcpy(dirac_data.fname_out, fileName.toAscii());
	outfile_valid = true;
	
	dot = checkExtn(dirac_data.fname_out, extn);

	if (strcmp(extn, ".yuv") == 0 || 
	    strcmp(extn, ".drc") == 0 || 
	    strcmp(extn, ".imt") == 0)
	{
	    int i = strlen(dirac_data.fname_out);
	    dirac_data.fname_out[i - 4] = 0;
	}

	sprintf(txt, "Output files: %s.yuv, %s.drc, %s.imt",
		basename(dirac_data.fname_out),
		basename(dirac_data.fname_out),
		basename(dirac_data.fname_out)); 
	output_file_name->setText(txt);    }
    else
    {
        QMessageBox::information(this, tr("Dirac"),
                                     tr("Cannot open %1.").arg(fileName));
    }
    
    if (infile_valid && outfile_valid)
        done->setEnabled(true);
}


bool MyDialog::checkExtn(char *fname, char *extn)
{
    bool dot;
    int i;
    
    for (i = 0; i < 6; i++)
        extn[i] = 0;
    dot = false;
    i = strlen(fname) - 1;
    while (i >= (int)strlen(fname) - 5)
    {
        i--; 
        if (fname[i + 1] == '.')
        {
            dot = true;
            break;
        }    
    }
    if (dot)
        strncpy(extn, &fname[i + 1], strlen(fname) - i);

    return dot;
}
