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

#ifndef DIRACDATA_H
#define DIRACDATA_H

class DiracData
{
public:
    short quality;
    float frame_rate;
    short frame_height;
    short frame_width;
    short cformat; 
    short cpd;
    short xblen;
    short yblen;
    short xbsep;
    short ybsep;
    long start;
    long stop;
    short num_l1;
    short sep_l1;
    char fname_in[1000];
    char fname_out[1000];
};

#endif
