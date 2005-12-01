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
#include <stdlib.h>

#include <QtGui>

#include "diracdata.h"
#include "repthread.h"
#include "diracgui.h"

RepThread::RepThread(QObject *parent)
         : QThread(parent)
{
    restart = false;
    abort = false;
}

RepThread::~RepThread()
{
    mutex.lock();
    abort = true;
    cond.wakeOne();
    mutex.unlock();
    wait();
}
    
void RepThread::report()
{
    QMutexLocker locker(&mutex);

    if (!isRunning())
    {
        fprintf (stderr, "Thread not running\n");
        start(LowPriority);
    }
    else
    {
        fprintf (stderr, "Thread Running\n");
        restart = true;
        cond.wakeOne();
    }
}

void RepThread::run()
{
    char cmd[1000];
    FILE *op_ptr;
    char buf[1000];
                
    sprintf(cmd, "dirac_encoder -width %d -height %d -fr %f -cformat %d -qf %d -cpd %d -xblen %d -yblen %d -xbsep %d -ybsep %d -start %ld -stop %ld -num_L1 %d -L1_sep %d %s %s %s", 
       local_data.frame_width,
       local_data.frame_height,
       local_data.frame_rate,
       local_data.cformat,
       local_data.quality,
       local_data.cpd,
       local_data.xblen,
       local_data.yblen,
       local_data.xbsep,
       local_data.ybsep,
       local_data.start,
       local_data.stop,
       local_data.num_l1,
       local_data.sep_l1,
       local_data.local ? "-local" : "",
       local_data.fname_in,
       local_data.fname_out);
       
    //printf("%s\n", cmd);
         
    if ((op_ptr = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "popen failed\n");
    }
         
    while (fgets(buf, 100, op_ptr))
    {
        if (restart)
            break;
        if (abort)
            break;
        
        QString qbuf = QString(buf);
        emit RepMessage(qbuf);

        fflush(stdout);
        fflush(op_ptr); 
    }
     
    pclose(op_ptr);
    
    emit RepFinished();
    
    mutex.lock();
    if (!restart)
        cond.wait(&mutex);
    restart = false;
    mutex.unlock();
}

void RepThread::RepGetDiracData(DiracData data)
{
    local_data = data;
}

