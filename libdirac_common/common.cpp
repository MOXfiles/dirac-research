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
using namespace dirac;

//const dirac::QuantiserLists dirac::dirac_quantiser_lists;

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
            m_Yfctrs[I_frame][i] = 1.0f;
            m_Ufctrs[I_frame][i] = 1.0f;
            m_Vfctrs[I_frame][i] = 1.0f;
            m_Yfctrs[L1_frame][i] = 0.85f;
            m_Ufctrs[L1_frame][i] = 0.85f;
            m_Vfctrs[L1_frame][i] = 0.85f;
            m_Yfctrs[L2_frame][i] = 0.85f;
            m_Ufctrs[L2_frame][i] = 0.85f;
            m_Vfctrs[L2_frame][i] = 0.85f;
        }
        else if ( i >= m_Yfctrs.LastX()-3 )
        {
            m_Yfctrs[I_frame][i] = 0.85f;
            m_Ufctrs[I_frame][i] = 0.85f;
            m_Vfctrs[I_frame][i] = 0.85f;
            m_Yfctrs[L1_frame][i] = 0.75f;
            m_Ufctrs[L1_frame][i] = 0.75f;
            m_Vfctrs[L1_frame][i] = 0.75f;
            m_Yfctrs[L2_frame][i] = 0.75f;
            m_Ufctrs[L2_frame][i] = 0.75f;
            m_Vfctrs[L2_frame][i] = 0.75f;            
        }
        else
        {
            m_Yfctrs[I_frame][i] = 0.75f;
            m_Ufctrs[I_frame][i] = 0.75f;
            m_Vfctrs[I_frame][i] = 0.75f;
            m_Yfctrs[L1_frame][i] = 0.75f;
            m_Ufctrs[L1_frame][i] = 0.75f;
            m_Vfctrs[L1_frame][i] = 0.75f;
            m_Yfctrs[L2_frame][i] = 0.75f;
            m_Ufctrs[L2_frame][i] = 0.75f;
            m_Vfctrs[L2_frame][i] = 0.75f;            
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

namespace dirac
{
std::ostream & operator<< (std::ostream & stream, OLBParams & params)
{
    stream << params.Ybsep() << " " << params.Xbsep();
//     stream << " " <<params.Yblen() << " " << params.Xblen();
    
    return stream;
}

std::istream & operator>> (std::istream & stream, OLBParams & params)
{
    int temp;

    stream >> temp;
    params.SetYbsep(temp);

    stream >> temp;
    params.SetXbsep(temp);

//     stream >> temp;
//     params.SetYblen(temp);

//     stream >> temp;
//     params.SetXblen(temp);
    
    return stream;
}

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
    m_cbparams(3),
    m_mv_precision(2),
    m_wlt_filter(APPROX97)
{}

void CodecParams::SetBlockSizes(const OLBParams& olbparams , const ChromaFormat cformat)
{
    //given the raw overlapped block parameters, set the modified internal parameters to
    //take account of the chroma sampling format and overlapping requirements, as well
    //as the equivalent parameters for sub-MBs and MBs.
    //Does NOT set the number of blocks or macroblocks, as padding may be required.

    OLBParams tmp_olbparams = olbparams;
    // Factors for scaling chroma blocks
    int xcfactor,ycfactor; 
 
    if (cformat == format420)
    {
        xcfactor = 2;
        ycfactor = 2;
    }
    else if (cformat == format422)
    {
        xcfactor = 2;
        ycfactor = 1;

    }
    else if (cformat==format411)
    {
        xcfactor = 4;
        ycfactor = 1;
    }
    else
    {// assume 444
        xcfactor = 1;
        ycfactor = 1;
    }

    // Loop until block level luma and chroma parameters satisfy all conditions
    do
    {
        m_lbparams[2] = tmp_olbparams;
        
        // Check there's now sufficient overlap,
        // XBLEN > XBSEP, YBLEN > YBSEP
        m_lbparams[2].SetXblen( std::max(m_lbparams[2].Xbsep()+1 , m_lbparams[2].Xblen()) );
        m_lbparams[2].SetYblen( std::max(m_lbparams[2].Ybsep()+1 , m_lbparams[2].Yblen()) );


        // Check the lengths aren't too big (100% is max roll-off)
        // XBLEN <= 2*XBSEP, YBLEN <= 2*YBSEP
        m_lbparams[2].SetXblen( std::min(m_lbparams[2].Xbsep()*2 , m_lbparams[2].Xblen()) );
        m_lbparams[2].SetYblen( std::min(m_lbparams[2].Ybsep()*2 , m_lbparams[2].Yblen()) );
    

        // If the overlapped blocks don't work for the chroma format, we have to iterate
        OLBParams new_olbparams( m_lbparams[2] );

        // Test if XBSEP % CHROMA_H_FACTOR == 0 && yBSEP % CHROMA_V_FACTOR == 0
        if ( (m_lbparams[2].Xbsep()%xcfactor != 0) || (m_lbparams[2].Ybsep()%ycfactor != 0) )
        {
            // luma XBSEP and/or YBSEP not multiples of chroma factor
            // Increment XBSEP and YBSEP so that they are multiples of chroma
            // factor.
            if (m_lbparams[2].Xbsep()%xcfactor != 0)
            {
                new_olbparams.SetXbsep( ( (m_lbparams[2].Xbsep()/xcfactor) + 1 )*xcfactor );
                new_olbparams.SetXblen( std::max( new_olbparams.Xbsep()+1 , olbparams.Xblen() ) );
            }

            if (m_lbparams[2].Ybsep()%ycfactor != 0)
            {
                new_olbparams.SetYbsep( ( (m_lbparams[2].Ybsep()/ycfactor) + 1 )*ycfactor );
                new_olbparams.SetYblen( std::max( new_olbparams.Ybsep()+1 , olbparams.Yblen() ) );
            }
              // Loop again with new luma block params 
            tmp_olbparams = new_olbparams;
        }
        else
        {
            // Now compute the resulting chroma block params
            // XBSEP_CHROMA=XBSEP//CHROMA_H_FACTOR
            m_cbparams[2].SetXbsep( m_lbparams[2].Xbsep()/xcfactor );
            // YBSEP_CHROMA=YBSEP//CHROMA_H_FACTOR
            m_cbparams[2].SetYbsep( m_lbparams[2].Ybsep()/ycfactor );
            m_cbparams[2].SetXblen( std::max(m_lbparams[2].Xblen()/xcfactor , m_cbparams[2].Xbsep()+1) );
            m_cbparams[2].SetYblen( std::max(m_lbparams[2].Yblen()/ycfactor , m_cbparams[2].Ybsep()+1) );
            bool recalc = false;
            // Check the lengths aren't too big (100% is max roll-off)
            // XBLEN_CHROMA <= 2*XBSEP_CHROMA, YBLEN_CHROMA <= 2*YBSEP_CHROMA
            if (m_cbparams[2].Xblen() > 2*m_cbparams[2].Xbsep())
            {
                new_olbparams.SetXbsep( ( (m_lbparams[2].Xbsep()/xcfactor) + 1 )*xcfactor );
                recalc = true;
            }
            if ( m_cbparams[2].Yblen() > 2*m_cbparams[2].Ybsep() )
            {
                 new_olbparams.SetYbsep( ( (m_lbparams[2].Ybsep()/ycfactor) + 1 ) *ycfactor );
                recalc = true;
            }
            if (recalc)
                tmp_olbparams = new_olbparams;
            else
                break;
        }
    
   } while(true); 

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

    if ( m_lbparams[2].Xbsep()!=olbparams.Xbsep() ||
         m_lbparams[2].Ybsep()!=olbparams.Ybsep() ||
         m_lbparams[2].Xblen()!=olbparams.Xblen() ||
         m_lbparams[2].Yblen()!=olbparams.Yblen() )
    {
        std::cout<<std::endl<<"WARNING: block parameters are inconsistent or ";
        std::cout<<"incompatible with chroma format.";
        std::cout<<std::endl<<"Instead, using:";
        std::cout<<" xblen="<<m_lbparams[2].Xblen();
        std::cout<<" yblen="<<m_lbparams[2].Yblen();
        std::cout<<" xbsep="<<m_lbparams[2].Xbsep();
        std::cout<<" ybsep="<<m_lbparams[2].Ybsep() << std::endl;
    }
}

//EncoderParams functions

//Default constructor    
EncoderParams::EncoderParams():
    CodecParams(),

    m_loc_decode(true),
    m_lossless(false),
    m_qf(7.0),
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

float EncoderParams::Lambda(const FrameSort& fsort) const
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
    m_cformat(format422),
    m_interlace(false),
    m_topfieldfirst(true),
    m_framerate(12)
{}

int SeqParams::ChromaWidth() const
{
    switch (m_cformat)
    {
    case Yonly:
        return 0;
    case format411:
        return m_xl/4;

    case format420:
    case format422:
        return m_xl/2;

    case format444:
    default:
        return m_xl;
    }
}

int SeqParams::ChromaHeight() const
{
    switch (m_cformat)
    {
    case Yonly:
        return 0;
        return m_yl;

    case format420:
        return m_yl/2;

    case format422:
    case format444:
    case format411:
    default:
        return m_yl;
    }
}
//FrameParams functions

// Default constructor
FrameParams::FrameParams():
m_fsort(I_frame),
m_output(false)
{}    

// Constructor 
FrameParams::FrameParams(const ChromaFormat& cf, int xlen, int ylen, int c_xlen, int c_ylen):
    m_cformat(cf),
    m_xl(xlen),
    m_yl(ylen),
    m_fsort(I_frame),
    m_output(false),
    m_chroma_xl(c_xlen),
    m_chroma_yl(c_ylen)
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
{
    m_chroma_xl = m_chroma_yl = 0;
    if(m_cformat == format422) 
    {
        m_chroma_xl = m_xl/2;
        m_chroma_yl = m_yl;
    }
    else if (m_cformat == format420)
    {
        m_chroma_xl = m_xl/2;
        m_chroma_yl = m_yl/2;
    }
    else if (m_cformat == format411)
    {
        m_chroma_xl = m_xl/4;
        m_chroma_yl = m_yl;
    }
    else if (m_cformat==format444)
    {
        m_chroma_xl = m_xl;
        m_chroma_yl = m_yl;
    }
}

// Constructor
FrameParams::FrameParams(const SeqParams& sparams, const FrameSort& fs):
    m_cformat(sparams.CFormat()),
    m_xl(sparams.Xl()),
    m_yl(sparams.Yl()),
    m_fsort(fs),
    m_output(false)
{
    m_chroma_xl = m_chroma_yl = 0;
    if(m_cformat == format422) 
    {
        m_chroma_xl = m_xl/2;
        m_chroma_yl = m_yl;
    }
    else if (m_cformat == format420)
    {
        m_chroma_xl = m_xl/2;
        m_chroma_yl = m_yl/2;
    }
    else if (m_cformat == format411)
    {
        m_chroma_xl = m_xl/4;
        m_chroma_yl = m_yl;
    }
    else if (m_cformat==format444)
    {
        m_chroma_xl = m_xl;
        m_chroma_yl = m_yl;
    }
}

QuantiserLists::QuantiserLists()
: 
    m_qflist( 61 ), 
    m_qfinvlist( 61 ) , 
    m_offset( 61 )
{
    for (int i=0; i<=60; ++i)
    {
        m_qflist[i] = int( std::pow(2.0, double(i)/4.0) + 0.5 );
        m_offset[i] = int( double( m_qflist[i]*0.375) + 0.5 );
        m_qfinvlist[i] = int( ( double( 1<<17 ) / double( m_qflist[i] ) ) + 0.5 );
    }// i
}
