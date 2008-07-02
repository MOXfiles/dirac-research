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
* Contributor(s): Thomas Davies (Original Author),
                  Scott R Ladd,
                  Tim Borer
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

#include <libdirac_common/common.h>
#include <algorithm>


//PicArray functions

PicArray::PicArray(int height, int width, CompSort cs):
    TwoDArray<ValueType>(height , width),
    m_csort(cs)
{
         //Nothing
}

const CompSort& PicArray::CSort() const 
{
    return m_csort;
}

void PicArray::SetCSort(const CompSort cs)
{
    m_csort=cs;
}    



//EntropyCorrector functions

EntropyCorrector::EntropyCorrector(int depth): 
     m_Yfctrs( 3 , 3*depth+1 ),
     m_Ufctrs( 3 , 3*depth+1 ),
     m_Vfctrs( 3 , 3*depth+1 )
{
    Init();
}

float EntropyCorrector::Factor(const int bandnum , const FrameSort fsort ,const CompSort c) const
{

    if (c == U_COMP)
        return m_Ufctrs[fsort][bandnum-1];
    else if (c == V_COMP)
        return m_Vfctrs[fsort][bandnum-1];
    else
        return m_Yfctrs[fsort][bandnum-1];
}

void EntropyCorrector::Init()
{

    //do I-frames
    for (int  i=0 ; i<m_Yfctrs.LengthX() ; ++i )
    {
        if ( i == m_Yfctrs.LastX() )
        {        
            m_Yfctrs[I_frame][i] = 1.0;
            m_Ufctrs[I_frame][i] = 1.0;
            m_Vfctrs[I_frame][i] = 1.0;
            m_Yfctrs[L1_frame][i] = 0.85;
            m_Ufctrs[L1_frame][i] = 0.85;
            m_Vfctrs[L1_frame][i] = 0.85;
            m_Yfctrs[L2_frame][i] = 0.85;
            m_Ufctrs[L2_frame][i] = 0.85;
            m_Vfctrs[L2_frame][i] = 0.85;
        }
        else if ( i >= m_Yfctrs.LastX()-3 )
        {
            m_Yfctrs[I_frame][i] = 0.85;
            m_Ufctrs[I_frame][i] = 0.85;
            m_Vfctrs[I_frame][i] = 0.85;
            m_Yfctrs[L1_frame][i] = 0.75;
            m_Ufctrs[L1_frame][i] = 0.75;
            m_Vfctrs[L1_frame][i] = 0.75;
            m_Yfctrs[L2_frame][i] = 0.75;
            m_Ufctrs[L2_frame][i] = 0.75;
            m_Vfctrs[L2_frame][i] = 0.75;            
        }
        else
        {
            m_Yfctrs[I_frame][i] = 0.75;
            m_Ufctrs[I_frame][i] = 0.75;
            m_Vfctrs[I_frame][i] = 0.75;
            m_Yfctrs[L1_frame][i] = 0.75;
            m_Ufctrs[L1_frame][i] = 0.75;
            m_Vfctrs[L1_frame][i] = 0.75;
            m_Yfctrs[L2_frame][i] = 0.75;
            m_Ufctrs[L2_frame][i] = 0.75;
            m_Vfctrs[L2_frame][i] = 0.75;            
        }
    }//i

}

void EntropyCorrector::Update(int bandnum , FrameSort fsort , CompSort c ,int est_bits , int actual_bits){
    //updates the factors - note that the estimated bits are assumed to already include the correction factor

    float multiplier;
    if (actual_bits != 0 && est_bits != 0)
        multiplier = float(actual_bits)/float(est_bits);
    else
        multiplier=1.0;
    if (c == U_COMP)
        m_Ufctrs[fsort][bandnum-1] *= multiplier;
    else if (c == V_COMP)
        m_Vfctrs[fsort][bandnum-1] *= multiplier;
    else
        m_Yfctrs[fsort][bandnum-1] *= multiplier;
}

// Overlapped block parameter functions

OLBParams::OLBParams(const int xblen, int const yblen, int const xbsep, int const ybsep):
    m_xblen(xblen),
    m_yblen(yblen),
    m_xbsep(xbsep),
    m_ybsep(ybsep),
    m_xoffset( (xblen-xbsep)/2 ),
    m_yoffset( (yblen-ybsep)/2 )
{}

std::ostream & operator<< (std::ostream & stream, OLBParams & params)
{
    stream << params.Ybsep() << " " << params.Xbsep();

    return stream;
}

std::istream & operator>> (std::istream & stream, OLBParams & params)
{
    int temp;
    stream >> temp;
    params.SetYbsep(temp);
    stream >> temp;
    params.SetXbsep(temp);

    return stream;
}

// Codec params functions

CodecParams::CodecParams():
    m_x_num_mb(0),
    m_y_num_mb(0),
    m_x_num_blocks(0),
    m_y_num_blocks(0),
    m_verbose(false),
    m_interlace(false),
    m_topfieldfirst(false),
    m_lbparams(3),
    m_cbparams(3)
{}



void CodecParams::SetBlockSizes(const OLBParams& olbparams , ChromaFormat cformat)
{
    //given the raw overlapped block parameters, set the modified internal parameters to
    //take account of the chroma sampling format and overlapping requirements, as well
    //as the equivalent parameters for sub-MBs and MBs.
    //Does NOT set the number of blocks or macroblocks, as padding may be required.

    m_lbparams[2] = olbparams;

    //check the separations aren't too small
    m_lbparams[2].SetXbsep( std::max(m_lbparams[2].Xbsep() , 4) );
    m_lbparams[2].SetYbsep( std::max(m_lbparams[2].Ybsep() , 4) );

    //check there's sufficient overlap    
    m_lbparams[2].SetXblen( std::max(m_lbparams[2].Xbsep()+2 , m_lbparams[2].Xblen()) );
    m_lbparams[2].SetYblen( std::max(m_lbparams[2].Ybsep()+2 , m_lbparams[2].Yblen()) );

    //check overlap is divisible by 2
    if (( m_lbparams[2].Xblen() - m_lbparams[2].Xbsep() )%2 != 0)
        m_lbparams[2].SetXblen( m_lbparams[2].Xblen()+1 );
    if (( m_lbparams[2].Yblen() - m_lbparams[2].Ybsep() )%2 != 0)
        m_lbparams[2].SetYblen( m_lbparams[2].Yblen()+1 );

    //Now compute the resulting chroma block params
    if (cformat == format420)
    {
        m_cbparams[2].SetXbsep( m_lbparams[2].Xbsep()/2 );
        m_cbparams[2].SetYbsep( m_lbparams[2].Ybsep()/2 );
        m_cbparams[2].SetXblen( std::max(m_lbparams[2].Xblen()/2 , m_cbparams[2].Xbsep()+2) );
        m_cbparams[2].SetYblen( std::max(m_lbparams[2].Yblen()/2 , m_cbparams[2].Ybsep()+2) );
    }
    else if (cformat == format422)
    {
        m_cbparams[2].SetXbsep( m_lbparams[2].Xbsep()/2 );
        m_cbparams[2].SetYbsep( m_lbparams[2].Ybsep() );
        m_cbparams[2].SetXblen( std::max(m_lbparams[2].Xblen()/2 , m_cbparams[2].Xbsep()+2) );
        m_cbparams[2].SetYblen( std::max(m_lbparams[2].Yblen() , m_cbparams[2].Ybsep()+2) );
    }
    else if (cformat==format411)
    {
        m_cbparams[2].SetXbsep( m_lbparams[2].Xbsep()/4 );
        m_cbparams[2].SetYbsep( m_lbparams[2].Ybsep() );
        m_cbparams[2].SetXblen( std::max(m_lbparams[2].Xblen()/4 , m_cbparams[2].Xbsep()+2) );
        m_cbparams[2].SetYblen( std::max(m_lbparams[2].Yblen() , m_cbparams[2].Ybsep()+2) );
    }
    else
    {// assume 444
        m_cbparams[2].SetXbsep( m_lbparams[2].Xbsep() );
        m_cbparams[2].SetYbsep( m_lbparams[2].Ybsep() );
        m_cbparams[2].SetXblen( std::max(m_lbparams[2].Xblen() , m_cbparams[2].Xbsep()+2) );
        m_cbparams[2].SetYblen( std::max(m_lbparams[2].Yblen() , m_cbparams[2].Ybsep()+2) );
    }

    //check overlap is divisible by 2
    if (( m_cbparams[2].Xblen() - m_cbparams[2].Xbsep() )%2 != 0)
        m_cbparams[2].SetXblen( m_cbparams[2].Xblen()+1 );
    if (( m_cbparams[2].Yblen() - m_cbparams[2].Ybsep() )%2 != 0)
        m_cbparams[2].SetYblen( m_cbparams[2].Yblen()+1 );

    //Now work out the overlaps for splitting levels 1 and 0
    m_lbparams[1].SetXbsep( m_lbparams[2].Xbsep()*2 );
    m_lbparams[1].SetXblen( m_lbparams[2].Xblen() + m_lbparams[2].Xbsep() );
    m_lbparams[1].SetYbsep( m_lbparams[2].Ybsep()*2 );
    m_lbparams[1].SetYblen( m_lbparams[2].Yblen() + m_lbparams[2].Xbsep() );

    m_lbparams[0].SetXbsep( m_lbparams[1].Xbsep()*2 );
    m_lbparams[0].SetXblen( m_lbparams[1].Xblen() + m_lbparams[1].Xbsep() );
    m_lbparams[0].SetYbsep( m_lbparams[1].Ybsep()*2 );
    m_lbparams[0].SetYblen( m_lbparams[1].Yblen() + m_lbparams[1].Xbsep() );        

    m_cbparams[1].SetXbsep( m_cbparams[2].Xbsep()*2 );
    m_cbparams[1].SetXblen( m_cbparams[2].Xblen() + m_cbparams[2].Xbsep() );
    m_cbparams[1].SetYbsep( m_cbparams[2].Ybsep()*2 );
    m_cbparams[1].SetYblen( m_cbparams[2].Yblen() + m_cbparams[2].Xbsep() );    

    m_cbparams[0].SetXbsep( m_cbparams[1].Xbsep()*2 );
    m_cbparams[0].SetXblen( m_cbparams[1].Xblen() + m_cbparams[1].Xbsep() );
    m_cbparams[0].SetYbsep( m_cbparams[1].Ybsep()*2 );
    m_cbparams[0].SetYblen( m_cbparams[1].Yblen() + m_cbparams[1].Xbsep() );        

}

//EncoderParams functions

//Default constructor    
EncoderParams::EncoderParams():
    CodecParams(),
    m_qf(5.0),
    m_num_L1(0),
    m_L1_sep(0),
    m_ufactor(1.0),
    m_vfactor(1.0),
    m_cpd(20.0),
    m_I_lambda(0.f),
    m_L1_lambda(0.0f),
    m_L2_lambda(0.0f),
    m_L1_me_lambda(0.0f),
    m_L2_me_lambda(0.0f),
    m_ent_correct(0),
    m_bit_out(0)
{}

const float EncoderParams::Lambda(const FrameSort& fsort) const
{
    if (fsort == I_frame)
        return ILambda();
    else if (fsort == L1_frame)
        return L1Lambda();
    else
        return L2Lambda();
}


void EncoderParams::SetLambda(const FrameSort& fsort, const float l)
{
    if (fsort == I_frame)
        SetILambda(l);
    else if (fsort == L1_frame)
        SetL1Lambda(l);
    else
        SetL2Lambda(l);
}

//SeqParams functions
//constructor
SeqParams::SeqParams():
    m_xl(0),
    m_yl(0),
    m_zl(0),
    m_cformat(format422),
    m_interlace(false),
    m_topfieldfirst(true),
    m_framerate(12)
{}

//FrameParams functions

// Default constructor
FrameParams::FrameParams():
    m_fsort(I_frame),
    m_output(false)
{}    

// Constructor 
FrameParams::FrameParams(const ChromaFormat& cf, int xlen, int ylen):
    m_cformat(cf),
    m_xl(xlen),
    m_yl(ylen),
    m_fsort(I_frame),
    m_output(false)
{}

// Constructor
FrameParams::FrameParams(const ChromaFormat& cf, const FrameSort& fs):
    m_cformat(cf),
    m_fsort(fs),
    m_output(false)
{}    

// Constructor
FrameParams::FrameParams(const SeqParams& sparams):
    m_cformat(sparams.CFormat()),
    m_xl(sparams.Xl()),
    m_yl(sparams.Yl()),
    m_fsort(I_frame),
    m_output(false)
{}

// Constructor
FrameParams::FrameParams(const SeqParams& sparams, const FrameSort& fs):
    m_cformat(sparams.CFormat()),
    m_xl(sparams.Xl()),
    m_yl(sparams.Yl()),
    m_fsort(fs),
    m_output(false)
{}
