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
* Contributor(s): Richard Felton (Original Author), Thomas Davies
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


#include <libdirac_common/mot_comp.h>
#include <libdirac_common/motion.h>
#include <libdirac_common/frame_buffer.h>

using std::vector;

//--public member functions--//
///////////////////////////////


//Constructor
//Initialises the lookup table that is needed for 1/8th pixel accuracy
//motion estimation. Creates the necessary arithmetic objects and
//calls ReConfig to create weighting blocks to fit the values within
//cparams.
MotionCompensator::MotionCompensator(const CodecParams &cp): 
    cparams(cp),
    luma_or_chroma(true),
    add_or_sub(SUBTRACT)
{
    //Configure weighting blocks for the first time
    m_block_weights = NULL;
    ReConfig();
}

//Destructor
MotionCompensator::~MotionCompensator(){

    //Tidy up the pointers
    delete[] m_block_weights;
}

//Called to perform motion compensated addition/subtraction on an entire frame.
void MotionCompensator::CompensateFrame(FrameBuffer& my_buffer,int fnum,const MvData& mv_data)
{

     int ref1_idx,ref2_idx;    
     Frame& my_frame=my_buffer.GetFrame(fnum);
     const ChromaFormat& cformat=my_frame.GetFparams().CFormat();
     const FrameSort& fsort=my_frame.GetFparams().FSort();

     if (fsort!=I_frame)
     {//we can motion compensate

         const vector<int>& refs=my_frame.GetFparams().Refs();
         if (refs.size()>0)
         {
             //extract the references
             ref1_idx=refs[0];
             if (refs.size()>1)
                 ref2_idx=refs[1];
             else
                 ref2_idx=refs[0];

             const Frame& ref1frame=my_buffer.GetFrame(ref1_idx);
             const Frame& ref2frame=my_buffer.GetFrame(ref2_idx);

             //now do all the components
             CompensateComponent( my_frame , ref1frame , ref2frame , mv_data , Y_COMP);

             if (cformat != Yonly)
             {
                 luma_or_chroma=false;                
                 CompensateComponent( my_frame , ref1frame , ref2frame , mv_data , U_COMP);
                 CompensateComponent( my_frame , ref1frame , ref2frame , mv_data , V_COMP);
             }
         }
     }
}

//--private member functions--//
////////////////////////////////

//Needs to be called if the blocksize changes (and
//on startup). This method creates an array of weighting
//blocks that are used to acheive correctly overlapping
//blocks.
void MotionCompensator::ReConfig()
{
    if (luma_or_chroma)
        m_bparams=cparams.LumaBParams(2);
    else
        m_bparams=cparams.ChromaBParams(2);

    if(m_block_weights != NULL)
        delete[] m_block_weights;

    // Create new weights array.
    m_block_weights = new TwoDArray<CalcValueType>[9];
    for(int i = 0; i < 9; i++)
        m_block_weights[i] = *(new TwoDArray<CalcValueType>(  m_bparams.Yblen() , m_bparams.Xblen() ));

    // We can create all nine weighting blocks by calculating values
    // for four blocks and mirroring them to generate the others.
    CreateBlock( m_bparams , false , false , m_block_weights[0] );
    CreateBlock( m_bparams , false , true , m_block_weights[3] );
    CreateBlock( m_bparams , true , false , m_block_weights[1] );
    CreateBlock( m_bparams , true , true , m_block_weights[4] );

    // Note order of flipping is important.    
    FlipX( m_block_weights[3] , m_bparams , m_block_weights[5] );
    FlipX( m_block_weights[0] , m_bparams , m_block_weights[2] );
    FlipY( m_block_weights[0] , m_bparams , m_block_weights[6] );
    FlipX( m_block_weights[6] , m_bparams , m_block_weights[8] );
    FlipY( m_block_weights[1] , m_bparams , m_block_weights[7] );

}

void MotionCompensator::CompensateComponent(Frame& picframe, const Frame &ref1frame, const Frame& ref2frame,
    const MvData& mv_data,const CompSort cs)
{

    // Set up references to pictures and references
    PicArray& pic_data = picframe.Data(cs);
    const PicArray& ref1up = ref1frame.UpData(cs);
    const PicArray& ref2up = ref2frame.UpData(cs);

    // Reference to the relevant DC array
    const TwoDArray<ValueType>& dcarray = mv_data.DC(cs);

    // Set up references to the vectors
    const int num_refs = picframe.GetFparams().Refs().size();
    const MvArray* mv_array1; 
    const MvArray* mv_array2;
    mv_array1 = &mv_data.Vectors(1);
    if (num_refs ==2 )
        mv_array2 = &mv_data.Vectors(2);
    else
        mv_array2 = &mv_data.Vectors(1);

    ReConfig();//set all the weighting blocks up    

    //Blocks are listed left to right, line by line.
    MVector mv1,mv2;
    PredMode block_mode;
    ValueType dc;

    //Coords of the top-left corner of a block
    ImageCoords pos;

    //Loop for each block in the output image.
    //The CompensateBlock function will use the image pointed to by ref1up
    //and add the compensated pixels to the image pointed to by pic_data.
    size_t wgt_idx;

    //Loop over all the block rows

    pos.y = -m_bparams.Yoffset();
    for(int yblock = 0; yblock < cparams.YNumBlocks(); ++yblock)
    {
        pos.x = -m_bparams.Xoffset();
        //loop over all the blocks in a row
        for(int xblock = 0 ; xblock < cparams.XNumBlocks(); ++xblock)
        {

            //Decide which weights to use.
            if((xblock != 0)&&(xblock < cparams.XNumBlocks() - 1))
            {
                if((yblock != 0)&&(yblock < cparams.YNumBlocks() - 1))    
                    wgt_idx = 4;
                else if(yblock == 0) 
                    wgt_idx = 1;
                else 
                    wgt_idx= 7;
            }
            else if(xblock == 0)
            {
                if((yblock != 0)&&(yblock < cparams.YNumBlocks() - 1))    
                    wgt_idx= 3;
                else if(yblock == 0) 
                    wgt_idx= 0;
                else 
                    wgt_idx= 6;
            }
            else
            {
                if((yblock != 0)&&(yblock < cparams.YNumBlocks() - 1))    
                    wgt_idx= 5;
                else if(yblock == 0) 
                    wgt_idx= 2;
                else 
                    wgt_idx= 8;
            }

            block_mode = mv_data.Mode()[yblock][xblock];
            mv1 = (*mv_array1)[yblock][xblock];
            mv2 = (*mv_array2)[yblock][xblock];
            dc = dcarray[yblock][xblock]<<2;// DC is only given 8 bits, 
                                            // so need to shift to get 10-bit data

            if(block_mode == REF1_ONLY)
            {
                if(add_or_sub == ADD) 
                    CompensateBlock(pic_data, ref1up, mv1, pos, m_block_weights[wgt_idx], m_add);
                else
                    CompensateBlock(pic_data, ref1up, mv1, pos, m_block_weights[wgt_idx], m_subtract);
            }
            else if (block_mode == REF2_ONLY)
            {                
                if(add_or_sub == ADD)
                    CompensateBlock(pic_data, ref2up, mv2, pos, m_block_weights[wgt_idx],m_add);
                else 
                    CompensateBlock(pic_data, ref2up, mv2, pos, m_block_weights[wgt_idx], m_subtract);
            }
            else if(block_mode == REF1AND2)
            {
                if(add_or_sub == ADD){
                    CompensateBlock(pic_data, ref1up, mv1, pos, m_block_weights[wgt_idx], m_addhalf);
                    CompensateBlock(pic_data, ref2up, mv2, pos, m_block_weights[wgt_idx], m_addhalf);                    
                }
                else
                {
                    CompensateBlock(pic_data, ref1up, mv1, pos, m_block_weights[wgt_idx], m_subtracthalf);
                    CompensateBlock(pic_data, ref2up, mv2, pos, m_block_weights[wgt_idx], m_subtracthalf);
                }
            }
            else
            {//we have a DC block.
                if(add_or_sub == ADD)
                    DCBlock(pic_data, dc,pos, m_block_weights[wgt_idx], m_add);
                else
                    DCBlock(pic_data, dc,pos, m_block_weights[wgt_idx], m_subtract);
            }

            //Increment the block horizontal position
            pos.x += m_bparams.Xbsep();

        }//xblock

        // Okay, we've done all the actual blocks. Now if the picture is further padded
        // we need to set the padded values to zero beyond the last block in the row,
        // for all the picture lines in the block row. Need only do this when we're
        // subtracting.
        if (add_or_sub==SUBTRACT)
        {

            for ( int y=yblock*m_bparams.Ybsep() ; y<(yblock+1)*m_bparams.Ybsep() ; ++y )
                for (int x=( cparams.XNumBlocks()*m_bparams.Xbsep() ); x<pic_data.LengthX() ; ++x )
                    pic_data[y][x] = 0;

        }//?add_or_sub

        //Increment the block vertical position
        pos.y += m_bparams.Ybsep();

    }//yblock

    // Finally, now we've done all the blocks, we must set all padded lines below 
    // the last row equal to 0, if we're subtracting
    if (add_or_sub == SUBTRACT)
    {

        for ( int y=cparams.YNumBlocks()*m_bparams.Ybsep() ; y<pic_data.LengthY() ; ++y )
            for ( int x=0 ; x<pic_data.LengthX() ; ++x )
                pic_data[y][x] = 0;

    }//?add_or_sub

}

void MotionCompensator::CompensateBlock( PicArray &pic_data , const PicArray &refup_data , const MVector &mv , 
const ImageCoords& pos , const TwoDArray<CalcValueType>& wt_array , const ArithObj& arith )
{

    //Coordinates in the image being written to.
    const ImageCoords start_pos( std::max(pos.x,0) , std::max(pos.y,0) );
    const ImageCoords end_pos( std::min( pos.x + m_bparams.Xblen() , pic_data.LengthX() ) , 
                               std::min( pos.y + m_bparams.Yblen() , pic_data.LengthY() ) );

    //The difference between the desired start point
    //pos and the actual start point start_pos.
    const ImageCoords diff( start_pos.x - pos.x , start_pos.y - pos.y );

    //Set up the start point in the reference image by rounding the motion vector
    //NB: bit shift rounds negative values DOWN, as required
    const MVector roundvec( mv.x>>2 , mv.y>>2 );

    //Get the remainder after rounding. NB rmdr values always 0,1,2 or 3
    const MVector rmdr( mv.x - ( roundvec.x<<2 ) , mv.y - ( roundvec.y<<2 ) );

    //Where to start in the upconverted image
    const ImageCoords ref_start( ( start_pos.x<<1 ) + roundvec.x ,( start_pos.y<<1 ) + roundvec.y );

    //weights for doing linear interpolation, calculated from the remainder values
    const ValueType TLweight( (4 - rmdr.x) * (4 - rmdr.y) );
    const ValueType TRweight( rmdr.x * ( 4 - rmdr.y ) );
    const ValueType BLweight( ( 4 - rmdr.x ) * rmdr.y );
    const ValueType BRweight( rmdr.x * rmdr.y );

    //An additional stage to make sure the block to be copied does not fall outside
    //the reference image.
    const int refXlen = refup_data.LengthX();
    const int refYlen = refup_data.LengthY();
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

    //Temporary Variable.
    CalcValueType temp;

     if( !do_bounds_checking )
     {
         for(int c = start_pos.y, wY = diff.y, uY = ref_start.y; c < end_pos.y; ++c, ++wY, uY += 2)
         {
             for(int l = start_pos.x, wX = diff.x, uX = ref_start.x; l < end_pos.x; ++l, ++wX, uX += 2)
             {

                 temp = ( TLweight * refup_data[uY][uX] +
                          TRweight * refup_data[uY][uX+1] +
                          BLweight * refup_data[uY+1][uX] +
                          BRweight * refup_data[uY+1][uX+1] +
                          8
                          ) >> 4;

                 arith.DoArith( pic_data[c][l] , CalcValueType(temp) , wt_array[wY][wX] );

             }//l
         }//c
     }
     else
     {
         //We're doing bounds checking because we'll fall off the edge of the reference otherwise.

        for(int c = start_pos.y, wY = diff.y, uY = ref_start.y,BuY=BChk(uY,refYlen),BuY1=BChk(uY+1,refYlen);
            c < end_pos.y; ++c, ++wY, uY += 2,BuY=BChk(uY,refYlen),BuY1=BChk(uY+1,refYlen))
        {
            for(int l = start_pos.x, wX = diff.x, uX = ref_start.x,BuX=BChk(uX,refXlen),BuX1=BChk(uX+1,refXlen);
                l < end_pos.x; ++l, ++wX, uX += 2,BuX=BChk(uX,refXlen),BuX1=BChk(uX+1,refXlen))
            {

                temp = ( TLweight * refup_data[BuY][BuX] +
                         TRweight * refup_data[BuY][BuX1]  +
                         BLweight * refup_data[BuY1][BuX]+
                         BRweight * refup_data[BuY1][BuX1] +
                         8
                         ) >> 4;

                arith.DoArith( pic_data[c][l] , CalcValueType(temp) , wt_array[wY][wX] );

            }//l
        }//c

    }

}

void MotionCompensator::DCBlock( PicArray &pic_data ,const ValueType dc , const ImageCoords& pos ,
    const TwoDArray<CalcValueType>& wt_array , const ArithObj& arith)
{

    //Coordinates in the image being written to.
    const ImageCoords start_pos( std::max(0 , pos.x) , std::max(0 , pos.y) );
    const ImageCoords end_pos( std::min(pos.x + m_bparams.Xblen() , pic_data.LengthX() ) , 
                               std::min(pos.y + m_bparams.Yblen() , pic_data.LengthY() ) );

    //The difference between the desired start point
    //pos and the actual start point start_pos.
    const ImageCoords diff(start_pos.x - pos.x , start_pos.y - pos.y);

    //Quick process where we can just copy from the double size image.

    for(int c = start_pos.y, wY = diff.y; c < end_pos.y; ++c, ++wY)
        for(int l = start_pos.x, wX = diff.x; l < end_pos.x; ++l, ++wX)
            arith.DoArith( pic_data[c][l] , CalcValueType( dc ) , wt_array[wY][wX]);

}
