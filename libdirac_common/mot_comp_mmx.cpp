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

#if defined(HAVE_MMX)
#include <mmintrin.h>
#include <libdirac_common/mot_comp.h>
#include <libdirac_common/mot_comp_mmx.h>
#include <libdirac_common/motion.h>
#include <libdirac_common/dirac_assertions.h>
using namespace dirac;

void MotionCompensator_QuarterPixel::CompensateBlock( TwoDArray<CalcValueType> &pic_data , 
                                   const ImageCoords& orig_pic_size , 
                                   const PicArray &refup_data , 
                                   const MVector &mv , 
                                   const ImageCoords& pos , 
                                   const TwoDArray<ValueType>& wt_array )
{
    //Coordinates in the image being written to.
    const ImageCoords start_pos( std::max(pos.x,0) , std::max(pos.y,0) );
    const ImageCoords end_pos( std::min( pos.x + m_bparams.Xblen() , orig_pic_size.x ) , 
                               std::min( pos.y + m_bparams.Yblen() , orig_pic_size.y ) );

    //The difference between the desired start point
    //pos and the actual start point start_pos.
    const ImageCoords diff( start_pos.x - pos.x , start_pos.y - pos.y );

    // Set up the start point in the reference image by rounding the motion vector
    // to 1/2 pel accuracy.NB: bit shift rounds negative values DOWN, as required
    const MVector roundvec( mv.x>>1 , mv.y>>1 );

    //Get the remainder after rounding. NB rmdr values always 0 or 1
    const MVector rmdr( mv.x & 1 , mv.y & 1 );

    //Where to start in the upconverted image
    const ImageCoords ref_start( ( start_pos.x<<1 ) + roundvec.x ,( start_pos.y<<1 ) + roundvec.y );

    //An additional stage to make sure the block to be copied does not fall outside
    //the reference image.
    const int refXlen = refup_data.LengthX();
    const int refYlen = refup_data.LengthY();
    CalcValueType *pic_curr = &pic_data[0][start_pos.x];
    ValueType *wt_curr = &wt_array[diff.y][diff.x];

    const int block_width = end_pos.x - start_pos.x;

    const int pic_next( pic_data.LengthX() - block_width ); //go down a row and back to beginning of block line
    const int wt_next( wt_array.LengthX() - block_width ); //go down a row and back to beginning of block line

    bool do_bounds_checking = false;

    //Check if there are going to be any problems copying the block from
    //the upvconverted reference image.
    if( ref_start.x < 0 ) 
        do_bounds_checking = true;
    else if( ref_start.x + ((end_pos.x - start_pos.x)<<1 ) >= refXlen )
        do_bounds_checking = true;
    if( ref_start.y < 0 ) 
        do_bounds_checking = true;
    else if( ref_start.y + ((end_pos.y - start_pos.y)<<1 ) >= refYlen)
        do_bounds_checking = true;

    if( !do_bounds_checking )
    {
#ifdef _MSC_VER
        // The following mmx routines try to access beyond the end of the
        // wt_array. The uninitialsed values read are not used. But 
        // MSVC++ crashes when we try to read beyond end of array even though
        // we do not attempt to write beyond end of array. This is a kludge
        // to fix it
        int stopX = 3;
#else
        int stopX = 1;
#endif
        ValueType *refup_curr = &refup_data[ref_start.y][ref_start.x];
        const int refup_next( ( refXlen - block_width )*2 ); //go down 2 rows and back to beginning of block line
        if( rmdr.x == 0 && rmdr.y == 0 )
        {
            __m64 tmp1 = _mm_set_pi16 (0, 0, 0, 0);

            for( int y=end_pos.y-start_pos.y; y > 0; --y, pic_curr+=pic_next, wt_curr+=wt_next, refup_curr+=refup_next )
            {
                int x;
                for( x=block_width; x > stopX; x-=2, pic_curr+=2, wt_curr+=2, refup_curr+=4 )
                {
                    tmp1 = _mm_xor_si64 (tmp1, tmp1);
                    tmp1 = _mm_unpacklo_pi16 (*(__m64 *)wt_curr, tmp1);
                    tmp1 = _mm_madd_pi16 (*(__m64 *)refup_curr, tmp1);
                    *(__m64 *)pic_curr = _mm_add_pi32 (*(__m64 *)pic_curr, tmp1);
                }
                // Mopup the last value
                for ( ; x > 0; --x)
                {
                    *pic_curr += CalcValueType( *refup_curr )* *wt_curr;
                    ++pic_curr;
                    ++wt_curr;
                    refup_curr+=2;
                }
            }
            _mm_empty();
        }
        else if( rmdr.y == 0 )
        {
            __m64 mul = _mm_set_pi16 (1, 1, 1, 1);
            __m64 round = _mm_set_pi32 (1, 1);
            __m64 tmp1;
            __m64 tmp2 = _mm_set_pi16 (0, 0, 0, 0);

            for( int y=end_pos.y-start_pos.y; y > 0; --y, pic_curr+=pic_next, wt_curr+=wt_next, refup_curr+=refup_next )
            {
                int x;
                for( x=block_width; x > stopX; x-=2, pic_curr+=2, wt_curr+=2, refup_curr+=4 )
                {
                    tmp1 = _mm_madd_pi16 (*(__m64 *)refup_curr, mul);
                    tmp1 = _mm_add_pi32 (tmp1, round);
                    tmp1 = _mm_srai_pi32 (tmp1, 1);
                    tmp2 = _mm_xor_si64(tmp2, tmp2);
                    tmp2 = _mm_unpacklo_pi16 (*(__m64 *)wt_curr, tmp2);
                    tmp1 = _mm_madd_pi16 (tmp1, tmp2);
                    *(__m64 *)pic_curr = _mm_add_pi32 (*(__m64 *)pic_curr, tmp1);
                }

                // Mopup the last value
                for ( ; x > 0; --x)
                {
                    *pic_curr += ((    *refup_curr  +
                                       *(refup_curr+1)  + 1
                                  ) >> 1) * CalcValueType(*wt_curr);
                    ++pic_curr;
                    ++wt_curr;
                    refup_curr+=2;
                }
            }
            _mm_empty();
        }
        else if( rmdr.x == 0 )
        {
            __m64 mul = _mm_set_pi16 (0, 1, 0, 1);
            __m64 round = _mm_set_pi32 (1, 1);
            __m64 tmp1;
            __m64 tmp2 = _mm_set_pi16 (0, 0, 0, 0);
            for( int y=end_pos.y-start_pos.y; y > 0; --y, pic_curr+=pic_next, wt_curr+=wt_next, refup_curr+=refup_next )
            {
                int x;
                for( x = block_width; x > stopX; x-=2, pic_curr+=2, wt_curr+=2, refup_curr+=4 )
                {
                    tmp1 = _mm_add_pi16 (*(__m64 *)refup_curr, *(__m64 *)(refup_curr+refXlen));
                    tmp1 = _mm_madd_pi16 (tmp1, mul);
                    tmp1 = _mm_add_pi32 (tmp1, round);
                    tmp1 = _mm_srai_pi32 (tmp1, 1);
                    tmp2 = _mm_xor_si64(tmp2, tmp2);
                    tmp2 = _mm_unpacklo_pi16 (*(__m64 *)wt_curr, tmp2);
                    tmp2 = _mm_madd_pi16 (tmp1, tmp2);
                    *(__m64 *)pic_curr = _mm_add_pi32 (*(__m64 *)pic_curr, tmp2);
                }
                for ( ; x > 0; --x)
                {
                    *pic_curr += ((    *refup_curr + *(refup_curr+refXlen) +
                                       1
                                   ) >> 1) * CalcValueType(*wt_curr);
                    ++pic_curr;
                    ++wt_curr;
                    refup_curr+=2;
                }
            }
            _mm_empty();
        }
        else
        {
            __m64 mul = _mm_set_pi16 (1, 1, 1, 1);
            __m64 round = _mm_set_pi32 (2, 2);
            __m64 tmp1;
            __m64 tmp2 = _mm_set_pi16 (0, 0, 0, 0);
            for( int y=end_pos.y-start_pos.y; y > 0; --y, pic_curr+=pic_next, wt_curr+=wt_next, refup_curr+=refup_next )
            {
                int x;
                for( x = block_width; x > stopX; x-=2, pic_curr+=2, wt_curr+=2, refup_curr+=4 )
                {
                    tmp1 = _mm_add_pi16 (*(__m64 *)refup_curr, *(__m64 *)(refup_curr+refXlen));
                    tmp1 = _mm_madd_pi16 (tmp1, mul);
                    tmp1 = _mm_add_pi32 (tmp1, round);
                    tmp1 = _mm_srai_pi32 (tmp1, 2);
                    tmp2 = _mm_xor_si64(tmp2, tmp2);
                    tmp2 = _mm_unpacklo_pi16 (*(__m64 *)wt_curr, tmp2);
                    tmp2 = _mm_madd_pi16 (tmp1, tmp2);
                    *(__m64 *)pic_curr = _mm_add_pi32 (*(__m64 *)pic_curr, tmp2);
                }
                for ( ; x > 0; --x)
                {
                    *pic_curr += ((    *refup_curr  +
                                       *(refup_curr+1)  +
                                       *(refup_curr+refXlen)  +
                                       *(refup_curr+refXlen+1)  +
                                       2
                                   ) >> 2) * CalcValueType(*wt_curr);
                    ++pic_curr;
                    ++wt_curr;
                    refup_curr+=2;
                }
            }
            _mm_empty();
        }
    }
    else
    {
        // We're doing bounds checking because we'll fall off the edge of the reference otherwise.

        //weights for doing linear interpolation, calculated from the remainder values
        const ValueType linear_wts[4] = {  (2 - rmdr.x) * (2 - rmdr.y),    //tl
                                           rmdr.x * (2 - rmdr.y),          //tr
                                           (2 - rmdr.x) * rmdr.y,          //bl
                                           rmdr.x * rmdr.y };              //br


       for(int c = 0, wY = diff.y, uY = ref_start.y,BuY=BChk(uY,refYlen),BuY1=BChk(uY+1,refYlen);
           c < end_pos.y - start_pos.y; ++c, ++wY, uY += 2,BuY=BChk(uY,refYlen),BuY1=BChk(uY+1,refYlen))
       {
           for(int l = start_pos.x, wX = diff.x, uX = ref_start.x,BuX=BChk(uX,refXlen),BuX1=BChk(uX+1,refXlen);
               l < end_pos.x; ++l, ++wX, uX += 2,BuX=BChk(uX,refXlen),BuX1=BChk(uX+1,refXlen))
           {

               pic_data[c][l] += ((     linear_wts[0] * CalcValueType( refup_data[BuY][BuX] ) +
                                        linear_wts[1] * CalcValueType( refup_data[BuY][BuX1] ) +
                                        linear_wts[2] * CalcValueType( refup_data[BuY1][BuX] )+
                                        linear_wts[3] * CalcValueType( refup_data[BuY1][BuX1] ) +
                                        2
                                  ) >> 2) * wt_array[wY][wX];
           }//l
       }//c

    }
}

namespace dirac
{
    void CompensateComponentAddAndShift_mmx (int start_y, int end_y, 
                                           TwoDArray<CalcValueType> &comp_data, 
                                           PicArray &pic_data_out)
    {

        int stopX = pic_data_out.FirstX() + ((pic_data_out.LengthX()>>2)<<2);
        __m64 max_val = _mm_set_pi32 (1024, 1024);
           CalcValueType *pic_row = &comp_data[0][comp_data.FirstX()];
           ValueType *out_row = &pic_data_out[start_y][pic_data_out.FirstX()];
        for ( int i = start_y; i < end_y; i++)
        {
            for ( int j =  pic_data_out.FirstX(); j < stopX; j+=4)
            {
                __m64 in1 = *(__m64 *)pic_row;
                in1 = _mm_add_pi32 (in1, max_val);
                in1 = _mm_srai_pi32 (in1, 11);
                __m64 in2 = *(__m64 *)(pic_row+2);
                in2 = _mm_add_pi32 (in2, max_val);
                in2 = _mm_srai_pi32 (in2, 11);
                in1 = _mm_packs_pi32 (in1, in2);
                __m64 *out = (__m64 *)out_row;
                *out = _mm_add_pi16 (in1, *out);
                pic_row += 4;
                out_row += 4;
            }
               for ( int j =stopX; j <= pic_data_out.LastX(); j++)
               {
                   *out_row += static_cast<ValueType>( (*pic_row + 1024) >> 11 ); 
                ++out_row;
                ++pic_row;
               }
         }
        _mm_empty();
   }
}
#endif
