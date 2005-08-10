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


#include <libdirac_decoder/comp_decompress.h>
#include <libdirac_common/wavelet_utils.h>
#include <libdirac_common/band_codec.h>
#include <libdirac_common/golomb.h>
using namespace dirac;

#include <vector>

#include <ctime>

using std::vector;

//Constructor
CompDecompressor::CompDecompressor( DecoderParams& decp, const FrameParams& fp)
:
    m_decparams(decp),
    m_fparams(fp)
{}


void CompDecompressor::Decompress( PicArray& pic_data )
{
	const FrameSort& fsort=m_fparams.FSort();
	const int depth( 4 );

    // A pointer to the object(s) we'll be using for coding the bands
	BandCodec* bdecoder;
    const size_t CONTEXTS_REQUIRED( 24 );

	unsigned int num_band_bytes;

	WaveletTransform wtransform( depth , m_decparams.TransformFilter() );
	SubbandList& bands=wtransform.BandList();

    // Initialise all the subbands
	bands.Init(depth , pic_data.LengthX() , pic_data.LengthY());

    // Set up the code blocks
    SetupCodeBlocks( bands , fsort );

	for ( int b=bands.Length() ; b>=1 ; --b )
	{
		// Read the header data first
        num_band_bytes = ReadBandHeader( m_decparams.BitsIn() , bands(b) );

		if ( !bands(b).Skipped() )
        {
			if ( b>=bands.Length()-3)
            {
				if ( fsort==I_frame && b==bands.Length() )
					bdecoder=new IntraDCBandCodec( &m_decparams.BitsIn() , CONTEXTS_REQUIRED ,bands);
				else
					bdecoder=new LFBandCodec( &m_decparams.BitsIn() , CONTEXTS_REQUIRED ,bands , b);
			}
			else
				bdecoder=new BandCodec( &m_decparams.BitsIn() , CONTEXTS_REQUIRED , bands , b);

			bdecoder->InitContexts();
			bdecoder->Decompress(pic_data , num_band_bytes);
			delete bdecoder;
		}
		else
        {
			if ( b==bands.Length() && fsort==I_frame )
				SetToVal( pic_data , bands(b) , wtransform.GetMeanDCVal() );
			else
				SetToVal( pic_data , bands(b) , 0 );
		}
	}
	wtransform.Transform(BACKWARD,pic_data);
}

int CompDecompressor::ReadBandHeader( BitInputManager& bits_in , Subband& band )
{
    int num_band_bytes( 0 );

    // See if the subband is skipped
    band.SetSkip( bits_in.InputBit() );

    if ( !band.Skipped() )
    {
        // If we're not skipped, we need a quantisation index for the subband
        band.SetQIndex( UnsignedGolombDecode( bits_in ) );

        // We also need to say whether there are multiple quantisers in the subband.
        // If so, offsets from the band QIndex will be decoded in the code block
        // headers in the arithmetic coded subband data.
        band.SetUsingMultiQuants( bits_in.InputBit() );

        if ( !band.UsingMultiQuants() )
        {
            // Propogate the quantiser index to all the code blocks if we 
            // don't have multiquants
            for ( int j=0 ; j<band.GetCodeBlocks().LengthY() ; ++j )
                for ( int i=0 ; i<band.GetCodeBlocks().LengthX() ; ++i )
                   band.GetCodeBlocks()[j][i].SetQIndex( band.QIndex() );
        }
        // In the multiquant case, quantiser indexes for
        // code blocks will be coded as offsets in the entropy-coded stream
    
        num_band_bytes = UnsignedGolombDecode( bits_in );
    }
    bits_in.FlushInput();

    // Read the number of bits read for the band and return
    return num_band_bytes;
}

void CompDecompressor::SetupCodeBlocks( SubbandList& bands , const FrameSort fsort )
{
    int xregions;
    int yregions;

    // The minimum x and y dimensions of a block
    const int min_dim( 4 );
  
    // The maximum number of regions horizontally and vertically
    int max_xregion, max_yregion;

    for (int band_num = 1; band_num<=bands.Length() ; ++band_num)
    {
        if ( band_num < bands.Length()-6 )
        {
            if ( fsort != I_frame )
            {
                xregions = 12;
                yregions = 8;
            }
            else
            {
                xregions = 4;
                yregions = 3;
            }
        }
        else if (band_num < bands.Length()-3)
        {
            if ( fsort != I_frame )
            {
                xregions = 8;
                yregions = 6;
            }
            else
            {
                xregions = 1;
                yregions = 1;
            }
        }
        else
        {
            xregions = 1;
            yregions = 1;
        }

        max_xregion = bands( band_num ).Xl() / min_dim;
        max_yregion = bands( band_num ).Yl() / min_dim;

        bands( band_num ).SetNumBlocks( std::min( yregions , max_yregion ), 
                                        std::min( xregions , max_xregion ) );

    }// band_num
}

void CompDecompressor::SetToVal( PicArray& pic_data , 
                                 const Subband& node , 
                                 ValueType val )
{

	for (int j=node.Yp() ; j<node.Yp()+node.Yl() ; ++j)
		for (int i=node.Xp() ; i<node.Xp()+node.Xl() ; ++i)
			pic_data[j][i]=val;

}
