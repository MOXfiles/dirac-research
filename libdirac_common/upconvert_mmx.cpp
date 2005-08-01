/* ***** BEGIN LICENSE BLOCK *****
*
* $Id$ $Name$
*
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Mozilla Public License
* Version 1.1 (the "License"); you may not use this file except in compliance
* with the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
* the specific language governing rights and limitations under the License.
*
* The Original Code is BBC Research and Development code.
*
* The Initial Developer of the Original Code is the British Broadcasting
* Corporation.
* Portions created by the Initial Developer are Copyright (C) 2004.
* All Rights Reserved.
*
* Contributor(s): Anuradha Suraparaju (Original Author)
*                 Thomas Davies  (upconvert.cpp)
*
* Alternatively, the contents of this file may be used under the terms of
* the GNU General Public License Version 2 (the "GPL"), or the GNU Lesser
* Public License Version 2.1 (the "LGPL"), in which case the provisions of
* the GPL or the LGPL are applicable instead of those above. If you wish to
* allow use of your version of this file only under the terms of the either
* the GPL or LGPL and not to allow others to use your version of this file
* under the MPL, indicate your decision by deleting the provisions above
* and replace them with the notice and other provisions required by the GPL
* or LGPL. If you do not delete the provisions above, a recipient may use
* your version of this file under the terms of any one of the MPL, the GPL
* or the LGPL.
* ***** END LICENSE BLOCK ***** */
#if defined (HAVE_MMX)

#include <libdirac_common/upconvert.h>
using namespace dirac;

#include <mmintrin.h>
typedef union 
{
    __m64 m;
    int i[2];
} u_sum;


inline void mmx_add (const short *pic1, const short *pic2, const __m64 tap, const __m64 zero, __m64 *sum1, __m64 *sum2)
{
    __m64 tmp = _mm_add_pi16 (*(__m64 *)pic1, *(__m64 *)pic2);
    __m64 m1 = _mm_unpacklo_pi16 ( tmp, zero);
    __m64 m2 = _mm_unpackhi_pi16 ( tmp, zero);
    m1 = _mm_madd_pi16 (m1, tap);
    m2 = _mm_madd_pi16 (m2, tap);
    *sum1 = _mm_add_pi32 (*sum1, m1);
    *sum2 = _mm_add_pi32 (*sum2, m2);
}

// Upconvert by a factor of 2
void UpConverter::DoUpConverter(const PicArray& pic_data, PicArray& up_data)
{

    m_width_old = pic_data.LengthX();
    m_height_old = pic_data.LengthY();
    m_width_new = up_data.LengthX();
    m_height_new = up_data.LengthY();    

    //Variables that will be used by the filter calculations
    u_sum sum1;
    u_sum sum2;
    int ypos(0);

    static __m64 zero = _mm_set_pi16(0, 0, 0, 0);
    static __m64 tap0 = _mm_set_pi16 (0, m_tap0, 0, m_tap0);
    static __m64 tap1 = _mm_set_pi16 (0, m_tap1, 0, m_tap1);
    static __m64 tap2 = _mm_set_pi16 (0, m_tap2, 0, m_tap2);
    static __m64 tap3 = _mm_set_pi16 (0, m_tap3, 0, m_tap3);
    static __m64 tap4 = _mm_set_pi16 (0, m_tap4, 0, m_tap4);

    //There are three y loops to cope with the leading edge, middle 
    //and trailing edge of each column.

    for(int y = 0 ; y < m_filter_size; ++y , ypos += 2)
    {

        //We are filtering each column but doing it bit by bit.
        //This means our main loop is in the x direction and
        //there is a much greater chance the data we need will
        //be in the cache.
        for(int x = 0 , xpos = 0; x < m_width_old; x+=4 , xpos+=8 )
        {
            sum1.m = _mm_set_pi32 (0, 0);
            sum2.m = _mm_set_pi32 (0, 0);
            
            up_data[ypos][xpos] = pic_data[y][x];
            up_data[ypos][xpos+2] = pic_data[y][x+1];
            up_data[ypos][xpos+4] = pic_data[y][x+2];
            up_data[ypos][xpos+6] = pic_data[y][x+3];

            //Work out the next pixel from filtered values.
            //Excuse the complicated ternary stuff but it sorts out the edge
            mmx_add (&pic_data[y][x], &pic_data[y+1][x], tap0, zero, &sum1.m, &sum2.m);
            mmx_add (&pic_data[(y>=1)?(y-1):0][x], &pic_data[y+2][x], tap1, zero, &sum1.m, &sum2.m);
            mmx_add (&pic_data[(y>=2)?(y-2):0][x], &pic_data[y+3][x], tap2, zero, &sum1.m, &sum2.m);
            mmx_add (&pic_data[(y>=3)?(y-3):0][x], &pic_data[y+4][x], tap3, zero, &sum1.m, &sum2.m);
            mmx_add (&pic_data[(y>=4)?(y-4):0][x], &pic_data[y+5][x], tap4, zero, &sum1.m, &sum2.m);

            sum1.m = _mm_srai_pi32 (sum1.m, m_filter_shift);
            sum2.m = _mm_srai_pi32 (sum2.m, m_filter_shift);
            
            up_data[ypos+1][xpos] = sum1.i[0];
            up_data[ypos+1][xpos+2] = sum1.i[1];
            up_data[ypos+1][xpos+4] = sum2.i[0];
            up_data[ypos+1][xpos+6] = sum2.i[1];
        }// x, xpos

        // The row loop.
        RowLoop(up_data, ypos);
    }// y, ypos
    // This loop is like the last one but it deals with the centre
    // section of the image and so the ternary operations are dropped
    // from the filter section.
    for(int y = m_filter_size; y < m_height_old - m_filter_size; ++y , ypos += 2)
    {
        for(int x = 0 , xpos=0; x < m_width_old; x+=4 , xpos+=8 )
        {

            sum1.m = _mm_set_pi32 (0, 0);
            sum2.m = _mm_set_pi32 (0, 0);
            up_data[ypos][xpos] = pic_data[y][x];
            up_data[ypos][xpos+2] = pic_data[y][x+1];
            up_data[ypos][xpos+4] = pic_data[y][x+2];
            up_data[ypos][xpos+6] = pic_data[y][x+3];

            mmx_add (&pic_data[y][x], &pic_data[y+1][x], tap0, zero, &sum1.m, &sum2.m);
            mmx_add (&pic_data[y-1][x], &pic_data[y+2][x], tap1, zero, &sum1.m, &sum2.m);
            mmx_add (&pic_data[y-2][x], &pic_data[y+3][x], tap2, zero, &sum1.m, &sum2.m);
            mmx_add (&pic_data[y-3][x], &pic_data[y+4][x], tap3, zero, &sum1.m, &sum2.m);
            mmx_add (&pic_data[y-4][x], &pic_data[y+5][x], tap4, zero, &sum1.m, &sum2.m);

            sum1.m = _mm_srai_pi32 (sum1.m, m_filter_shift);
            sum2.m = _mm_srai_pi32 (sum2.m, m_filter_shift);
            
            up_data[ypos+1][xpos] = sum1.i[0];
            up_data[ypos+1][xpos+2] = sum1.i[1];
            up_data[ypos+1][xpos+4] = sum2.i[0];
            up_data[ypos+1][xpos+6] = sum2.i[1];

        }// x,xpos
        RowLoop(up_data, ypos);

    }// y, ypos 
    // Another similar loop! - this time we are dealing with
    // the trailing edge so the ternary stuff is back in the
    // filter calcs but in the second parameter.    
    for(int y = m_height_old - m_filter_size; y < m_height_old; ++y , ypos+=2)
    {
        for(int x = 0 , xpos=0 ; x < m_width_old; x+=4 , xpos+=8)
        {
            sum1.m = _mm_set_pi32 (0, 0);
            sum2.m = _mm_set_pi32 (0, 0);
            
            up_data[ypos][xpos] = pic_data[y][x];
            up_data[ypos][xpos+2] = pic_data[y][x+1];
            up_data[ypos][xpos+4] = pic_data[y][x+2];
            up_data[ypos][xpos+6] = pic_data[y][x+3];

            //Work out the next pixel from filtered values.
            //Excuse the complicated ternary stuff but it sorts out the edge
            mmx_add (&pic_data[y][x], &pic_data[((y+1)<m_height_old)?(y+1):(m_height_old-1)][x], tap0, zero, &sum1.m, &sum2.m);
            mmx_add (&pic_data[y-1][x], &pic_data[((y+2)<m_height_old)?(y+2):(m_height_old-1)][x], tap1, zero, &sum1.m, &sum2.m);
            mmx_add (&pic_data[y-2][x], &pic_data[((y+3)<m_height_old)?(y+3):(m_height_old-1)][x], tap2, zero, &sum1.m, &sum2.m);
            mmx_add (&pic_data[y-3][x], &pic_data[((y+4)<m_height_old)?(y+4):(m_height_old-1)][x], tap3, zero, &sum1.m, &sum2.m);
            mmx_add (&pic_data[y-4][x], &pic_data[((y+5)<m_height_old)?(y+5):(m_height_old-1)][x], tap4, zero, &sum1.m, &sum2.m);

            sum1.m = _mm_srai_pi32 (sum1.m, m_filter_shift);
            sum2.m = _mm_srai_pi32 (sum2.m, m_filter_shift);
            
            up_data[ypos+1][xpos] = sum1.i[0];
            up_data[ypos+1][xpos+2] = sum1.i[1];
            up_data[ypos+1][xpos+4] = sum2.i[0];
            up_data[ypos+1][xpos+6] = sum2.i[1];

        }//x,xpos
        RowLoop(up_data, ypos);

    }//y,ypos
    _mm_empty();
}
#endif
