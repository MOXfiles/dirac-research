/* ***** BEGIN LICENSE BLOCK *****
*
* $Id$ $Name$
*
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Mozilla Public License
* Version 1.1 (the "License");  you may not use this file except in compliance
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
* Contributor(s): Anuradha Suraparaju (Original Author),
*                 Thomas Davies (wavelet_utils.cpp)
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


#ifdef HAVE_MMX
#include <libdirac_common/wavelet_utils.h>
#include <cstdlib>
#include <mmintrin.h>
using namespace dirac;

static TwoDArray<ValueType> t_temp_data;

inline void Interleave_mmx( const int xp , 
                    const int yp , 
                    const int xl , 
                    const int yl , 
                    PicArray& pic_data)
{
    const int xl2( xl>>1);
    const int yl2( yl>>1);
    const int yend( yp + yl );

    if (pic_data.LengthX() > t_temp_data.LengthX() ||
        pic_data.LengthY() > t_temp_data.LengthY())
    {
        t_temp_data.Resize(pic_data.LengthY(), pic_data.LengthX());
    }

    // Make a temporary copy of the subband
    for (int j = yp; j<yend ; j++ )
        memcpy( t_temp_data[j-yp] , pic_data[j]+xp , xl * sizeof( ValueType ) );

    int stopx = (xl2>>2)<<2;
    // Re-order to interleave
    for (int j = 0, s=yp; j<yl2 ; j++, s+=2)
    {
        ValueType *tmp1 = &t_temp_data[j][0];
        ValueType *tmp2 = &t_temp_data[j][xl2];
        __m64 *out = (__m64 *)&pic_data[s][xp];
        for (int i = 0 , t = xp ; i<xp+stopx ; i+=4 , t+=8)
        {
            __m64 m1 = *(__m64 *)tmp1;
            __m64 m2 = *(__m64 *)tmp2;
            *out = _mm_unpacklo_pi16 (m1, m2);    
            ++out;
            *out = _mm_unpackhi_pi16 (m1, m2);    
            ++out;
            tmp1 += 4;
            tmp2 += 4;
        }
    }// j 
    
    for (int j = yl2, s=yp+1 ; j<yl ; j++ , s += 2)
    {
        ValueType *tmp1 = &t_temp_data[j][0];
        ValueType *tmp2 = &t_temp_data[j][xl2];
        __m64 *out = (__m64 *)&pic_data[s][xp];
        for (int i = 0 , t=xp; i<stopx ; i+=4 , t += 8)
        {
            __m64 m1 = *(__m64 *)tmp1;
            __m64 m2 = *(__m64 *)tmp2;
            *out = _mm_unpacklo_pi16 (m1, m2);
            ++out;
            *out = _mm_unpackhi_pi16 (m1, m2);    
            ++out;
            tmp1 += 4;
            tmp2 += 4;
        }
    }// j 

    // Mop up
    if (stopx != xl2)
    {
        for (int j = 0, s=yp; j<yl2 ; j++, s+=2)
        {
            ValueType *pic = pic_data[s];
            ValueType *tmp = t_temp_data[j];
            for (int i = stopx , r=2*(xp+stopx) ; i<xl2 ; i++ , r += 2)
                pic[r] = tmp[i];
            for (int i = xl2+stopx, r=2*(xp+stopx)+1; i<xl ; i++ , r += 2)
                pic[r] = tmp[i];
        }// j 

        for (int j = yl2, s=yp+1 ; j<yl ; j++ , s += 2)
        {
            ValueType *pic = pic_data[s];
            ValueType *tmp = t_temp_data[j];
            for (int i = stopx , r=2*stopx ; i<xl2 ; i++ , r += 2)
                pic[r] = tmp[i];
            for (int i = xl2+stopx, r=2*(xp+stopx)+1; i<xl ; i++ , r += 2)
                pic[r] = tmp[i];
        }
    }
    _mm_empty();
}

void WaveletTransform::VHFilterApprox9_7::Synth(const int xp , 
                                                const int yp , 
                                                const int xl , 
                                                const int yl , 
                                                PicArray& pic_data)
{
    int i, j;
    const int xend( xp+xl );
    const int yend( yp+yl );
    const int ymid = yp+yl/2;

    PredictStepShift<2> predict;
    UpdateStepFourTap< 4 , 9 , -1> update;
    
    int xstop = xp + (xl>>2)<<2;

    // First lifting stage
    // Top edge
    ValueType *in1 =  pic_data[ymid];
    ValueType *in2 =  pic_data[ymid];
    ValueType *out = pic_data[yp];
       for ( i = xp ; i < xstop ; i+=4 )
    {
        __m64 tmp = _mm_add_pi16 (*(__m64 *)in1, *(__m64 *)in2);
        tmp = _mm_srai_pi16(tmp, 2);
        *(__m64 *)out = _mm_sub_pi16 (*(__m64*)out, tmp);
        out += 4;
        in1 += 4;
        in2 += 4;
    }

    // Middle bit
       for ( j=1 ; j < yl/2 ; ++j )
    {
        in1 =  pic_data[ymid+j-1];
        in2 =  pic_data[ymid+j];
        out = pic_data[yp+j];
           for ( i = xp ; i < xstop ; i+=4 )
        {
            __m64 tmp = _mm_add_pi16 (*(__m64 *)in1, *(__m64 *)in2);
            tmp = _mm_srai_pi16(tmp, 2);
            *(__m64 *)out = _mm_sub_pi16 (*(__m64*)out, tmp);
            out += 4;
            in1 += 4;
            in2 += 4;
        }
    }

    // Mopup
    if (xstop != xend)
    {
        for ( i = xstop ; i < xend ; i++)
        {
            predict.Filter( pic_data[yp][i] , pic_data[ymid][i] , pic_data[ymid][i] );
        }// i

        for ( j=1 ; j < yl/2 ; ++j )
        {
            for ( i = xstop ; i < xend ; i++)
            {
                predict.Filter( pic_data[yp+j][i] , pic_data[ymid+j-1][i] , pic_data[ymid+j][i] );
            }// i
        }// j
    }
 
    // Second lifting stage
    // top edge
    in1 = pic_data[yp];
    in2 = pic_data[yp+1];
    ValueType *in3 = pic_data[yp];
    ValueType *in4 = pic_data[yp+2];
    out = pic_data[ymid];
    __m64 tap1 = _mm_set_pi16 (9, 9, 9, 9);
    __m64 tap2 = _mm_set_pi16 (-1, -1, -1, -1);
    
    for ( i = xp ; i < xstop ; i+=4 )
    {
        __m64 val1 = _mm_add_pi16(*(__m64*)in1, *(__m64*)in2);
        __m64 val2 = _mm_add_pi16(*(__m64*)in3, *(__m64*)in4);
        val1 = _mm_mullo_pi16 (val1, tap1);
        val2 = _mm_mullo_pi16 (val2, tap2);
        val1 = _mm_add_pi16 (val1, val2);
        val1 = _mm_srai_pi16(val1, 4);
        *(__m64*)out = _mm_add_pi16 (*(__m64*)out,val1);
        in1 +=4;
        in2 +=4;
        in3 +=4;
        in4 +=4;
        out +=4;
    }

       // middle bit
       for ( j=1 ; j < yl/2 - 2 ; ++j)
       {
        in1 = pic_data[yp+j];
        in2 = pic_data[yp+j+1];
        in3 = pic_data[yp+j-1];
        in4 = pic_data[yp+j+2];
        out = pic_data[ymid+j];
           for ( i = xp ; i < xstop ; i+=4)
           {
            __m64 val1 = _mm_add_pi16(*(__m64*)in1, *(__m64*)in2);
            __m64 val2 = _mm_add_pi16(*(__m64*)in3, *(__m64*)in4);
            val1 = _mm_mullo_pi16 (val1, tap1);
            val2 = _mm_mullo_pi16 (val2, tap2);
            val1 = _mm_add_pi16 (val1, val2);
            val1 = _mm_srai_pi16(val1, 4);
            *(__m64*)out = _mm_add_pi16 (*(__m64*)out,val1);
            in1 +=4;
            in2 +=4;
            in3 +=4;
            in4 +=4;
            out +=4;
           }// i
       }// k
   
       // bottom edge
    in1 = pic_data[ymid-2];
    in2 = pic_data[ymid-1];
    in3 = pic_data[ymid-3];
    in4 = pic_data[ymid-1];
    out = pic_data[yend-2];
    for ( i = xp ; i < xstop ; i+=4)
    {
        __m64 val1 = _mm_add_pi16(*(__m64*)in1, *(__m64*)in2);
        __m64 val2 = _mm_add_pi16(*(__m64*)in3, *(__m64*)in4);
        val1 = _mm_mullo_pi16 (val1, tap1);
        val2 = _mm_mullo_pi16 (val2, tap2);
        val1 = _mm_add_pi16 (val1, val2);
        val1 = _mm_srai_pi16(val1, 4);
        *(__m64*)out = _mm_add_pi16 (*(__m64*)out,val1);
        in1 +=4;
        in2 +=4;
        in3 +=4;
        in4 +=4;
        out +=4;
    }
        
    in1 = pic_data[ymid-1];
    in2 = pic_data[ymid-1];
    in3 = pic_data[ymid-2];
    in4 = pic_data[ymid-1];
    out = pic_data[yend-1];
    for ( i = xp ; i < xstop ; i+=4)
    {
        __m64 val1 = _mm_add_pi16(*(__m64*)in1, *(__m64*)in2);
        __m64 val2 = _mm_add_pi16(*(__m64*)in3, *(__m64*)in4);
        val1 = _mm_mullo_pi16 (val1, tap1);
        val2 = _mm_mullo_pi16 (val2, tap2);
        val1 = _mm_add_pi16 (val1, val2);
        val1 = _mm_srai_pi16(val1, 4);
        *(__m64*)out = _mm_add_pi16 (*(__m64*)out,val1);
        in1 +=4;
        in2 +=4;
        in3 +=4;
        in4 +=4;
        out +=4;
    }
        
    if (xstop != xend)
    {
        for ( i = xstop ; i < xend ; i++)
        {
               update.Filter( pic_data[ymid][i] , pic_data[yp][i], pic_data[yp+1][i], pic_data[yp][i],pic_data[yp+2][i]);
        }// i

        // middle bit
           for ( j=1 ; j < yl/2 - 2 ; ++j)
        {
            for ( i = xstop ; i < xend ; i++)
            {
                update.Filter( pic_data[ymid+j][i] , pic_data[yp+j][i], pic_data[yp+j+1][i], pic_data[yp+j-1][i],pic_data[yp+j+2][i]);
            }// i
        }// k
    
        for ( i = xstop ; i < xend ; i++)
        {
            update.Filter( pic_data[yend - 2][i] , pic_data[ymid-2][i], pic_data[ymid-1][i], pic_data[ymid-3][i],pic_data[ymid-1][i]);
            update.Filter( pic_data[yend - 1][i] , pic_data[ymid-1][i], pic_data[ymid-1][i], pic_data[ymid-2][i],pic_data[ymid-1][i]);
        }// i
    }


    // Horizontal sythesis
    
    const int xmid = xl/2;
    xstop = xmid %4 ? ((xmid>>2)<<2) + 1 : xmid -3;

    for (j = yp;  j < yend; ++j)
    {
        ValueType *line_data = &pic_data[j][xp];                 

        // First lifting stage acts on even samples i.e. the low pass ones
         predict.Filter( line_data[0] , line_data[xmid] , line_data[xmid] );
        for (i=1 ; i < xmid ; ++i)
        {
            predict.Filter( line_data[i] , line_data[xmid+i-1] , line_data[xmid+i] );
        }

        // Second lifting stage
           update.Filter( line_data[xmid] , line_data[0] , line_data[1] , line_data[0] , line_data[2] );

        for (i=1 ; i < xmid - 2; ++i)
        {
            update.Filter( line_data[xmid+i] , line_data[i] , line_data[i+1] , line_data[i-1] , line_data[i+2] );
        }// i 
        update.Filter( line_data[xl-2] , line_data[xmid-2] , line_data[xmid-1] , line_data[xmid-3] , line_data[xmid-1] );
        update.Filter( line_data[xl-1] , line_data[xmid-1] , line_data[xmid-1] , line_data[xmid-2] , line_data[xmid-1] );
    }// j
    _mm_empty();
    Interleave_mmx( xp , yp , xl ,yl , pic_data );
}

void WaveletTransform::VHFilter13_5::Synth(const int xp ,
                                           const int yp , 
                                           const int xl ,
                                           const int yl , 
                                           PicArray& pic_data)
{
    int i,j,k;

    const int xend( xp+xl );
    const int yend( yp+yl );

    PredictStepFourTap< 5 , 9 , -1 > predict;
    UpdateStepFourTap< 4 , 9 , -1> update;

    // Next, do the vertical synthesis
    int ymid = yp + yl/2;

    int xstop = xp + (xl>>2)<<2;
    // First lifting stage - odd samples
    // bottom edge
    ValueType *out = pic_data[ymid-1];
    ValueType *in1 = pic_data[yend-2];
    ValueType *in2 = pic_data[yend-1];
    ValueType *in3 = pic_data[yend-3];
    ValueType *in4 = pic_data[yend-1];

    __m64 tap1 = _mm_set_pi16 (9, 9, 9, 9);
    __m64 tap2 = _mm_set_pi16 (-1, -1, -1, -1);
    for ( i = xp ; i<xstop; i+=4)
    {
        __m64 val1 = _mm_add_pi16(*(__m64*)in1, *(__m64*)in2);
        __m64 val2 = _mm_add_pi16(*(__m64*)in3, *(__m64*)in4);
        val1 = _mm_mullo_pi16 (val1, tap1);
        val2 = _mm_mullo_pi16 (val2, tap2);
        val1 = _mm_add_pi16 (val1, val2);
        val1 = _mm_srai_pi16(val1, 5);
        *(__m64*)out = _mm_sub_pi16 (*(__m64*)out,val1);
        in1 +=4;
        in2 +=4;
        in3 +=4;
        in4 +=4;
        out +=4;
    }// i

    // middle bit
    for ( j = 2 ; j < yl/2 -1 ; ++j)
    {
        out = pic_data[yp+j];
        in1 = pic_data[ymid+j-1];
        in2 = pic_data[ymid+j];
        in3 = pic_data[ymid+j-2];
        in4 = pic_data[ymid+j+1];
        for ( i = xp ; i<xstop ; i+=4)
        {
            __m64 val1 = _mm_add_pi16(*(__m64*)in1, *(__m64*)in2);
            __m64 val2 = _mm_add_pi16(*(__m64*)in3, *(__m64*)in4);
            val1 = _mm_mullo_pi16 (val1, tap1);
            val2 = _mm_mullo_pi16 (val2, tap2);
            val1 = _mm_add_pi16 (val1, val2);
            val1 = _mm_srai_pi16(val1, 5);
            *(__m64*)out = _mm_sub_pi16 (*(__m64*)out,val1);
            in1 +=4;
            in2 +=4;
            in3 +=4;
            in4 +=4;
            out +=4;
        }// i
    }// j

    // top edge - j=xp
    out = pic_data[yp+1];
    in1 = pic_data[ymid];
    in2 = pic_data[ymid+1];
    in3 = pic_data[ymid+2];
    in4 = pic_data[ymid];
    for ( i = xp ; i<xstop ; i+=4)
    {
        __m64 val1 = _mm_add_pi16(*(__m64*)in1, *(__m64*)in2);
        __m64 val2 = _mm_add_pi16(*(__m64*)in3, *(__m64*)in4);
        val1 = _mm_mullo_pi16 (val1, tap1);
        val2 = _mm_mullo_pi16 (val2, tap2);
        val1 = _mm_add_pi16 (val1, val2);
        val1 = _mm_srai_pi16(val1, 5);
        *(__m64*)out = _mm_sub_pi16 (*(__m64*)out,val1);
        in1 +=4;
        in2 +=4;
        in3 +=4;
        in4 +=4;
        out +=4;
    }

    out = pic_data[yp];
    in1 = pic_data[ymid];
    in2 = pic_data[ymid];
    in3 = pic_data[ymid+1];
    in4 = pic_data[ymid];
    for ( i = xp ; i<xstop ; i+=4)
    {
        __m64 val1 = _mm_add_pi16(*(__m64*)in1, *(__m64*)in2);
        __m64 val2 = _mm_add_pi16(*(__m64*)in3, *(__m64*)in4);
        val1 = _mm_mullo_pi16 (val1, tap1);
        val2 = _mm_mullo_pi16 (val2, tap2);
        val1 = _mm_add_pi16 (val1, val2);
        val1 = _mm_srai_pi16(val1, 5);
        *(__m64*)out = _mm_sub_pi16 (*(__m64*)out,val1);
        in1 +=4;
        in2 +=4;
        in3 +=4;
        in4 +=4;
        out +=4;
    }

    // Mopup 
    if ( xstop != xend)
    {
        // Mopup bottom edge
        for ( i = xstop ; i<xend ; ++ i)
        {
            predict.Filter( pic_data[ymid-1][i] , pic_data[yend-2][i] , pic_data[yend-1][i] , pic_data[yend-3][i] , pic_data[yend-1][i] );
        }// i

        // Mopup middle bit
        for ( k = 2 ; k < yl/2 - 1 ; ++k)
        {
            for ( i = xstop ; i<xend ; ++ i)
            {
                   predict.Filter( pic_data[yp+k][i] , pic_data[ymid+k-1][i] , pic_data[ymid+k][i] , pic_data[ymid+k-2][i] , pic_data[ymid+k+1][i] );
            }// i
        }// k

        //Mopup top edge
        for ( i = xstop ; i<xend ; ++ i)
        {
            predict.Filter( pic_data[yp+1][i] , pic_data[ymid][i] , pic_data[ymid+1][i] , pic_data[ymid+2][i] , pic_data[ymid][i] );
            predict.Filter( pic_data[yp][i] , pic_data[ymid][i] , pic_data[ymid][i] , pic_data[ymid+1][i] , pic_data[ymid][i] );

        }// i

    }

    // Second lifting stage
    // top edge - j=xp
    out = pic_data[ymid];
    in1 = pic_data[yp];
    in2 = pic_data[yp+1];
    in3 = pic_data[yp];
    in4 = pic_data[yp+2];
    for ( i = xp ; i<xstop ; i+=4)
    {
        __m64 val1 = _mm_add_pi16(*(__m64*)in1, *(__m64*)in2);
        __m64 val2 = _mm_add_pi16(*(__m64*)in3, *(__m64*)in4);
        val1 = _mm_mullo_pi16 (val1, tap1);
        val2 = _mm_mullo_pi16 (val2, tap2);
        val1 = _mm_add_pi16 (val1, val2);
        val1 = _mm_srai_pi16(val1, 4);
        *(__m64*)out = _mm_add_pi16 (*(__m64*)out,val1);
        in1 +=4;
        in2 +=4;
        in3 +=4;
        in4 +=4;
        out +=4;
    }// i


    // middle bit
    for ( k = 1 ; k < yl/2  - 2  ; ++k)
    {
        out = pic_data[ymid+k];
        in1 = pic_data[k];
        in2 = pic_data[k+1];
        in3 = pic_data[k-1];
        in4 = pic_data[k+2];
        for ( i = xp ; i<xstop ; i+=4)
        {
            __m64 val1 = _mm_add_pi16(*(__m64*)in1, *(__m64*)in2);
            __m64 val2 = _mm_add_pi16(*(__m64*)in3, *(__m64*)in4);
            val1 = _mm_mullo_pi16 (val1, tap1);
            val2 = _mm_mullo_pi16 (val2, tap2);
            val1 = _mm_add_pi16 (val1, val2);
            val1 = _mm_srai_pi16(val1, 4);
            *(__m64*)out = _mm_add_pi16 (*(__m64*)out,val1);
            in1 +=4;
            in2 +=4;
            in3 +=4;
            in4 +=4;
            out +=4;
        }// i
    }// k

    // bottom edge
    out = pic_data[yend-2];
    in1 = pic_data[ymid-2];
    in2 = pic_data[ymid-1];
    in3 = pic_data[ymid-3];
    in4 = pic_data[ymid-1];
    for ( i = xp ; i<xstop ; i+=4)
    {
        __m64 val1 = _mm_add_pi16(*(__m64*)in1, *(__m64*)in2);
        __m64 val2 = _mm_add_pi16(*(__m64*)in3, *(__m64*)in4);
        val1 = _mm_mullo_pi16 (val1, tap1);
        val2 = _mm_mullo_pi16 (val2, tap2);
        val1 = _mm_add_pi16 (val1, val2);
        val1 = _mm_srai_pi16(val1, 4);
        *(__m64*)out = _mm_add_pi16 (*(__m64*)out,val1);
        in1 +=4;
        in2 +=4;
        in3 +=4;
        in4 +=4;
        out +=4;
    }// i

    out = pic_data[yend-1];
    in1 = pic_data[ymid-1];
    in2 = pic_data[ymid-1];
    in3 = pic_data[ymid-2];
    in4 = pic_data[ymid-1];
    for ( i = xp ; i<xstop ; i+=4)
    {
        __m64 val1 = _mm_add_pi16(*(__m64*)in1, *(__m64*)in2);
        __m64 val2 = _mm_add_pi16(*(__m64*)in3, *(__m64*)in4);
        val1 = _mm_mullo_pi16 (val1, tap1);
        val2 = _mm_mullo_pi16 (val2, tap2);
        val1 = _mm_add_pi16 (val1, val2);
        val1 = _mm_srai_pi16(val1, 4);
        *(__m64*)out = _mm_add_pi16 (*(__m64*)out,val1);
        in1 +=4;
        in2 +=4;
        in3 +=4;
        in4 +=4;
        out +=4;
    }// i
    

    // Mopup 
    if ( xstop != xend)
    {
        // bottom edge
        for ( i = xstop ; i<xend ; ++ i)
        {
            update.Filter( pic_data[yend-1][i] , pic_data[ymid-1][i] , pic_data[ymid-1][i] , pic_data[ymid-2][i] , pic_data[ymid-1][i] );
            update.Filter( pic_data[yend-2][i] , pic_data[ymid-2][i] , pic_data[ymid-1][i] , pic_data[ymid-3][i] , pic_data[ymid-1][i] );

        }// i

        // middle bit
        for ( k = 1 ; k < yl/2  - 2  ; ++k)
        {
            for ( i = xstop ; i<xend ; ++ i)
            {
                update.Filter( pic_data[ymid+k][i] , pic_data[k][i] , pic_data[k+1][i] , pic_data[k-1][i] , pic_data[k+2][i] );
            }// i
        }// j

        // top edge - j=xp
        for ( i = xstop ; i<xend ; ++ i)
        {
            update.Filter( pic_data[ymid][i] , pic_data[yp][i] , pic_data[yp+1][i] , pic_data[yp][i] , pic_data[yp+2][i] ); 
        }// i
    }

    // Next do the horizontal synthesis

    ValueType* line_data;
    int xmid = xl/2;

    for (j = yp;  j < yend ; ++j)
    {
        line_data = &pic_data[j][xp];                 

        // First lifting stage

        predict.Filter( line_data[0] , line_data[xmid] , line_data[xmid] , line_data[xmid+1] , line_data[xmid] );
        predict.Filter( line_data[1] , line_data[xmid] , line_data[xmid+1] , line_data[xmid+2] , line_data[xmid] );

        for (k=2 ; k < xmid-1 ; ++k)
        {
            predict.Filter( line_data[k] , line_data[xmid+k-1] , line_data[xmid+k] , line_data[xmid+k-2] , line_data[xmid+k+1] );

        }// i 
        predict.Filter( line_data[xmid-1] , line_data[xl-2] , line_data[xl-1] , line_data[xl-3] , line_data[xl-1] );

         //second lifting stage 

        update.Filter( line_data[xmid] , line_data[0] , line_data[1] , line_data[0] , line_data[2] ); 
        for (k=1 ; k<xmid-2 ; ++k)
        {
            update.Filter( line_data[xmid+k] , line_data[k] , line_data[k+1] , line_data[k-1] , line_data[k+2] );
        }// i 
        update.Filter( line_data[xl-2] , line_data[xmid-2] , line_data[xmid-1] , line_data[xmid-3] , line_data[xmid-1] );
        update.Filter( line_data[xl-1] , line_data[xmid-1] , line_data[xmid-1] , line_data[xmid-2] , line_data[xmid-1] );

    }// j

    _mm_empty();
    // Interleave subbands 
    Interleave_mmx( xp , yp , xl , yl , pic_data );  
}

void WaveletTransform::VHFilter5_3::Synth(const int xp ,
                                          const int yp , 
                                          const int xl , 
                                          const int yl , 
                                          PicArray& pic_data)
{
    int i,j,k;

    const int xend( xp+xl );
    const int yend( yp+yl );

    const PredictStepShift< 2 > predict;
    const UpdateStepShift< 1 > update;

    ValueType* line_data;

    // Firstly reorder to interleave subbands, so that subsequent calculations 
    // can be in-place
    Interleave_mmx( xp , yp , xl , yl , pic_data );

    // Next, do the vertical synthesis
    // First lifting stage

    int xstop = (xend>>2)<<2;

    // Begin with the bottom edge
    ValueType *row1 = &pic_data[yend-1][xp];
    ValueType *row2 = &pic_data[yend-2][xp];
    ValueType *row3 = &pic_data[yend-3][xp];
    for ( i = xp ; i < xstop ; i+=4)
    {
        __m64 tmp = _mm_add_pi16 (*(__m64 *)row3, *(__m64 *)row1);
        tmp = _mm_srai_pi16(tmp, 2);
        *(__m64 *)row2 = _mm_sub_pi16 (*(__m64*)row2, tmp);

        tmp = _mm_add_pi16 (*(__m64 *)row2, *(__m64 *)row2);
        tmp = _mm_srai_pi16(tmp, 1);
        *(__m64 *)row1 = _mm_add_pi16 (*(__m64*)row1, tmp);
        row1 += 4;
        row2 += 4;
        row3 += 4;
    }// i
    // Mop up
    for ( i = xstop ; i < xend ; ++i)
    {
        predict.Filter( pic_data[yend-2][i] , pic_data[yend-3][i] , pic_data[yend-1][i] );
        update.Filter( pic_data[yend-1][i] , pic_data[yend-2][i] , pic_data[yend-2][i] );
    }// i
    // Next, do the middle bit
    for ( k = yend-3 ; k>yp+1 ; k-=2)
    {
        ValueType *row1 = &pic_data[k][xp];
        ValueType *row2 = &pic_data[k-1][xp];
        ValueType *row3 = &pic_data[k-2][xp];
        ValueType *row4 = &pic_data[k+1][xp];
        for ( i = xp ; i < xstop ; i+=4)
        {
            __m64 tmp = _mm_add_pi16 (*(__m64 *)row3, *(__m64 *)row1);
            tmp = _mm_srai_pi16(tmp, 2);
            *(__m64 *)row2 = _mm_sub_pi16 (*(__m64*)row2, tmp);

            tmp = _mm_add_pi16 (*(__m64 *)row4, *(__m64 *)row2);
            tmp = _mm_srai_pi16(tmp, 1);
            *(__m64 *)row1 = _mm_add_pi16 (*(__m64*)row1, tmp);
            row1 += 4;
            row2 += 4;
            row3 += 4;
            row4 += 4;
        }// i

        //Mopup
        for ( i = xstop ; i < xend ; ++i)
        {
            predict.Filter( pic_data[k-1][i] , pic_data[k-2][i] , pic_data[k][i] );
            update.Filter( pic_data[k][i] , pic_data[k+1][i] , pic_data[k-1][i] );
        }// i
    }// j
    // Then do the top edge
    row1 = &pic_data[yp][xp];
    row2 = &pic_data[yp+1][xp];
    row3 = &pic_data[yp+2][xp];
    for ( i = xp ; i < xstop ; i+=4)
    {
        __m64 tmp = _mm_add_pi16 (*(__m64 *)row2, *(__m64 *)row2);
        tmp = _mm_srai_pi16(tmp, 2);
        *(__m64 *)row1 = _mm_sub_pi16 (*(__m64*)row1, tmp);

        tmp = _mm_add_pi16 (*(__m64 *)row3, *(__m64 *)row1);
        tmp = _mm_srai_pi16(tmp, 1);
        *(__m64 *)row2 = _mm_add_pi16 (*(__m64*)row2, tmp);
        row1 += 4;
        row2 += 4;
        row3 += 4;
    }
    // Mopup
    for ( i = xstop ; i < xend ; ++i)
    {
        predict.Filter( pic_data[yp][i] , pic_data[yp+1][i] , pic_data[yp+1][i] );
        update.Filter( pic_data[yp+1][i] , pic_data[yp+2][i] , pic_data[yp][i] );
    }// i

    // Next do the horizontal synthesis
    for (j = yend-1;  j >= yp ; --j)
    {
        // First lifting stage 
        line_data = &pic_data[j][xp];

        predict.Filter( line_data[xl-2] , line_data[xl-3] , line_data[xl-1] ); 
        update.Filter( line_data[xl-1] , line_data[xl-2] , line_data[xl-2] );

        for ( k = xl-3;  k > 1; k-=2)
        { 
            predict.Filter( line_data[k-1] , line_data[k-2] , line_data[k] );
            update.Filter( line_data[k] , line_data[k+1] , line_data[k-1] );
        }// i

        predict.Filter( line_data[0] , line_data[1] , line_data[1] );
        update.Filter( line_data[1] , line_data[2] , line_data[0] );

    }
    _mm_empty();
}

#endif
