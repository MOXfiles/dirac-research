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
    m_mv_precision(2)
{}

void CodecParams::SetBlockSizes(const OLBParams& olbparams , const ChromaFormat cformat)
{
    //given the raw overlapped block parameters, set the modified internal parameters to
    //take account of the chroma sampling format and overlapping requirements, as well
    //as the equivalent parameters for sub-MBs and MBs.
    //Does NOT set the number of blocks or macroblocks, as padding may be required.

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

    m_lbparams[2] = olbparams;
    
    // Check the separations aren't too small
    m_lbparams[2].SetXbsep( std::max(m_lbparams[2].Xbsep() , 4) );
    m_lbparams[2].SetYbsep( std::max(m_lbparams[2].Ybsep() , 4) );

    // Check the lengths aren't too big (100% is max roll-off)
    m_lbparams[2].SetXblen( std::min(m_lbparams[2].Xbsep()*2 , m_lbparams[2].Xblen()) );
    m_lbparams[2].SetYblen( std::min(m_lbparams[2].Ybsep()*2 , m_lbparams[2].Yblen()) );
    
    // Check overlap is divisible by 2
    if (( m_lbparams[2].Xblen() - m_lbparams[2].Xbsep() )%2 != 0)
        m_lbparams[2].SetXblen( m_lbparams[2].Xblen()-1 );
    if (( m_lbparams[2].Yblen() - m_lbparams[2].Ybsep() )%2 != 0)
        m_lbparams[2].SetYblen( m_lbparams[2].Yblen()-1 );

    // Check there's now sufficient overlap  
    m_lbparams[2].SetXblen( std::max(m_lbparams[2].Xbsep()+2 , m_lbparams[2].Xblen()) );
    m_lbparams[2].SetYblen( std::max(m_lbparams[2].Ybsep()+2 , m_lbparams[2].Yblen()) );


    // If the overlapped blocks don't work for the chroma format, we have to iterate
    if ( (m_lbparams[2].Xbsep()%xcfactor != 0) || (m_lbparams[2].Ybsep()%ycfactor != 0) )
    {
        OLBParams new_olbparams( m_lbparams[2] );

        if (m_lbparams[2].Xbsep()%xcfactor != 0)
            new_olbparams.SetXbsep( ( (m_lbparams[2].Xbsep()/xcfactor) + 1 )*xcfactor );

        if (m_lbparams[2].Ybsep()%ycfactor != 0)
            new_olbparams.SetYbsep( ( (m_lbparams[2].Ybsep()/ycfactor) + 1 )*ycfactor );

        new_olbparams.SetXblen( std::max( new_olbparams.Xbsep()+2 , olbparams.Xblen() ) );
        new_olbparams.SetYblen( std::max( new_olbparams.Xbsep()+2 , olbparams.Xblen() ) );
        
        SetBlockSizes( new_olbparams , cformat );
    }
    
    // Now compute the resulting chroma block params

    m_cbparams[2].SetXbsep( m_lbparams[2].Xbsep()/xcfactor );
    m_cbparams[2].SetYbsep( m_lbparams[2].Ybsep()/ycfactor );
    m_cbparams[2].SetXblen( std::max(m_lbparams[2].Xblen()/xcfactor , m_cbparams[2].Xbsep()+2) );
    m_cbparams[2].SetYblen( std::max(m_lbparams[2].Yblen()/ycfactor , m_cbparams[2].Ybsep()+2) );
    
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
    m_recode(2),
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

QuantiserLists::QuantiserLists()
: 
    m_qflist( 60 ), 
    m_qfinvlist( 60 ) , 
    m_offset( 60 )
{
    m_qflist[0]=1;        m_qfinvlist[0]=131072;    m_offset[0]=0;
    m_qflist[1]=1;        m_qfinvlist[1]=131072;    m_offset[1]=0;
    m_qflist[2]=1;        m_qfinvlist[2]=131072;    m_offset[2]=0;
    m_qflist[3]=1;        m_qfinvlist[3]=131072;    m_offset[3]=0;
    m_qflist[4]=2;        m_qfinvlist[4]=65536;        m_offset[4]=1;
    m_qflist[5]=2;        m_qfinvlist[5]=65536;        m_offset[5]=1;
    m_qflist[6]=2;        m_qfinvlist[6]=65536;        m_offset[6]=1;
    m_qflist[7]=3;        m_qfinvlist[7]=43690;        m_offset[7]=1;
    m_qflist[8]=4;        m_qfinvlist[8]=32768;        m_offset[8]=2;
    m_qflist[9]=4;        m_qfinvlist[9]=32768;        m_offset[9]=2;
    m_qflist[10]=5;        m_qfinvlist[10]=26214;    m_offset[10]=2;
    m_qflist[11]=6;        m_qfinvlist[11]=21845;    m_offset[11]=2;
    m_qflist[12]=8;        m_qfinvlist[12]=16384;    m_offset[12]=3;
    m_qflist[13]=9;        m_qfinvlist[13]=14563;    m_offset[13]=3;
    m_qflist[14]=11;        m_qfinvlist[14]=11915;    m_offset[14]=4;
    m_qflist[15]=13;        m_qfinvlist[15]=10082;    m_offset[15]=5;
    m_qflist[16]=16;        m_qfinvlist[16]=8192;        m_offset[16]=6;
    m_qflist[17]=19;        m_qfinvlist[17]=6898;        m_offset[17]=7;
    m_qflist[18]=22;        m_qfinvlist[18]=5957;        m_offset[18]=8;
    m_qflist[19]=26;        m_qfinvlist[19]=5041;        m_offset[19]=10;
    m_qflist[20]=32;        m_qfinvlist[20]=4096;        m_offset[20]=12;
    m_qflist[21]=38;        m_qfinvlist[21]=3449;        m_offset[21]=14;
    m_qflist[22]=45;        m_qfinvlist[22]=2912;        m_offset[22]=17;
    m_qflist[23]=53;        m_qfinvlist[23]=2473;        m_offset[23]=20;
    m_qflist[24]=64;        m_qfinvlist[24]=2048;        m_offset[24]=24;
    m_qflist[25]=76;        m_qfinvlist[25]=1724;        m_offset[25]=29;
    m_qflist[26]=90;        m_qfinvlist[26]=1456;        m_offset[26]=34;
    m_qflist[27]=107;        m_qfinvlist[27]=1224;        m_offset[27]=40;
    m_qflist[28]=128;        m_qfinvlist[28]=1024;        m_offset[28]=48;
    m_qflist[29]=152;        m_qfinvlist[29]=862;        m_offset[29]=57;
    m_qflist[30]=181;        m_qfinvlist[30]=724;        m_offset[30]=68;
    m_qflist[31]=215;        m_qfinvlist[31]=609;        m_offset[31]=81;
    m_qflist[32]=256;        m_qfinvlist[32]=512;        m_offset[32]=96;
    m_qflist[33]=304;        m_qfinvlist[33]=431;        m_offset[33]=114;
    m_qflist[34]=362;        m_qfinvlist[34]=362;        m_offset[34]=136;
    m_qflist[35]=430;        m_qfinvlist[35]=304;        m_offset[35]=161;
    m_qflist[36]=512;        m_qfinvlist[36]=256;        m_offset[36]=192;
    m_qflist[37]=608;        m_qfinvlist[37]=215;        m_offset[37]=228;
    m_qflist[38]=724;        m_qfinvlist[38]=181;        m_offset[38]=272;
    m_qflist[39]=861;        m_qfinvlist[39]=152;        m_offset[39]=323;
    m_qflist[40]=1024;    m_qfinvlist[40]=128;        m_offset[40]=384;
    m_qflist[41]=1217;    m_qfinvlist[41]=107;        m_offset[41]=456;
    m_qflist[42]=1448;    m_qfinvlist[42]=90;        m_offset[42]=543;
    m_qflist[43]=1722;    m_qfinvlist[43]=76;        m_offset[43]=646;
    m_qflist[44]=2048;    m_qfinvlist[44]=64;        m_offset[44]=768;
    m_qflist[45]=2435;    m_qfinvlist[45]=53;        m_offset[45]=913;
    m_qflist[46]=2896;    m_qfinvlist[46]=45;        m_offset[46]=1086;
    m_qflist[47]=3444;    m_qfinvlist[47]=38;        m_offset[47]=1292;
    m_qflist[48]=4096;    m_qfinvlist[48]=32;        m_offset[48]=1536;
    m_qflist[49]=4870;    m_qfinvlist[49]=26;        m_offset[49]=1826;
    m_qflist[50]=5792;    m_qfinvlist[50]=22;        m_offset[50]=2172;
    m_qflist[51]=6888;    m_qfinvlist[51]=19;        m_offset[51]=2583;
    m_qflist[52]=8192;    m_qfinvlist[52]=16;        m_offset[52]=3072;
    m_qflist[53]=9741;    m_qfinvlist[53]=13;        m_offset[53]=3653;
    m_qflist[54]=11585;    m_qfinvlist[54]=11;        m_offset[54]=4344;
    m_qflist[55]=13777;    m_qfinvlist[55]=9;        m_offset[55]=5166;
    m_qflist[56]=16384;    m_qfinvlist[56]=8;        m_offset[56]=6144;
    m_qflist[57]=19483;    m_qfinvlist[57]=6;        m_offset[57]=7306;
    m_qflist[58]=23170;    m_qfinvlist[58]=5;        m_offset[58]=8689;
    m_qflist[59]=27554;    m_qfinvlist[59]=4;        m_offset[59]=10333;
}
