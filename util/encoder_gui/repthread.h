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

#ifndef REPTHREAD_H
#define REPTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
    
#include "diracdata.h"
//#include "diracgui.h"

class RepThread : public QThread
{
    Q_OBJECT

public:
    RepThread(QObject *parent = 0);
    ~RepThread();

    void report();
    
    void run();

signals:
    void RepMessage(QString message);
    void RepFinished(void);

public slots:
    void RepGetDiracData(DiracData data);
    
private:
    QMutex mutex;
    QWaitCondition cond;
    bool restart;
    bool abort;
    
    DiracData local_data;
};


#endif
