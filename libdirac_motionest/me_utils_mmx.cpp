/* ***** BEGIN LICENSE BLOCK *****
*
* $Id$
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
* The Original Code is contributed by Peter Meerwald.
*
* The Initial Developer of the Original Code is Peter Meerwald.
* Portions created by the Initial Developer are Copyright (C) 2004.
* All Rights Reserved.
*
* Contributor(s): Peter Meerwald (Original Author)
*                 Anuradha Suraparaju
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

#include <libdirac_motionest/me_utils_mmx.h>

#if defined HAVE_MMX
using namespace dirac;

namespace dirac
{
    CalcValueType simple_block_diff_mmx_4(
            const BlockDiffParams& dparams, const MVector& mv, 
            const PicArray& pic_data, const PicArray& ref_data)
    {
        __m64 sum = _mm_set_pi32(0, 0);

        int last_idx = (dparams.Xl()%4) ? (dparams.Xl()+4 - (dparams.Xl()%4)) :
                            dparams.Xl();

        for (int j=dparams.Yp() ; j != dparams.Yp()+dparams.Yl() ; ++j )
        {
            for(int i=dparams.Xp() ; i!= dparams.Xp()+last_idx ; i += 4 )
            {
                __m64 pic = *(__m64 *)&pic_data[j][i];
                __m64 ref = *(__m64 *)&ref_data[j+mv.y][i+mv.x];
                // pic - ref
                pic = _mm_sub_pi16 (pic, ref);
                // abs (pic - ref)
                ref = _mm_srai_pi16(pic, 15);
                pic = _mm_xor_si64(pic, ref);
                pic = _mm_sub_pi16 (pic, ref);
                // sum += abs(pic -ref)
                ref = _mm_xor_si64(ref, ref);
                ref = _mm_unpackhi_pi16(pic, ref);
                pic = _mm_unpacklo_pi16(pic, pic);
                pic = _mm_srai_pi32 (pic, 16);
                //ref = _mm_srai_pi32 (ref, 16);
                if (i + 4 <= (dparams.Xp()+dparams.Xl()))
                {
                    pic = _mm_add_pi32 (pic, ref);
                }
                sum = _mm_add_pi32 (sum, pic);
            }
        }
        int *result = (int *) &sum;
        _mm_empty();

        return result[0] + result[1];
    }

    CalcValueType simple_block_diff_up_mmx_4(
            const PicArray& pic_data, const PicArray& ref_data, 
            const ImageCoords& start_pos, const ImageCoords& end_pos, 
            const ImageCoords &ref_start, const ValueType weights[4])
    {
        __m64 sum = _mm_set_pi32(0, 0);
        __m64 eight = _mm_set_pi32(8, 8);

        __m64 weights1 = _mm_set_pi16(weights[0], weights[1], weights[0], weights[1]);
        __m64 weights2 = _mm_set_pi16(weights[2], weights[3], weights[2], weights[3]);

        for(int c = start_pos.y, uY = ref_start.y; c < end_pos.y; ++c, uY += 2)
        {
            ValueType *p1 = &ref_data[uY][ref_start.x];
            ValueType *p2 = &ref_data[uY+1][ref_start.x];

            for(int l = start_pos.x; l < end_pos.x; l+=2, p1 += 4, p2 += 4)
            {
                __m64 ref1 = *(__m64 *)p1;
                __m64 ref2 = *(__m64 *)p2;
                // multiply by weights
                ref1 = _mm_madd_pi16 (ref1, weights1);
                // multiply by weights
                ref2 = _mm_madd_pi16 (ref2, weights2);
                // temp wo +8 or shr 4
                ref1 = _mm_add_pi32 (ref1, ref2);
                // add eight to temp
                ref1 = _mm_add_pi32 (ref1, eight);
                // shift temp >>4
                ref1 = _mm_srai_pi32 (ref1, 4);

                // load pic_data
                ref2 = _mm_xor_si64(ref2, ref2);
                ref2 = _mm_unpacklo_pi16 (*(__m64 *)&pic_data[c][l], ref2);
                // pic_data - temp
                ref1 =_mm_sub_pi32 (ref1, ref2);
                // abs (pic_data - temp)
                ref2 = _mm_srai_pi32(ref1, 31);
                ref1 = _mm_xor_si64 (ref1, ref2);
                ref1 = _mm_sub_pi32 (ref1, ref2);
                // sum += abs(pic_data - temp)
                sum = _mm_add_pi32 (sum, ref1);
            }//l
        }//c

        _mm_empty();

        int *result = (int *) &sum;
        return result[0] + result[1];
    }

    inline void check_active_columns(
            int x, int xmax, ValueType act_cols1[4], 
            ValueType act_cols2[4], ValueType *row1, ValueType *row2)
    {
        // check if we need any clipping
        if (x >= 0 && (x+3) < xmax) {
            // special case, nothing to do
            memcpy(act_cols1, &row1[x], 4 * sizeof(ValueType));
            memcpy(act_cols2, &row2[x], 4 * sizeof(ValueType));
        }
        else {
            act_cols1[0] = row1[BChk(x,xmax)];
            act_cols2[0] = row2[BChk(x,xmax)];
            act_cols1[1] = row1[BChk(x+1,xmax)];
            act_cols2[1] = row2[BChk(x+1,xmax)];
            act_cols1[2] = row1[BChk(x+2,xmax)];
            act_cols2[2] = row2[BChk(x+2,xmax)];
            act_cols1[3] = row1[BChk(x+3,xmax)];
            act_cols2[3] = row2[BChk(x+3,xmax)];
        }
    }

    CalcValueType bchk_block_diff_up_mmx_2(
            const PicArray& pic_data, const PicArray& ref_data, 
            const ImageCoords& start_pos, const ImageCoords& end_pos, 
            const ImageCoords &ref_start, const ValueType weights[4])
    {
        const int double_xdim=ref_data.LengthX();
        const int double_ydim=ref_data.LengthY();

        __m64 sum = _mm_set_pi32(0, 0);
        __m64 eight = _mm_set_pi32(8, 8);
        
        __m64 weights1 = _mm_set_pi16(weights[0], weights[1], weights[0], weights[1]);
        __m64 weights2 = _mm_set_pi16(weights[2], weights[3], weights[2], weights[3]);

        ValueType active_columns1[4];
        ValueType active_columns2[4];

        for(int c = start_pos.y, uY = ref_start.y; c < end_pos.y; ++c, uY += 2)
        {
            for(int l = start_pos.x, uX = ref_start.x; l < end_pos.x; l+=2, uX += 4)
            {
                check_active_columns(uX, double_xdim, active_columns1, active_columns2, ref_data[BChk(uY,double_ydim)], ref_data[BChk(uY+1,double_ydim)]);

                __m64 col1 = *(__m64 *)active_columns1;
                __m64 col2 = *(__m64 *)active_columns2;
                // multiply ref_data by weights
                col1 = _mm_madd_pi16 (col1, weights1);
                // multiply ref_data by weights
                col2 = _mm_madd_pi16 (col2, weights2);
                // temp. value 1 w/o add-8, shr-4
                col1 = _mm_add_pi32 (col1, col2);
                // add 8
                col1 = _mm_add_pi32 (col1, eight);
                // shift right by 4
                col1 = _mm_srai_pi32 (col1, 4);

                // load pic_data
                col2 = _mm_xor_si64(col2, col2);
                col2 = _mm_unpacklo_pi16 (*(__m64 *)&pic_data[c][l], col2);

                // pic_data - temp
                col1 =_mm_sub_pi32 (col1, col2);
                // calculate absolute value
                col2 = _mm_srai_pi32(col1, 31);
                col1 = _mm_xor_si64 (col1, col2);
                col1 = _mm_sub_pi32 (col1, col2);

                // sum += abs (pic_data - temp)
                sum = _mm_add_pi32 (sum, col1);
            }//l
        }//c

        _mm_empty();

        int *result = (int *) &sum;
        return result[0] + result[1];
    }


    CalcValueType bibchk_block_diff_up_mmx_2(
            const PicArray& pic_data, const PicArray& ref_data1, 
            const PicArray& ref_data2, const ImageCoords& start_pos, 
            const ImageCoords& end_pos, const ImageCoords &ref_start1, 
            const ImageCoords &ref_start2, const ValueType weights[2][4])
    {
        //as above, but with bounds checking
        const int xmax1 = ref_data1.LengthX();
        const int ymax1 = ref_data1.LengthY();
        const int xmax2 = ref_data2.LengthX();
        const int ymax2 = ref_data2.LengthY();

        __m64 sum = _mm_set_pi32(0, 0);
        __m64 sixteen = _mm_set_pi32(16, 16);
        __m64 temp;

        __m64 weights1 = _mm_set_pi16(weights[0][0], weights[0][1], weights[0][0], weights[0][1]);
        __m64 weights2 = _mm_set_pi16(weights[1][0], weights[1][1], weights[1][0], weights[1][1]);
        __m64 weights3 = _mm_set_pi16(weights[0][2], weights[0][3], weights[0][2], weights[0][3]);
        __m64 weights4 = _mm_set_pi16(weights[1][2], weights[1][3], weights[1][2], weights[1][3]);

        ValueType active_columns1[4];
        ValueType active_columns2[4];
        ValueType active_columns3[4];
        ValueType active_columns4[4];

        for(int c = start_pos.y, uY1 = ref_start1.y,uY2=ref_start2.y; c < end_pos.y; ++c, uY1 += 2,uY2 += 2)
        {
            for(int l = start_pos.x, uX1 = ref_start1.x,uX2=ref_start2.x; l < end_pos.x; l+=2, uX1 += 4, uX2 += 4)
            {
                check_active_columns(uX1, xmax1, active_columns1, active_columns2, ref_data1[BChk(uY1,ymax1)], ref_data1[BChk(uY1+1,ymax1)]);

                check_active_columns(uX2, xmax2, active_columns3, active_columns4, ref_data2[BChk(uY2,ymax2)], ref_data2[BChk(uY2+1,ymax2)]);

                __m64 col1 = *(__m64 *)active_columns1;
                __m64 col2 = *(__m64 *)active_columns2;
                // multiply ref_data by weights
                col1 = _mm_madd_pi16 (col1, weights1);
                // multiply ref_data by weights
                col2 = _mm_madd_pi16 (col2, weights2);
                // temp. value 1 w/o add-16, shr-5
                temp = _mm_add_pi32 (col1, col2);
                // add 16
                temp = _mm_add_pi32 (temp, sixteen);
                // shift right by 5
                temp = _mm_srai_pi32 (temp, 5);

                col1 = *(__m64 *)active_columns3;
                col2 = *(__m64 *)active_columns4;
                // multiply ref_data by weights
                col1 = _mm_madd_pi16 (col1, weights3);
                // multiply ref_data by weights
                col2 = _mm_madd_pi16 (col2, weights4);
                // temp. value 1 w/o add-16, shr-5
                col1 = _mm_add_pi32 (col1, col2);
                // add 16
                col1 = _mm_add_pi32 (col1, sixteen);
                // shift right by 5
                col1 = _mm_srai_pi32 (col1, 5);
                temp = _mm_add_pi32 (temp, col1);

                // load pic_data
                col1 = _mm_xor_si64(col1, col1);
                col1 = _mm_unpacklo_pi16 (*(__m64 *)&pic_data[c][l], col1);
                // pic_data - temp
                col1 =_mm_sub_pi32 (col1, temp);
                // calculate absolute value
                col2 = _mm_srai_pi32(col1, 31);
                col1 = _mm_xor_si64 (col1, col2);
                col1 = _mm_sub_pi32 (col1, col2);

                // sum += abs (pic_data - temp)
                sum = _mm_add_pi32 (sum, col1);

            }//l
        }//c

        _mm_empty();

        int *result = (int *) &sum;
        return result[0] + result[1];
    }
}
#endif
