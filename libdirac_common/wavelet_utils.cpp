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
* Contributor(s): Thomas Davies (Original Author), Scott R Ladd
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


#include <libdirac_common/wavelet_utils.h>
#include <libdirac_common/common.h>
#include <cstdlib>

// Default constructor
Subband::Subband()
{
    // this space intentionally left blank
}

// Constructor
Subband::Subband(int xpos,int ypos, int xlen, int ylen):
    xps(xpos),
    yps(ypos),
    xln(xlen),
    yln(ylen),
    wgt(1),
    qfac(8)
{
    // this space intentionally left blank
}

// Constructor
Subband::Subband(int xpos,int ypos, int xlen, int ylen, int d)
  : xps(xpos),
    yps(ypos),
    xln(xlen), 
    yln(ylen),
    wgt(1),
    dpth(d),
    qfac(8)
{
    // this space intentionally left blank
}

//! Destructor
Subband::~Subband()
{
    // this space intentionally left blank
}

//subband list methods

void SubbandList::Init(const int depth,const int xlen,const int ylen)
{
    int xl=xlen; 
    int yl=ylen; 
    
    Clear(); 
    Subband* tmp;
     
    for (int level = 1; level <= depth; ++level)
    {
        xl/=2; 
        yl/=2; 
        
        tmp=new Subband(xl , 0 , xl , yl , level); 
        AddBand( *tmp ); 
        delete tmp; 
        
        tmp=new Subband( 0 , yl , xl , yl , level); 
        AddBand( *tmp ); 
        delete tmp; 
        
        tmp=new Subband( xl , yl , xl , yl , level); 
        AddBand( *tmp ); 
        delete tmp; 
        
        if (level == depth)
        {
            tmp=new Subband( 0 , 0 , xl , yl , level); 
            AddBand( *tmp ); 
            delete tmp; 
        }        
    }
    //now set the parent-child relationships
    int len = bands.size(); 
    (*this)(len).SetParent(0);         
    (*this)(len).AddChild(len-3); 
    (*this)(len-3).SetParent(len); 
    (*this)(len).AddChild(len-2); 
    (*this)(len-2).SetParent(len); 
    (*this)(len).AddChild(len-1); 
    (*this)(len-1).SetParent(len); 

    for (int level = 1; level < depth; ++level)
    {
         //do parent-child relationship for other bands
        (*this)(3*level + 1).AddChild( 3*(level-1) + 1); 
        (*this)(3*(level-1) + 1).SetParent(3*level + 1); 

        (*this)(3*level + 2).AddChild(3*(level-1) + 2); 
        (*this)(3*(level-1) + 2).SetParent(3*level + 2); 

        (*this)(3*level + 3).AddChild(3*(level-1) + 3); 
        (*this)(3*(level-1) + 3).SetParent(3*level + 3); 
    }// level
}

//wavelet transform methods
///////////////////////////

//public methods

WaveletTransform::WaveletTransform(int d, WltFilter f)
  : depth(d),
    filt_sort(f)
{
    // this space intentionally left blank
}

//! Destructor
WaveletTransform::~WaveletTransform()
{
    // this space intentionally left blank
}

void WaveletTransform::Transform(const Direction d, PicArray& pic_data)
{
    int xl,yl; 

    if (d == FORWARD)
    {
        //do work
        xl=pic_data.LengthX(); 
        yl=pic_data.LengthY(); 
        
        for (int l = 1; l <= depth; ++l)
        {
            VHSplit(0,0,xl,yl,pic_data); 
            xl /= 2; 
            yl /= 2; 
        }

        band_list.Init( depth , pic_data.LengthX() , pic_data.LengthY() ); 
    }
    else
    {
        //do work
        xl = pic_data.LengthX()/(1<<(depth-1)); 
        yl = pic_data.LengthY()/(1<<(depth-1)); 
        
        for (int l = 1; l <= depth; ++l)
        {
            VHSynth(0,0,xl,yl,pic_data); 
            xl *= 2; 
            yl *= 2; 
        }
        
        //band list now inaccurate, so clear        
        band_list.Clear();     
    }
}

//private functions
///////////////////

void WaveletTransform::VHSplit(const int xp, const int yp, const int xl, const int yl, PicArray& pic_data){

    //version based on integer-like types
    //using edge-extension rather than reflection

    OneDArray<ValueType *> tmp_data(yl); 
    const int xl2 = xl/2; 
    const int yl2 = yl/2; 
    ValueType * line_data; 
    ValueType * col_data=new ValueType[yl];     

    // Positional variables
    int i,j,k,l; 
  
    // Objects to do lifting stages 
    // (in revese order and type from synthesis)
    const PredictStep< 6497 > predictA;
    const PredictStep< 217 > predictB;
    const UpdateStep< 3616 > updateA;
    const UpdateStep< 1817 > updateB;

     //first do horizontal 

    int xend=xl2;     
    int yend=yl; 


    for (j = 0;  j < yend; ++j)
    {
        line_data=new ValueType[xl]; 

        // Copy data across to a temporary array, separating odd and even
        for (i = 0, k = xl2, l = 0;  i < xend; ++i, ++k, ++l)
        {
            line_data[i] = pic_data[ j+yp ][ l+xp ]; 
            line_data[k] = pic_data[ j+yp ][ ++l+xp ];             
        }//i

        // First lifting stage
 
        predictA.Filter( line_data[xl2] , line_data[1] , line_data[0] );
        predictB.Filter( line_data[0] , line_data[xl2] , line_data[xl2] );
        
        for (i = 1, k = xl2+1; i < xend-1; ++i, ++k)
        {
            predictA.Filter( line_data[k] , line_data[i+1] , line_data[i] );
            predictB.Filter( line_data[i] , line_data[k-1] , line_data[k] );
        }//i
        
        predictA.Filter( line_data[xl-1] , line_data[xend-1] , line_data[xend-1] );
        predictB.Filter( line_data[xend-1] , line_data[xl-2] , line_data[xl-1] );

         //second lifting stage 
        updateA.Filter( line_data[xl2] , line_data[1] , line_data[0] );
        updateB.Filter( line_data[0] , line_data[xl2]  ,line_data[xl2] );
        
        //main body
        for (i = 1, k = xl2+1;  i < xend-1; ++i,++k)
        { 
            updateA.Filter( line_data[k] , line_data[i+1] , line_data[i] );
            updateB.Filter( line_data[i] , line_data[k-1] , line_data[k] );
        }
        
        updateA.Filter( line_data[xl-1] , line_data[xend-1] , line_data[xend-1] );
        updateB.Filter( line_data[xend-1] , line_data[xl-2] , line_data[xl-1] );

        tmp_data[j] = line_data; 
    }

     //next do vertical
    xend = xl;     
    yend = yl2; 

    for (i = 0; i < xend; ++i)
    {

        for (j = 0, k = yl2, l=0;  j < yend; ++j, ++k, ++l)
        {
            col_data[j] = tmp_data[l][i]; 
            col_data[k] = tmp_data[++l][i];             
        }//j

        // First lifting stage 
        predictA.Filter( col_data[yl2] , col_data[1] , col_data[0] );
        predictB.Filter( col_data[0] , col_data[yl2] , col_data[yl2] );
        
        //main body
        for (j = 1, k = yl2+1;  j < yend-1; ++j, ++k)
        {
            predictA.Filter( col_data[k] , col_data[j+1] , col_data[j] );
            predictB.Filter( col_data[j] , col_data[k-1] , col_data[k] ); 
        }//j
        
        predictA.Filter( col_data[yl-1] , col_data[yend-1] , col_data[yend-1] );
        predictB.Filter( col_data[yend-1] , col_data[yl-2] , col_data[yl-1] );        

        //second lifting stage
        updateA.Filter( col_data[yl2] , col_data[1] , col_data[0] );
        updateB.Filter( col_data[0] , col_data[yl2] , col_data[yl2] );
        
        for (j = 1,k = yl2+1;  j < yend-1; ++j, ++k)
        {
            updateA.Filter( col_data[k] , col_data[j+1] , col_data[j] );
            updateB.Filter( col_data[j] , col_data[k-1] , col_data[k]);
        }//j
        
        updateA.Filter( col_data[yl-1] , col_data[yend-1] , col_data[yend-1] );
        updateB.Filter( col_data[yend-1] , col_data[yl-2] , col_data[yl-1] );

        // Copy the data back into the original array
        for (j=0, k=yl2;  j < yend; ++j, ++k)
        {
            pic_data[ j+yp ][ i+xp ] = col_data[j]; 
            pic_data[ k+yp ][ i+xp ] = col_data[k]; 
        }//i

    }
    
    // Tidy up
    delete [] col_data;
    
    for (int j=0; j<yl; ++j)
        delete[] tmp_data[j]; 

}

void WaveletTransform::VHSynth(const int xp, const int yp, const int xl, const int yl, PicArray& pic_data){    

    ValueType* line_data = new ValueType[xl]; 
    ValueType* col_data;     
    OneDArray<ValueType*> tmp_data(xl); 

    const int xl2 = xl/2; 
    const int yl2 = yl/2; 
    int i,j,k,l,m; //positional variables

    // set up lifting objects
    const PredictStep< 1817 > predictA;
    const PredictStep< 3616 > predictB;

    const UpdateStep< 217 > updateA;
    const UpdateStep< 6497 > updateB;

    //first do vertical synth//
    ///////////////////////////

    int xend=xl;
    int yend=yl2; 

    for (i=0;  i<xend; ++i)
    {        
        // copy a column of data into a temp array
        col_data = new ValueType[yl];
        for ( j=0 , k=yl2 ; j<yl2 ; ++j , ++k)
        {
            col_data[j] = pic_data[ j+yp ][ i+xp ];     
            col_data[k] = pic_data[ k+yp ][ i+xp ];             
        }

        // First lifting stage
        predictA.Filter( col_data[yend-1] , col_data[yl-2] , col_data[yl-1] );
        predictB.Filter( col_data[yl-1] , col_data[yend-1] , col_data[yend-1] );

        for ( j=yend-2 , k=yl-2 ; j>=1 ; --j , --k ) 
        {
            predictA.Filter( col_data[j] , col_data[k-1] , col_data[k] );
            predictB.Filter( col_data[k] , col_data[j+1] , col_data[j] );
        }// j

        predictA.Filter( col_data[0] , col_data[yl2] , col_data[yl2] );
        predictB.Filter( col_data[yl2] , col_data[1] , col_data[0] );
 
       // Second lifting stage
        updateA.Filter( col_data[yend-1] , col_data[yl-2] , col_data[yl-1] );
        updateB.Filter( col_data[yl-1] , col_data[yend-1] , col_data[yend-1] );

        for ( j=yend-2 , k=yl-2 ; j>=1; --j , --k )
        {
            updateA.Filter( col_data[j] , col_data[k-1] , col_data[k] );
            updateB.Filter( col_data[k] , col_data[j+1] , col_data[j] );
        }// j

        updateA.Filter( col_data[0] , col_data[yl2] , col_data[yl2] );
        updateB.Filter( col_data[yl2] , col_data[1] , col_data[0] );

        tmp_data[i] = col_data;

    }// i

     //next do horizontal//    
     //////////////////////

    xend=xl2;     
    yend=yl; 

    for ( j=0 ; j<yend ; ++j )
    {

        for ( i=0 , k=xl2 ; i<xl2; ++i , ++k )
        {
            line_data[i] = tmp_data[i][j]; 
            line_data[k] = tmp_data[k][j]; 
        }        

        // First lifting stage
        predictA.Filter( line_data[xend-1] , line_data[xl-2] , line_data[xl-1] );
        predictB.Filter( line_data[xl-1] , line_data[xend-1] , line_data[xend-1] );                         

        for ( i=xend-2 , k=xl-2 ; i>=1; --i , --k )
        {
            predictA.Filter( line_data[i] , line_data[k-1] , line_data[k] );
            predictB.Filter( line_data[k] , line_data[i+1] , line_data[i] );
        }// i

        predictA.Filter( line_data[0] , line_data[xl2] , line_data[xl2] );
        predictB.Filter( line_data[xl2] , line_data[1] , line_data[0] );

         // Second lifting stage 
        updateA.Filter( line_data[xend-1] , line_data[xl-2] , line_data[xl-1] );
        updateB.Filter( line_data[xl-1] , line_data[xend-1] , line_data[xend-1] );                         

        for ( i=xend-2 , k=xl-2 ; i>=1 ; --i,--k)
        {
            updateA.Filter( line_data[i] , line_data[k-1] , line_data[k] );
            updateB.Filter( line_data[k] , line_data[i+1] , line_data[i] );
        }// i 
    
        updateA.Filter( line_data[0] , line_data[xl2] , line_data[xl2] );
        updateB.Filter( line_data[xl2] , line_data[1] , line_data[0] );

         // Copy data back into the original array
         if ( j<yl2 )
         {
             l = 2*j; 
             for ( i=0 , k=xl2 ,m=0 ; i<xl2 ; ++i , ++k , m=m+2 )
             {
                 pic_data[l][m] = line_data[i]; 
                 pic_data[l][m+1] = line_data[k]; 
             }// i
         }
         else
         {
             l = 2*j - yl + 1; 
             for ( i=0 , k=xl2 , m=0; i<xl2 ; ++i , ++k , m=m+2 )
             {
                 pic_data[l+yp][m+xp] = line_data[i]; 
                 pic_data[l+yp][m+1+xp] = line_data[k]; 
             }// i

         }
    }

    // Tidy up
    delete[] line_data; 

    // This deletes all the allocated col_data's
    for ( int i=0 ; i<xl ; ++i )
        delete[] tmp_data[i]; 

}

//perceptual weighting stuff
////////////////////////////

// Returns a perceptual noise weighting based on extending CCIR 959 values
// assuming a two-d isotropic response. Also has a fudge factor of 20% for chroma
float WaveletTransform::PerceptualWeight( float xf , float yf , CompSort cs )
{
    double freq_sqd( xf*xf + yf*yf );

    if ( cs != Y_COMP )
        freq_sqd *= 1.2;

    return 0.255 * std::pow( 1.0 + 0.2561*freq_sqd , 0.75) ;

}

void WaveletTransform::SetBandWeights (const float cpd, 
                                       const FrameSort& fsort,
                                       const ChromaFormat& cformat,
                                       const CompSort csort)
{
    //NB - only designed for progressive to date    

    int xlen, ylen, xl, yl, xp, yp, depth;
    float xfreq, yfreq;
    float temp;

    // Compensate for chroma subsampling

    float chroma_xfac(1.0);
    float chroma_yfac(1.0);

    if( csort != Y_COMP)
    {
        if( cformat == format422)
        {
            chroma_xfac = 2.0;
            chroma_yfac = 1.0;
        }
        else if( cformat == format411 )
        {
            chroma_xfac = 4.0;
            chroma_yfac = 1.0;
        }
        else if( cformat == format420 )
        {
            chroma_xfac = 2.0;
            chroma_yfac = 2.0;
        }

    }

    xlen = 2 * band_list(1).Xl();
    ylen = 2 * band_list(1).Yl();

    if (cpd != 0.0)
    {
        for( int i = 1; i<=band_list.Length() ; i++ )
        {
            xp = band_list(i).Xp();
            yp = band_list(i).Yp();
            xl = band_list(i).Xl();
            yl = band_list(i).Yl();


            xfreq = cpd * ( float(xp) + (float(xl)/2.0) ) / float(xlen);
            yfreq = cpd * ( float(yp) + (float(yl)/2.0) ) / float(ylen);

            if ( fsort != I_frame )
            {
                xfreq /= 8.0;
                yfreq /= 8.0;
            }


            temp = PerceptualWeight( xfreq/chroma_xfac , yfreq/chroma_yfac , csort );

            band_list(i).SetWt(temp);
        }// i

        // Give more welly to DC in a completely unscientific manner ...
        // (should really relate this to the frame rate)
        band_list( band_list.Length() ).SetWt(band_list(13).Wt()/6.0);

        // Make sure dc is always the lowest weight
        float min_weight=band_list(band_list.Length()).Wt();

        for( int b=1 ; b<=band_list.Length()-1 ; b++ )
            min_weight = ((min_weight>band_list(b).Wt()) ? band_list(b).Wt() : min_weight);

        band_list( band_list.Length() ).SetWt( min_weight );

        // Now normalize weights so that white noise is always weighted the same

        // Overall factor to ensure that white noise ends up with the same RMS, whatever the weight
        double overall_factor=0.0;
        //fraction of the total number of samples belonging to each subband
        double subband_fraction;    

        for( int i=1 ; i<=band_list.Length() ; i++ )
        {
            subband_fraction = 1.0/((double) band_list(i).Scale() * band_list(i).Scale());
            overall_factor += subband_fraction/( band_list(i).Wt() * band_list(i).Wt() );
        }
        overall_factor = std::sqrt( overall_factor );

        //go through and normalise

        for( int i=band_list.Length() ; i>0 ; i-- )
            band_list(i).SetWt( band_list(i).Wt() * overall_factor );
    }
    else
    {//cpd=0 so set all weights to 1

        for( int i=1 ; i<=band_list.Length() ; i++ )
           band_list(i).SetWt( 1.0 );        

    }

    //Finally, adjust to compensate for the absence of scaling in the transform
    //Factor used to compensate:
    const double alpha(1.149658203);    
    for ( int i=1 ; i<=band_list.Length() ; ++i )
    {
        depth=band_list(i).Depth();

        if ( band_list(i).Xp() == 0 && band_list(i).Yp() == 0)
            temp=std::pow(alpha,2*depth);
        else if ( band_list(i).Xp() != 0 && band_list(i).Yp() != 0)
            temp=std::pow(alpha,2*(depth-2));
        else
            temp=std::pow(alpha,2*(depth-1));

        band_list(i).SetWt(band_list(i).Wt()/temp);

    }// i        

}    
