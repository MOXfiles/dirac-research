/* ***** BEGIN LICENSE BLOCK *****
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
* Contributor(s):
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

/*
*
* $Author$
* $Revision$
* $Log$
* Revision 1.5  2004-06-18 15:58:36  tjdwave
* Removed chroma format parameter cformat from CodecParams and derived
* classes to avoid duplication. Made consequential minor mods to
* seq_{de}compress and frame_{de}compress code.
* Revised motion compensation to use built-in arrays for weighting
* matrices and to enforce their const-ness.
* Removed unnecessary memory (de)allocations from Frame class copy constructor
* and assignment operator.
*
* Revision 1.4  2004/05/12 08:35:34  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.3  2004/04/11 22:50:46  chaoticcoyote
* Modifications to allow compilation by Visual C++ 6.0
* Changed local for loop declarations into function-wide definitions
* Replaced variable array declarations with new/delete of dynamic array
* Added second argument to allocator::alloc calls, since MS has no default
* Fixed missing and namespace problems with min, max, cos, and abs
* Added typedef unsigned int uint (MS does not have this)
* Added a few missing std:: qualifiers that GCC didn't require
*
* Revision 1.2  2004/04/06 18:06:53  chaoticcoyote
* Boilerplate for Doxygen comments; testing ability to commit into SF CVS
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include "libdirac_common/bit_manager.h"
#include "libdirac_common/arrays.h"
#include "libdirac_common/context.h"
#include <vector>

//common header for the encoder and decoder
//basic types used throughout the codec

//! Types of chroma formatting (formatNK=format not known)
enum ChromaFormat {Yonly,format422, format444, format420, format411,formatNK};
//! Types of frame
enum FrameSort{I_frame,L1_frame,L2_frame};
//! Prediction modes for blocks
enum PredMode{INTRA,REF1_ONLY,REF2_ONLY,REF1AND2};
//! Types of picture component
enum CompSort{Y,U,V,R,G,B};
//! Addition or subtraction
enum AddOrSub{ADD,SUBTRACT};
//! Forward or backward
enum Direction {FORWARD,BACKWARD};
//! Currently only Daubechies (DAUB) implemented
enum WltFilter {DAUB,HAAR};

//! A class for picture component data.
/*!
	A class for encapsulating picture data, derived from TwoDArray. NB: in the future there will be
	separate classes for input/output picture data, difference picture data, and wavelet coefficient
	data. Currently PicArray is used for all of these. TJD 13 April 2004.
 */
class PicArray: public TwoDArray<ValueType>{
public:
    //! Default constructor
    /*!
        Default constructor creates an empty array. 
     */	
	PicArray(): TwoDArray<ValueType>(){}
    //! Constructor.
    /*!
        Contructor creates a two-D array, with component sort=Y.
     */	
	PicArray(int xl,int yl): TwoDArray<ValueType>(xl,yl),csort(Y){}
	//copy constructor and assignment= derived by inheritance

    //! Destructor	
	~PicArray(){}

	CompSort csort;
};

//! A class for recording costs, particularly in quantisation.
class CostType{
public:
	//! The Mean Square Error	
	double MSE;
    //! The entropy in bits per symbol.
	double ENTROPY;
    //! The Lagrangian combination of MSE+lambda*entropy	
	double TOTAL;
};

//! Contexts used for coefficient coding
enum CtxAliases{//used for residual coding
	SIGN0_CTX,		//0		-sign, previous symbol is 0
	SIGN_POS_CTX,	//1		-sign, previous symbol is +ve
	SIGN_NEG_CTX,	//2		-sign, previous symbol is -ve


	Z_BIN1z_CTX,	//3		-bin 1, parent is zero, neighbours zero
	Z_BIN1nz_CTX,	//4		-bin 1, parent is zero, neighbours non-zero
	Z_BIN2_CTX,		//5		-bin 2, parent is zero
	Z_BIN3_CTX,		//6		-bin 3, parent is zero
	Z_BIN4_CTX,		//7		-bin 4, parent is zero
	Z_BIN5plus_CTX,	//8		-bins 5 plus, parent is zero

	NZ_BIN1z_CTX,	//9		-bin 1, parent is non-zero, neighbours zero
	NZ_BIN1a_CTX,	//10	-bin 1, parent is non-zero, neighbours small
	NZ_BIN1b_CTX,	//11	-bin 1, parent is non-zero, neighbours large
	NZ_BIN2_CTX,	//12	-bin 2, parent is non-zero
	NZ_BIN3_CTX,	//13	-bin 3, parent is non-zero
	NZ_BIN4_CTX,	//14	-bin 4, parent is non-zero
	NZ_BIN5plus_CTX,//15	-bins 5 plus, parent is non-zero

	ZTz_CTX,		//16	-zerotree, neighbouring symbols are zerotree elements
	ZTnz_CTX,		//17	-zerotree, neighbouring symbols are not zerotree elements
	ZTzb_CTX,		//16	-zerotree, neighbouring symbols are zerotree elements
	ZTnzb_CTX		//17	-zerotree, neighbouring symbols are not zerotree elements
};

//! Contexts used for MV data coding
enum MvCtxAliases{

	YDC_BIN1_CTX,		//0 	-1st bin of DC value for Y
	YDC_BIN2plus_CTX,	//1 	-remaining DC bins
	YDC_SIGN0_CTX,		//2		-sign of Y DC value, previous value 0
	UDC_BIN1_CTX,		//3 	--ditto
	UDC_BIN2plus_CTX,	//4 	--for
	UDC_SIGN0_CTX,		//5		--U
	VDC_BIN1_CTX,		//6 	--and
	VDC_BIN2plus_CTX,	//7 	--V
	VDC_SIGN0_CTX,		//8		--components

	REF1x_BIN1_CTX,		//9		-bin 1, REF1 x vals
	REF1x_BIN2_CTX,		//10	-bin 2, REF1 x vals
	REF1x_BIN3_CTX,		//11	-bin 3, REF1 x vals
	REF1x_BIN4_CTX,		//12	-bin 4, REF1 x vals
	REF1x_BIN5plus_CTX,	//13	-bin 5, REF1 x vals
	REF1x_SIGN0_CTX,	//14	-sign, REF1 x vals, previous value 0
	REF1x_SIGNP_CTX,	//15	-sign, REF1 x vals, previous value +ve
	REF1x_SIGNN_CTX,	//16	-sign, REF1 x vals, previous value -ve

	REF1y_BIN1_CTX,		//17		-bin 1, REF1 y vals
	REF1y_BIN2_CTX,		//18	-bin 2, REF1 y vals
	REF1y_BIN3_CTX,		//19	-bin 3, REF1 y vals
	REF1y_BIN4_CTX,		//20	-bin 4, REF1 y vals
	REF1y_BIN5plus_CTX,	//21	-bin 5, REF1 y vals
	REF1y_SIGN0_CTX,	//22	-sign, REF1 y vals, previous value 0
	REF1y_SIGNP_CTX,	//23	-sign, REF1 y vals, previous value +ve
	REF1y_SIGNN_CTX,	//24	-sign, REF1 y vals, previous value -ve

	REF2x_BIN1_CTX,		//25	-bin 1, REF2 x vals
	REF2x_BIN2_CTX,		//26	-bin 2, REF2 x vals
	REF2x_BIN3_CTX,		//27	-bin 3, REF2 x vals
	REF2x_BIN4_CTX,		//28	-bin 4, REF2 x vals
	REF2x_BIN5plus_CTX,	//29	-bin 5, REF2 x vals
	REF2x_SIGN0_CTX,	//30	-sign, REF2 x vals, previous value 0
	REF2x_SIGNP_CTX,	//31	-sign, REF1 y vals, previous value +ve
	REF2x_SIGNN_CTX,	//32	-sign, REF1 y vals, previous value -ve

	REF2y_BIN1_CTX,		//33	-bin 1, REF2 y vals
	REF2y_BIN2_CTX,		//34	-bin 2, REF2 y vals
	REF2y_BIN3_CTX,		//35	-bin 3, REF2 y vals
	REF2y_BIN4_CTX,		//36	-bin 4, REF2 y vals
	REF2y_BIN5plus_CTX,	//37	-bin 5, REF2 y vals
	REF2y_SIGN0_CTX,	//38	-sign, REF2 y vals, previous value 0
	REF2y_SIGNP_CTX,	//39	-sign, REF2 y vals, previous value +ve
	REF2y_SIGNN_CTX,	//40	-sign, REF2 y vals, previous value -ve

	PMODE_BIN1_CTX,		//41	-bin 1, prediction mode value
	PMODE_BIN2_CTX,		//42	-bin 2, prediction mode value
	PMODE_BIN3_CTX,		//43	-bin 3, prediction mode value. Bin 4 not required

	MB_CMODE_CTX,		//44	-context for MB common block mode
	MB_SPLIT_BIN1_CTX,	//45	-bin1, MB split mode vals
	MB_SPLIT_BIN2_CTX	//46	-bin2, MB split mode vals. Bin 3 not required
};

//! A class used for correcting estimates of entropy.
/*!
	A class used by the encoder for correcting estimates of entropy. Used for selecting quantisers in
	subband coefficient coding. Factors can be adjusted in the light of previous experience.
 */
class EntropyCorrector{//factors for correcting entropy estimates
public:
	//! Constructor.
    /*!
        Constructs arrays of correction factors of size.
		/param	depth	the depth of the wavelet transform.
     */
	EntropyCorrector(int depth): Yfctrs(3*depth+1,3),Ufctrs(3*depth+1,3),Vfctrs(3*depth+1,3){Init();}

	////////////////////////////////////////////////////////////////////
	//NB: Assume default copy constructor, assignment = and destructor//
	////////////////////////////////////////////////////////////////////	

	//! Returns the correction factor.
    /*!
        Returns the correction factor for the band given also the type of frame and component.
     */
	float Factor(const int bandnum, const FrameSort fsort,const CompSort c) const;
    //! Update the correction factors.
    /*!
        Update the factors for a given subband, component and frame type.
		/param	est_bits	the number of bits it was estimated would be used
		/param	actual_bits	the number of bits that actually were used
     */	
	void Update(int bandnum, FrameSort fsort, CompSort c,int est_bits,int actual_bits);
private:
	void Init();//initialises the correction factors
	TwoDArray<float> Yfctrs;
	TwoDArray<float> Ufctrs;
	TwoDArray<float> Vfctrs;
};

//! Parameters for overlapped block motion compensation
struct OLBParams{//params for overlapped blocks

    //! The horizontal block length
	int XBLEN;
	//! The vertical block length
	int YBLEN;
	//! The horizontal block separation
	int XBSEP;
	//! The vertical block separation
	int YBSEP;
	//! The offset in the horizontal start of the block caused by overlap,=(XBLEN-XBSEP)/2
	int XOFFSET;
	//! The offset in the vertical start of the block caused by overlap,=(YBLEN-YBSEP)/2
	int YOFFSET;
};

//! Parameters relating to the video sequence being encoded/decoded
struct SeqParams{

	//! Constructor 
	SeqParams(): 
	xl(0),
	yl(0),
	zl(0),
	cformat(format422),
	interlace(false),
	topfieldfirst(true),
	framerate(12){}

	////////////////////////////////////////////////////////////////////
	//NB: Assume default copy constructor, assignment = and destructor//
	////////////////////////////////////////////////////////////////////	

	   //! Width of video
	int xl;

    //! Height of video
	int yl;

    //! Number of frames in the sequence 
	int zl;	

    //! Presence of chroma and/or chroma sampling structure 
	ChromaFormat cformat;

    //! True if interlaced
	bool interlace;

    //! If interlaced, true if the top field is first in temporal order
	bool topfieldfirst;

    //! Frame rate, per second
	int framerate;
};

//! Parameters common to coder and decoder operation
/*!
	Parameters used throughout both the encoder and the decoder
*/
struct CodecParams{

	//! Default constructor 
	CodecParams(): 
	X_NUM_MB(0),
	Y_NUM_MB(0),
	X_NUMBLOCKS(0),
	Y_NUMBLOCKS(0),
	VERBOSE(false),
	//	cformat(Yonly),
	interlace(false),
	topfieldfirst(false),
	lbparams(3),
	cbparams(3){}

	////////////////////////////////////////////////////////////////////
	//NB: Assume default copy constructor, assignment = and destructor//
	////////////////////////////////////////////////////////////////////

	//! Return the Luma block parameters for each macroblock splitting level
	const OLBParams& LumaBParams(int n) const {return lbparams[n];}
	//! Return the Chroma block parameters for each macroblock splitting level
	const OLBParams& ChromaBParams(int n) const {return cbparams[n];}
	//! Set the block sizes for all MB splitting levels given these prototype block sizes for level=2
	void SetBlockSizes(const OLBParams& olbparams, ChromaFormat cformat);

	//! The number of macroblocks horizontally
	int X_NUM_MB;
	//! The number of macroblocks verticaly
	int Y_NUM_MB;

    //! The number of blocks horizontally
	int X_NUMBLOCKS;	
	//! The number of blocks vertically
	int Y_NUMBLOCKS;

    //! Code/decode with commentary if true	
	bool VERBOSE;

	//! True if input is interlaced, false otherwise
	bool interlace;

	//! True if interlaced and top field is first in temporal order 
	bool topfieldfirst;

private:
	OneDArray<OLBParams> lbparams;
	OneDArray<OLBParams> cbparams;
};

//! Parameters for the encoding process
/*!
	Parameters for the encoding process, derived from CodecParams.
 */
class EncoderParams: public CodecParams{//codec params plus parameters relating solely to the operation of the encoder

public:
//! Default constructor	
	EncoderParams(): 
	CodecParams(),
	NUM_L1(0),
	L1_SEP(0),
	UFACTOR(1.0),
	VFACTOR(1.0),
	CPD(20.0),
	I_lambda(0.f),
	L1_lambda(0.0f),
	L2_lambda(0.0f),
	L1I_lambda(0.0f),
	L1_ME_lambda(0.0f),
	L2_ME_lambda(0.0f),
	L1I_ME_lambda(0.0f),
	EntCorrect(0),
	BIT_OUT(0){}

	////////////////////////////////////////////////////////////////////
	//NB: Assume default copy constructor, assignment = and destructor//
	//This means pointers are copied, not the objects they point to.////   	
	////////////////////////////////////////////////////////////////////

	//! Number of L1 frames before next I frame
	int NUM_L1;

	//! Separation between L1 frames
	int L1_SEP;	

	  //! factor for weighting U component quantisation errors 
	float UFACTOR;

    //! factor for weighting V component quantisation errors 
	float VFACTOR;

    //! Cycles per degree assumed for viewing the video
	float CPD;

	//! Lagrangian parameters for coding
	float I_lambda,L1_lambda,L2_lambda,L1I_lambda;
	//! Lagrangian params for motion estimation
	float L1_ME_lambda,L2_ME_lambda,L1I_ME_lambda;

    //! Correction factors for quantiser selection 
	EntropyCorrector* EntCorrect;

    //! Pointer to object for managing bitstream output
	BitOutputManager* BIT_OUT;
};

//! Parameters for the decoding process
/*!
	Parameters for the decoding process. Derived from CodecParams.
 */
class DecoderParams: public CodecParams{
public:
	//! Default constructor
	DecoderParams(): CodecParams(),BIT_IN(0){}

	////////////////////////////////////////////////////////////////////
	//NB: Assume default copy constructor, assignment = and destructor//
	//This means pointers are copied, not the objects they point to.////   	
	////////////////////////////////////////////////////////////////////

	//! Pointer to the bitstream input manager
	BitInputManager* BIT_IN;
};

//! Parameters for initialising frame class objects
struct FrameParams {

	//! Default constructor
	FrameParams(): 
	fsort(I_frame),
	output(false){}	

    //! Constructor 
    /*!
		Frame chroma format is set Frame sort defaults to I frame.
     */	
	FrameParams(const ChromaFormat& cf, int xlen, int ylen): 
	cformat(cf),
	xl(xlen),
	yl(ylen),
	fsort(I_frame),
	output(false){}

    //! Constructor
    /*!
		Frame chroma format and frame sort are set.
     */	
	FrameParams(const ChromaFormat& cf, const FrameSort& fs): 
	cformat(cf),
	fsort(fs),
	output(false){}	

	////////////////////////////////////////////////////////////////////
	//NB: Assume default copy constructor, assignment = and destructor//
	////////////////////////////////////////////////////////////////////	

	//! The chroma format
	ChromaFormat cformat;
	//! Frame width
	int xl;
	//!	Frame height
	int yl;
	//! The frame sort
	FrameSort fsort;
	//! The set of frame numbers of reference frames
	std::vector<int> refs;
	//! The number of frames, after the current frame number, after the (de)coding of which the frame can be deleted
	int expiry_time;
	//! True if the frame has been output, false if not
	bool output;
	//! The frame number, in temporal order
	int fnum;		
};

//! A simple bounds checking function, very useful in a number of places
inline ValueType BChk(const ValueType &num, const ValueType &max){
	if(num < 0) return 0;
	else if(num >= max) return max-1;
	else return num;
}

#endif
