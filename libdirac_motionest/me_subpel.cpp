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
* Revision 0.1.0  2004/02/20 09:36:09  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#include "libdirac_motionest/me_subpel.h"
#include "libdirac_common/frame_buffer.h"

#include <iostream>

using std::vector;

SubpelRefine::SubpelRefine(EncoderParams& cp): 
encparams(cp),
nshift(4),
lambda(3)
{
	//define the relative coordinates of the four neighbours	
	nshift[0].x=-1; nshift[0].y=0;
	nshift[1].x=-1; nshift[1].y=-1;
	nshift[2].x=0; nshift[2].y=-1;
	nshift[3].x=1; nshift[3].y=-1;
}

void SubpelRefine::DoSubpel(const FrameBuffer& my_buffer,int frame_num, MvData& mvd){
	//main loop for the subpel refinement
	int ref1,ref2;

 	//these factors normalise costs for sub-MBs and MBs to those of blocks, so that the overlap is
 	//take into account (e.g. a sub-MB has length XBLEN+XBSEP and YBLEN+YBSEP):
	factor1x1=float(16*encparams.LumaBParams(2).XBLEN*encparams.LumaBParams(2).YBLEN)/
		float(encparams.LumaBParams(0).XBLEN*encparams.LumaBParams(0).YBLEN);
	factor2x2=float(4*encparams.LumaBParams(2).XBLEN*encparams.LumaBParams(2).YBLEN)/
		float(encparams.LumaBParams(1).XBLEN*encparams.LumaBParams(1).YBLEN);

	const FrameSort fsort=my_buffer.GetFrame(frame_num).GetFparams().fsort;
	if (fsort!=I_frame){
		if (fsort==L1_frame)
			lambda=encparams.L1_ME_lambda;
		else
			lambda=encparams.L2_ME_lambda;

		mv_data=&mvd;
		matchparams.pic_data=&(my_buffer.GetComponent(frame_num,Y));

		const vector<int>& refs=my_buffer.GetFrame(frame_num).GetFparams().refs;
		num_refs=refs.size();
		ref1=refs[0];
		if (num_refs>1)
			ref2=refs[1];
		else	
			ref2=ref1;
		up1_data=&(my_buffer.GetUpComponent(ref1,Y));
		up2_data=&(my_buffer.GetUpComponent(ref2,Y));

		for (int yblock=0;yblock<encparams.Y_NUMBLOCKS;++yblock){
			for (int xblock=0;xblock<encparams.X_NUMBLOCKS;++xblock){				
				DoBlock(xblock,yblock);
			}//xblock		
		}//yblock		
	}
}

void SubpelRefine::DoBlock(int xblock,int yblock){
	vector<vector<MVector> > vect_list;
	matchparams.vect_list=&vect_list;
	matchparams.me_lambda=lambda;
	const OLBParams& lbparams=encparams.LumaBParams(2);
	matchparams.Init(lbparams,xblock,yblock);

	//do the first reference
	const MvArray* mvarray=&(mv_data->mv1);
	matchparams.ref_data=up1_data;
	matchparams.mv_pred=GetPred(xblock,yblock,*mvarray);
	FindBestMatchSubp(matchparams,(*mvarray)[yblock][xblock],(mv_data->block_costs1)[yblock][xblock]);
	(*mvarray)[yblock][xblock]=matchparams.best_mv;
	if (num_refs>1){
		//do the second reference
		mvarray=&(mv_data->mv2);
		matchparams.ref_data=up2_data;
		matchparams.mv_pred=GetPred(xblock,yblock,*mvarray);
		FindBestMatchSubp(matchparams,(*mvarray)[yblock][xblock],(mv_data->block_costs2)[yblock][xblock]);
		(*mvarray)[yblock][xblock]=matchparams.best_mv;		
	}
}

MVector SubpelRefine::GetPred(int xblock,int yblock,const MvArray& mvarray){
	MVector mv_pred;
	ImageCoords n_coords;
	vector<MVector> neighbours;

	if (xblock>0 && yblock>0 && xblock<mvarray.last(0)){
		for (int I=0;I<nshift.length();++I){
			n_coords.x=xblock+nshift[I].x;
			n_coords.y=yblock+nshift[I].y;
			neighbours.push_back(mvarray[n_coords.y][n_coords.x]);
		}//I
	}
	else {
		for (int I=0;I<nshift.length();++I){
			n_coords.x=xblock+nshift[I].x;
			n_coords.y=yblock+nshift[I].y;
			if (n_coords.x>=0 && n_coords.y>=0 && n_coords.x<mvarray.length(0) && n_coords.y<mvarray.length(1)){
				neighbours.push_back(mvarray[n_coords.y][n_coords.x]);
			}
		}//I
	}
	mv_pred=MvMedian(neighbours);
	return mv_pred;
}
