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
 * Contributor(s): Chris Bowley (Original Author)
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

#ifndef _GLOBALMOTION_H_
#define _GLOBALMOTION_H_

#include <libdirac_common/motion.h>
#include <libdirac_common/frame_buffer.h>
#include <libdirac_common/frame.h>
#include <libdirac_motionest/test_mv_sad.h>
#include <libdirac_motionest/reject_sad.h>
#include <libdirac_motionest/reject_local.h>
#include <libdirac_motionest/reject_intensity.h>
#include <libdirac_motionest/reject_value.h>
#include <libdirac_motionest/filter_weighted_local.h>
#include <libdirac_motionest/reject_outlier.h>
#include <libdirac_motionest/reject_edge.h>
#include <libdirac_motionest/model_global_motion.h>

namespace dirac
{
/*!
    Class specifying global motion approximation procedure
*/
    class GlobalMotion
    {
    public :
        //! Constructor
        GlobalMotion(const FrameBuffer &,
                     MEData &,
                     unsigned int,
                     const EncoderParams & encparams);
    
        //! Destructor (empty)
        ~GlobalMotion();
    
        //! Single public function. Interface to carry out approximation process
        void ModelGlobalMotion();
		void ModelGlobalMotion(int);
    
    private :
        //! Frame buffer reference
        const FrameBuffer & m_frame_buffer;
    
        //! Motion estimation data reference
        MEData & m_me_data;
    
        //! Current frame number
        unsigned int m_frame_number;
    
        //! Floating-point global motion array
        MvFloatArray m_gmv;
    
        //! Motion vectors used / rejected from model
        TwoDArray<int> m_inliers;
    
        //! Encoder parameters - includes original video frame size before padding
        const EncoderParams & m_encparams;
    
        //! DEBUG OUTPUT MODE
        bool m_debug;

		// Outlier Rejection Methods:
		int OutlierReject_Edge(int, ModelAffine&, TestGlobalMotionModel&, int);
		int OutlierReject_SAD(int, ModelAffine&, TestGlobalMotionModel&, int);
		int OutlierReject_LocalMean(int, ModelAffine&, TestGlobalMotionModel&, int);
		int OutlierReject_Intensity(int, ModelAffine&, TestGlobalMotionModel&, int);
		int OutlierReject_Value(int, ModelAffine&, TestGlobalMotionModel&, int);
		int OutlierReject_Outlier(int, ModelAffine&, TestGlobalMotionModel&, int);

    };
}

#endif
