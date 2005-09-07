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

#ifndef DIRACGUI_H
#define DIRACGUI_H

#include <QApplication>
#include <QDialog>
#include <QFont>
#include <QLCDNumber>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>
#include <QGridLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QObject>
#include <QList>
#include <QLabel>
#include <QGroupBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QWidget>
#include <QComboBox>
#include <QListWidget>
#include <QThread>
//#include "tables.h"

#include "diracdata.h"
#include "repthread.h"
    

class MyDialog : public QDialog
{
    Q_OBJECT

public:
    //FILE *op_ptr;
    /*char fname_in[1000];
    char fname_out[1000];*/
    bool infile_valid;
    bool outfile_valid;
    DiracData dirac_data;

    QPushButton *done;
    QSpinBox *width_val;
    QSpinBox *height_val;
    QLineEdit *frame_rate_val;    
    QSpinBox *cpd_val;
    QSpinBox *xblen_val;
    QSpinBox *yblen_val; 
    QSpinBox *xbsep_val;
    QSpinBox *ybsep_val;
    QSpinBox *num_l1_val;
    QSpinBox *sep_l1_val;
    QSpinBox *start_val;
    QSpinBox *stop_val;
    QComboBox *cformats;
    QRadioButton **presets;

    QLineEdit *input_file_text;
    QLineEdit *output_file_text;
    QLabel *input_file_name;
    QLabel *output_file_name;

    QLabel *reportrun;
        
    MyDialog(QDialog *parent = 0);
    void createFilesBox();
    void createQualityBox();
    void createPresetBox();
    void createVideoBox();
    void createEncodeBox(); 
    void createReportBox();
    void setPresetValues(int num);

    void useInputFile(QString fileName);
    void useOutputFile(QString fileName); 
    bool checkExtn(char *fname, char *extn);
    
public slots:
    void doneValues();
    void checkQualityBox(int val);
    void setReset(void);
    void help(void);
    
    void setPreset0(void);
    void setPreset1(void);
    void setPreset2(void);
    void setPreset3(void);
        
    void checkVideoBoxW(int val);    
    void checkVideoBoxH(int val); 
    void checkVideoBoxFR(void); 
    void checkVideoBoxCF(int val);

    void showEncode(void);
    void checkEncodeBoxCPD(int val);
    void checkEncodeBoxXL(int val);
    void checkEncodeBoxYL(int val);
    void checkEncodeBoxXS(int val);
    void checkEncodeBoxYS(int val);
    void checkEncodeBoxN1(int val);    
    void checkEncodeBoxS1(int val);        
    void checkEncodeBoxStart(int val);
    void checkEncodeBoxStop(int val);
    
    void openInputFile();
    void openOutputFile();
    void textInputFile();
    void textOutputFile();    
    
    void ReportSetMessage(QString mess);
    void ReportFinished(void);
    
signals:
    void SendDiracData(DiracData dirac_data);

private:
    RepThread rep_thread;
    QGroupBox *filesGroupBox;
    QGroupBox *qualityGroupBox;
    QGroupBox *presetGroupBox;
    QGroupBox *videoGroupBox;
    QGroupBox *encodeGroupBox;
    QGroupBox *reportGroupBox;
    
    QGridLayout *encode_box;
    QGridLayout *encode_box2;    
    QPushButton *encode_more;
    QWidget *encode_widget;
    QWidget *encode_widget2;
        
    QVBoxLayout *report_vbox;
    QWidget *report_widget;
    QListWidget *reportdoc;
};


//extern DiracData dirac_data;
//extern FILE *op_ptr;
//extern QListWidget *reportdoc;

#endif
