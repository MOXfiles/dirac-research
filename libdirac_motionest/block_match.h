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
* Revision 1.2  2004-05-12 08:35:34  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#ifndef _BLOCK_MATCH_H_
#define _BLOCK_MATCH_H_

#include "libdirac_motionest/me_utils.h"

//handles the business of finding the best block match

//! Finds the best match from a list of motion vectors for a given block
/*!
	Finds the best match from a list of motion vectors for a given block. Block parameters and a pointer
	to the list of candidate vectors are encapsulated in 
	/param matchparams
*/
MvCostData FindBestMatch(BMParams& matchparams);//finds best match

//! Finds the best match for sub-pixel refinement
/*!
	Finds the best match at sub-pixel level by refining motion vectors from pixel to half-pel, quarter-pel and then
	1/8 pel.
*/
void FindBestMatchSubp(BMParams& matchparams,const MVector& pel_mv,MvCostData& block_cost);//finds best match for subpel refinement

//! Add a new motion vector list of neighbours of a vector to the set of lists
/*
	Add a new motion vector list to the set of lists consisting of the square neighbourhood [mv.x-xr,mv.x+xr] by 
	[mv.y-yr,mv.y+yr]. Vectors that already occur in previous lists are not added.
*/
void AddNewVlist(std::vector<std::vector<MVector> >& vect_list,const MVector& mv,int xr,int yr);

//! Add a new motion vector list to the set of lists for sub-pixel matching
/*
	Add a new motion vector list to the set of lists consisting of the vectors of the form (mv.x+m*step,mv.y+n*step) where m 
	lies between -xr and xr and n lies between -yr and yr. Vectors that already occur in previous lists are not added. 
*/
void AddNewVlist(std::vector<std::vector<MVector> >& vect_list,const MVector& mv,int xr,int yr,int step);

//! Add a new motion vector list of diagnonal neighbours of a vector to the set of lists 
/*
	Add a new motion vector list to the set of lists consisting of the diagonal neighbourhood of height 2yr+1 pixels and 
	width 2xr+1 centred on \param mv
	Vectors that already occur in previous lists are not added.
*/
void AddNewVlistD(std::vector<std::vector<MVector> >& vect_list,const MVector& mv,int xr,int yr);

//! Add a motion vector to the set of motion vector lists
/*!
	Add a motion vector to the set of motion vector lists, making sure it's not a duplicate.
*/
void AddVect(std::vector<std::vector<MVector> >& vect_list,const MVector& mv,int list_num);

//!	Get the variance of the mode from its neighbours
/*!
	Returns a weighted measure of the difference between a block prediction mode and that of its neighbours
*/
float GetModeVar(const MvData* mv_data,int xindex,int yindex, PredMode predmode,float loc_lambda);

//!	Get the (absolute) variation between two motion vectors
/*!
	Return the variation between two motion vectors, computed as the sum of absolute differences of their components
*/
ValueType GetVar(const MVector& mv1,const MVector& mv2);

//!	Get the (absolute) variation between a motion vector and a list of motion vectors
/*!
	Return the variation between a motion vector and a list of motion vectos, computed as the sum of
	 absolute differences between the components of the vector and the median vector produced by the list
	of vectors
*/
ValueType GetVar(const std::vector<MVector>& pred_list,const MVector& mv);

#endif
