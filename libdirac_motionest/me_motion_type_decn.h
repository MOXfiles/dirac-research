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
* Contributor(s): Thomas Davies (Original Author)
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

#ifndef _ME_MOTION_TYPE_DECN_H_
#define _ME_MOTION_TYPE_DECN_H_

#include <libdirac_common/motion.h>
#include <libdirac_motionest/me_mode_decn.h>
#include <libdirac_motionest/block_match.h>

namespace dirac
{
    //class FrameBuffer;

    class MotionTypeDecider
    {

    public:
		//! Constructor
        MotionTypeDecider();    

        //! Destructor
        ~MotionTypeDecider();

        //! Does the actual decision between global and block motion
        /*!
            Does the motion type decision
            \param    my_buffer    the buffer of all the relevant frames
            \param    frame_num    the frame number for which motion estimation is being done
            \param    me_data    the motion vector data into which decisions will be written
         */
        //int DoMotionTypeDecn( const FrameBuffer& my_buffer , int frame_num , MEData& me_data);
        int DoMotionTypeDecn( MvData& in_data);

    private:
        MotionTypeDecider( const MotionTypeDecider& cpy );//private, body-less copy constructor: this class should not be copied
        MotionTypeDecider& operator=( const MotionTypeDecider& rhs );//private, body-less assignment=: this class should not be assigned

        //! Decide on a motion type for a given prediction unit (block, sub-MB or MB)
        //float DoUnitDecn( const int xpos , const int ypos , const int level );
        void DoUnitDecn( MvData& in_data );

         // Member data
        FrameSort fsort;

		//! Motion vector data for each level of splitting
        OneDArray< MEData* > m_me_data_set;

		int b_xp, b_yp;            //position of current block
        int mb_xp, mb_yp;        //position of current MB
        int mb_tlb_x, mb_tlb_y;    //position of top-left block of current MB
		
		/*
        //! A local reference to the encoder params
        const EncoderParams& m_encparams;

        //! The Lagrangian parameter for motion estimation
        float m_lambda;

        //! Correction factor for comparing SAD costs for different MB splittings
        OneDArray<float> m_level_factor;


        //! Correction factor for comparing mode costs for different MB splittings
        OneDArray<float> m_mode_factor;


        const PicArray* m_pic_data;
        const PicArray* m_ref1_updata;
        const PicArray* m_ref2_updata;
        int num_refs;

        IntraBlockDiff* m_intradiff;
        BiBChkBlockDiffUp* m_bicheckdiff;

        //position variables, used in all the mode decisions
        int m_xmb_loc,m_ymb_loc;    //coords of the current MB
*/
    };

} // namespace dirac

#endif
