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
* Portions created by the Initial Developer are Copyright (C) 2008.
* All Rights Reserved.
*
* Contributor(s): Thomas Davies (Original Author), 
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

#include<libdirac_encoder/prefilter.h>
#include<libdirac_common/arrays.h>

using namespace dirac;

void dirac::CWMFilter( Picture& picture, const int strength )
{
    CWMFilterComponent( picture.Ydata(), strength ); 
    CWMFilterComponent( picture.Udata(), strength );
    CWMFilterComponent( picture.Vdata(), strength );

}

void dirac::CWMFilterComponent( PicArray& pic_data, const int strength )
{
    // Do centre-weighted median denoising

    PicArray pic_copy( pic_data );

    const int width( 3 );
    const int offset( (width-1)/2 );
    const int centre_weight = std::max(1, (width*width+1)-strength );
    const int list_length = centre_weight+(width*width)-1;

    ValueType* val_list = new ValueType[list_length]; 
    
    for (int j=offset; j<pic_data.LengthY()-offset; ++j){
        for (int i=offset; i<pic_data.LastX()-offset; ++i){
            // Make the value list
            int pos=0;
            for (; pos<centre_weight-1; ++pos)
                val_list[pos] = pic_copy[j][i];
                
            for (int s=-offset; s<=offset; ++s){
                for (int r=-offset; r<=offset; ++r){
                    val_list[pos]=pic_copy[j+s][i+r];
                    pos++;
                }// r
            }// s
            
            pic_data[j][i] = Median( val_list, list_length );
        }// i 
    }// j

    delete[] val_list;
}

ValueType dirac::Median( const ValueType* val_list, const int length)
{


    OneDArray<ValueType> ordered_vals( length );

    // Place the values in order
    int pos=0;
    ordered_vals[0] = val_list[0]; 
    for (int i=1 ; i<length ; ++i )
    {
        for (int k=0 ; k<i ; ++k)
        {
            if (val_list[i]<ordered_vals[k])
            {
                pos=k;
                break;
            }
            else
                pos=k+1;
        }// k

        if ( pos==i)
            ordered_vals[i] = val_list[i];
        else
        {
            for (int k=i-1 ; k>=pos ; --k )
            {
                ordered_vals[k+1] = ordered_vals[k];
            }// k
            ordered_vals[pos] = val_list[i];
        }
    }// i

    // return the middle value
    if ( length%2!=0 )
        return ordered_vals[(length-1)/2];
    else
        return (ordered_vals[(length/2)-1]+ordered_vals[length/2]+1)>>1;
          
}

/*************************************************************/


void VFilter( PicArray& pic_data, const OneDArray<int>& filter, const int bits );
void HFilter( PicArray& pic_data, const OneDArray<int>& filter, const int bits );

double sinxoverx( const double val )
{
    if ( 0.0f == val )
        return 1.0;
    else
        return sin(val)/val;
    
}

OneDArray<int> MakeFilter( const float bw, const int bits )
{
    const int tl = 8;
    const float pi = 3.1415926535;
    
    OneDArray<double> double_filter( Range( -tl, tl ) ); 
    OneDArray<int> int_filter( Range( -tl, tl) );

    // Use the Hanning window
    for (int i=double_filter.First(); i<=double_filter.Last(); ++i)
    {
        double_filter[i] = cos( (pi*i)/
                                   (double_filter.Length()+1) );
    }
    
    // Apply sinc function
    for (int i=double_filter.First(); i<=double_filter.Last(); ++i)
    {
        double_filter[i] *= sinxoverx( pi*1.0*bw*i );       
    }

    // Get DC gain = 1<<bits
    double sum = 0.0;
    for (int i=double_filter.First(); i<=double_filter.Last(); ++i)
        sum += double_filter[i];

    for (int i=double_filter.First(); i<=double_filter.Last(); ++i)
    {
        double_filter[i] *= double(1<<(bits+4));
        double_filter[i] /= sum;
    }
    
    // Turn the float filter into an integer filter
    for (int i=double_filter.First(); i<=double_filter.Last(); ++i)
    {
        int_filter[i] = double_filter[i]>0 ? int( double_filter[i]+0.5 ) : -int( -double_filter[i]+0.5 ); 
        int_filter[i] = (int_filter[i]+8)>>4;
    }

    return int_filter;
}

void dirac::LPFilter( PicArray& pic_data, const float qf, const int strength )
{
    float bw = (std::min( std::max( qf+3.0f-float(strength), 1.0f ), 10.0f ))/10.0;

    // filter with 14-bit accuracy
    OneDArray<int> filter=MakeFilter(bw, 14);    
    
    HFilter( pic_data, filter, 14 );
    VFilter( pic_data, filter, 14 );
    
}

void HFilter( PicArray& pic_data, const OneDArray<int>& filter, const int bits )
{
    ValueType* line_data = new ValueType[pic_data.LengthX()];
    const int offset = (1<<(bits-1));
    
    int sum;

    for (int j=0; j<pic_data.LengthY(); ++j)
    { 
        // Do the first bit
        for (int i=0; i<filter.Last(); ++i)
        {
            sum = offset;
            for (int k=filter.Last(); k>=filter.First(); --k)
                sum += filter[k]*pic_data[j][std::max(i-k,0)];
            sum >>= bits;
            sum = std::min( 127, std::max( -128, sum) );
            line_data[i] = ValueType( sum );
        }// i        
        // Do the middle bit
        for (int i=filter.Last(); i<=pic_data.LastX()+filter.First(); ++i)
        {
            sum = offset;
            for (int k=filter.Last(); k>=filter.First(); --k)
                sum += filter[k]*pic_data[j][i-k];
            sum >>= bits;
            sum = std::min( 127, std::max( -128, sum) );
            line_data[i] = ValueType( sum );
        }// i
        // Do the last bit
        for (int i=pic_data.LastX()+filter.First()+1; i<pic_data.LengthX(); ++i)
        {
            sum = offset;
            for (int k=filter.Last(); k>=filter.First(); --k)
                sum += filter[k]*pic_data[j][std::min(i-k,pic_data.LastX())];
            sum >>= bits;
            sum = std::min( 127, std::max( -128, sum) );
            line_data[i] = ValueType( sum );
        }// i

        // Copy data back
        
        for (int i=0; i<pic_data.LengthX(); ++i )
            pic_data[j][i] = line_data[i];

    }// j

    delete[] line_data;
}

void VFilter( PicArray& pic_data, const OneDArray<int>& filter, const int bits )
{
    PicArray tmp_data( pic_data );
    const int offset = (1<<(bits-1));
    
    int sum;

    // Do the first bit
    for (int j=0; j<filter.Last(); ++j)
    {    

        for (int i=0; i<pic_data.LengthX(); ++i)
        {
            sum = offset;
            for (int k=filter.Last(); k>=filter.First(); --k)
                sum += filter[k]*pic_data[std::max(j-k,0)][i];
            sum >>= bits;
            sum = std::min( 127, std::max( -128, sum) );
            tmp_data[j][i] = ValueType( sum );
        }// i

    }// j

    // Do the middle bit
    for (int j=filter.Last(); j<=pic_data.LastY()+filter.First(); ++j)
    {    

        for (int i=0; i<pic_data.LengthX(); ++i)
        {
            sum = offset;
            for (int k=filter.Last(); k>=filter.First(); --k)
                sum += filter[k]*pic_data[j-k][i];
            sum >>= bits;
            sum = std::min( 127, std::max( -128, sum) );
            tmp_data[j][i] = ValueType( sum );
        }// i

    }// j
    
    // Do the last bit
    for (int j=pic_data.LastY()+filter.First()+1; j<pic_data.LengthY(); ++j)
    {    

        for (int i=0; i<pic_data.LengthX(); ++i)
        {
            sum = offset;
            for (int k=filter.Last(); k>=filter.First(); --k)
                sum += filter[k]*pic_data[std::min(j-k,pic_data.LastY())][i];
            sum >>= bits;
            sum = std::min( 127, std::max( -128, sum) );
            tmp_data[j][i] = ValueType( sum );
        }// i

    }// j

    // Copy data across
    pic_data = tmp_data;

}

/***************************************************************************/

ValueType DiagFilterBchkD( const PicArray& pic, 
                           const int xpos, const int ypos, 
                           const int filter[7][7],
                           const int shift)
{
    // Half the filter length
    const int len2 = 6;

    const int height = pic.LengthY();
    const int width = pic.LengthX();

    int uplus, uneg, vplus, vneg;
    int val = (1<<(shift-1));

    // Do 0 position horizontally
    val += filter[0][0]*pic[ypos][xpos];

    for (int i=1; i<=len2;++i){
            uplus = xpos + i;
            uplus = (uplus>=width ? width-1 : uplus); 
            uneg = xpos - i;
            uneg = (uneg<0 ? 0 : uneg );             
            val += filter[0][i]*(pic[ypos][uplus]+pic[ypos][uneg] );
    }

    // Do other positions vertically//
    //////////////////////////////////

    for (int j=1; j<=len2;++j){
        vplus = ypos + j;
        vplus = ( vplus>=height ? height-1 : vplus); 
        vneg = ypos - j;
        vneg = (vneg<0 ? 0 : vneg );             

        // Do 0 position horizontally
        val += filter[j][0]*(pic[vneg][xpos]+pic[vplus][xpos]);
        for (int i=1; i<=len2;++i){
            uplus = xpos + i;
            uplus = (uplus>=width ? width-1 : uplus); 
            uneg = xpos - i;
            uneg = (uneg<0 ? 0 : uneg );             
            val += filter[j][i]*(pic[vneg][uplus]+pic[vneg][uneg]+
                                 pic[vplus][uplus]+pic[vplus][uneg] );
        }
    }
 
    val >>= shift;

    return ValueType(val);
}

ValueType DiagFilterD( const PicArray& pic, 
                       const int xpos, const int ypos, 
                       const int filter[7][7],
                       const int shift)
{
    // Half the filter length
    const int len2 = 6;

    int uplus, uneg, vplus, vneg;
    int val = (1<<(shift-1));

    // Do 0 position horizontally
    val += filter[0][0]*pic[ypos][xpos];

    for (int i=1; i<=len2;++i){
            uplus = xpos + i;
            uneg = xpos - i;
            val += filter[0][i]*(pic[ypos][uplus]+pic[ypos][uneg] );
    }

    // Do other positions vertically//
    //////////////////////////////////

    for (int j=1; j<=len2;++j){
        vplus = ypos + j;
        vneg = ypos - j;

        // Do 0 position horizontally
        val += filter[j][0]*(pic[vneg][xpos]+pic[vplus][xpos]);
        for (int i=1; i<=len2;++i){
            uplus = xpos + i;
            uneg = xpos - i;
            val += filter[j][i]*(pic[vneg][uplus]+pic[vneg][uneg]+
                                 pic[vplus][uplus]+pic[vplus][uneg] );
        }
    }
 
    val >>= shift;

    return ValueType(val);
}



// Does a diagnonal prefilter
void dirac::DiagFilter( PicArray& pic_data, const float qf, const int strength ){
    // One quadrant of the filter taps 
    int filter[7][7] = {
    {16421,10159,1716,33,325,57,6},
    {10159,5309,-580,-747,44,-43,-25},
    {1716,-580,-2310,-763,100,-19,-12},
    {33,-747,-763,308,326,27,1},
    {325,44,100,326,84,-14,0},
    {57,-43,-19,27,-14,0,0},
    {6,-25,-12,1,0,0,0}
   };

    // Set up the filter based on qf value and filter strength
    float ffactor = (9.0 - qf )/6.0 + float(strength-5)/7.5; 
    int factor = std::max(0, std::min( 256, int( ffactor*256.0 ) ) ) ;

    filter[0][0] = ( factor*filter[0][0] + ( (1<<8)-factor )*(1<<16) + (1<<7) ) >> 8;
 
    for (int i=1;i<7; ++i )
        filter[0][i] = ( factor*filter[0][i] + (1<<7) ) >> 8;

    for (int j=1;j<7; ++j )
        for (int i=0;i<7; ++i )
            filter[j][i] = ( factor*filter[j][i] + (1<<7) ) >> 8;


    PicArray tmp_data( pic_data.LengthY(), pic_data.LengthX(), pic_data.CSort() );

    const int shift = 16;

    for (int j=0; j<7;++j)
        for (int i=0; i<pic_data.LengthX();++i)
            tmp_data[j][i] = DiagFilterBchkD( pic_data, i, j, filter, shift);

    for (int j=7; j<pic_data.LengthY()-7;++j){
        
    for (int i=0; i<7;++i)
            tmp_data[j][i] = DiagFilterBchkD( pic_data, i, j, filter, shift );
        
        for (int i=7; i<pic_data.LengthX()-7;++i)
            tmp_data[j][i] = DiagFilterD( pic_data, i, j, filter, shift );
        
    for (int i=pic_data.LengthX()-7; i<pic_data.LengthX();++i)
            tmp_data[j][i] = DiagFilterBchkD( pic_data, i, j, filter, shift );

    }

    for (int j=pic_data.LengthY()-7; j<pic_data.LengthY();++j)
        for (int i=0; i<pic_data.LengthX();++i)
            tmp_data[j][i] = DiagFilterBchkD( pic_data, i, j, filter, shift );

    pic_data = tmp_data;

}

