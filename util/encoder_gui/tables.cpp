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

#include "diracgui.h"
#include "tables.h"

char cformat_names[NUM_CFS][10] = {"Y", "422", "444", "420", "411"};
    
char preset_names[NUM_PRESETS][10] = {"CIF", "SD576", "HD720", "HD1080"};

/* DiracData format (all short integers, except frame_rate (float)):                        */
/*   {quality, frame_rate, frame_height, frame_width, cformat,   */
/*    cpd, xblen, yblen, xbsep, ybsep, start, stop, num_l1, sep_l1} */
    
DiracData dirac_preset[NUM_PRESETS] =
  {{10, 12.5, 288, 352, 3,  20, 12, 12, 8, 8, 0, 999999, 11, 3, "", ""},
   {10, 25.0, 576, 720, 3,  32, 12, 12, 8, 8, 0, 999999, 11, 3, "", ""},
   {10, 50.0, 720, 1280, 3,  20, 12, 12, 8, 8, 0, 999999, 11, 3, "", ""},
   {10, 25.0, 1080, 1920, 3,  32, 24, 24, 16, 16, 0, 999999, 11, 3, "", ""}}; 
