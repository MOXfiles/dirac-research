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

#ifndef TABLES_H
#define TABLES_H

#include "diracdata.h"

#define NUM_PRESETS 4
#define NUM_CFS 5

#define MIN_QUALITY 0
#define MAX_QUALITY 15
#define DEF_QUALITY 10

#define MIN_WIDTH 160
#define MAX_WIDTH 2560
#define MIN_HEIGHT 100
#define MAX_HEIGHT 2048
#define MIN_FPS 10
#define MAX_FPS 60

#define MIN_CPD 1
#define MAX_CPD 100
#define MIN_LEN 1
#define MAX_LEN 128
#define MIN_SEP 1
#define MAX_SEP 128
#define MIN_NUML1 0
#define MAX_NUML1 24
#define DEF_NUML1 11
#define MIN_SEPL1 0
#define MAX_SEPL1 24
#define DEF_SEPL1 3

#define MIN_START 0
#define MAX_START 999998
#define MIN_STOP 1
#define MAX_STOP 999999

extern char cformat_names[NUM_CFS][10];
extern char preset_names[NUM_PRESETS][10];
extern DiracData dirac_preset[NUM_PRESETS];
#endif
