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
* Revision 1.2  2004-04-06 18:06:53  chaoticcoyote
* Boilerplate for Doxygen comments; testing ability to commit into SF CVS
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:09  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
* Motion compensation by Richard Felton, with mods by Thomas Davies.
*/

//  Motion Compensation routines.
//  Supports different sizes of blocks as long as the parameters
// 	describing them are 'legal'. Blocks overlap the edge of the image
// 	being written to but blocks in the reference image are forced to
// 	lie completely within the image bounds.

#ifndef _INCLUDED_MOT_COMP
#define _INCLUDED_MOT_COMP

#include <cstdlib>
#include <ctime>
#include <iostream>
#include "common.h"
#include "upconvert.h"
#include "motion.h"
#include "gop.h"

//Motion compensation class, for all your motion compensation needs
//! 
/*!

 */
class MotionCompensator{

public:

    //! 
    /*!
        
     */
	//Constructor sets up the member variables using information
	//from cparams.
	MotionCompensator(const CodecParams &cp);

    //! 
    /*!
        
     */
	//Destructor
	~MotionCompensator();

    //! 
    /*!
        
     */
	void SetCompensationMode(AddOrSub a_or_s) {add_or_sub=a_or_s;}	//Toggles the MC mode


    //! 
    /*!
        
     */
	//Perform motion compensated addition/subtraction on pic_data
	//Note for L1 frames you can duplicate arguments so UpConv and UpConvTwo are
	//the same thing - the function will not use UpConvTwo unless there are L2 blocks
	//in the frame.
	void CompensateFrame(Gop& my_gop,int fnum,MvData& mv_data);	//motion compensate a given frame

private:
	//functions
	void CompensateComponent(Frame& picframe, Frame &ref1frame, Frame& ref2frame, MvData& mv_data,CompSort cs);//MC a component
	void CompensateBlock(PicArray &pic_data, PicArray &refup_data, MVector &Vec, ImageCoords Pos, CalcValueType** Weights);
	void DCBlock(PicArray &pic_data,ValueType dc, ImageCoords Pos, CalcValueType** Weights);	
	int InterpLookup[9][4];//A lookup table to simplify the 1/8 pixel accuracy code
	void ReConfig();		//Recalculates the weight matrix and stores other key block related parameters.

	//variables	
	CodecParams cparams;
	bool luma_or_chroma;	//true if we're doing luma, false if we're coding chroma
	ArithObj* arith;		//Pointer to generic arithmetic object
	ArithObj* add;			//Particular arith obj
	ArithObj* subtract;		//ditto
	ArithObj* subtracthalf;	//ditto
	ArithObj* addhalf;		//ditto
	CalcValueType*** BlockWeights;			//My weighting block to allow overlapping blocks
	AddOrSub add_or_sub;					//Motion compensated Addition/Subtraction flag

	//Image and block information
	OLBParams bparams;	//either luma or chroma block parameters
	int	xBlockSize,yBlockSize;
	int ImageWidth;
	int ImageHeight;
};

#endif
