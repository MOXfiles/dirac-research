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


#ifndef _MOTION_ESTIMATE_H_
#define _MOTION_ESTIMATE_H_

#include "libdirac_common/motion.h"

class FrameBuffer;


//! Class to handle the whole motion estimation process 
class MotionEstimator{
public:
	//! Constructor
	MotionEstimator(const EncoderParams& params);
	//! Destructor
	~MotionEstimator(){}

	//! Do the motion estimation
	void DoME(const FrameBuffer& my_buffer,int frame_num, MvData& mv_data);

private:
	//Copy constructor
	MotionEstimator(const MotionEstimator& cpy);//private, body-less copy constructor: class should not be copied
	//Assignment= 
	MotionEstimator& operator=(const MotionEstimator& rhs);//private, body-less copy constructor: class should not be copied

	//internal data
	FrameSort fsort;
	EncoderParams encparams;
	int xr, yr; // search ranges for block matching
	int depth;

	//functions
	void DoHierarchicalSearch(const FrameBuffer& my_buffer, int frame_num, MvData& mv_data); 	//find vectors to pixel accuracy
																						//using a hierarchical search method
	void DoFinalSearch(const FrameBuffer& my_buffer, int frame_num, MvData& mv_data);		//refine vectors to 1/8 pel accuracy, given
																					//pixel-accurate vectors
	void MatchPic(const PicArray& ref_data,const PicArray& pic_data,MvData& mv_data,const MvData& guide_data,
		int ref_id,int level);												//do a basic matching from pic_data to reference
																			//ref_data by block-matching, using guide data from
																			//lower levels in the resolution hierarchy
	void SetChromaDC(const FrameBuffer& my_buffer, int frame_num, MvData& mv_data);	//go through all the intra blocks and extract the
																				//dc values to be coded from the chroma components
	void SetChromaDC(const PicArray& pic_data, MvData& mv_data,CompSort csort);		//Called by previous fn for each component
	ValueType GetChromaBlockDC(const PicArray& pic_data, MvData& mv_data,int xloc,int yloc,int split);
};

#endif
