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

#ifndef _MODELGLOBALMOTION_H_
#define _MODELGLOBALMOTION_H_

#include <libdirac_common/motion.h>

namespace dirac
{
    //! Base class for modelling global motion
    class ModelGlobalMotion
    {
    public :
        //! Constructor
        ModelGlobalMotion(const MEData &,
                          const EncoderParams &);
    
        //! Destructor
        virtual ~ModelGlobalMotion();
    
        //! Calculate model parameters based on integer motion vectors
        virtual void CalculateModelParameters(const MvArray &,
                                              TwoDArray<int> &,
                                              OneDArray<float>&) = 0;
    
        //! Calculate model parameters based on float motion vectors
        virtual void CalculateModelParameters(MvFloatArray &,
                                              TwoDArray<int> &,
                                              OneDArray<float> &) = 0;
    
        //! Generate integer global motion vector field
        virtual void GenerateGlobalMotionVectors(MvFloatArray &,
                                                 OneDArray<float> &) = 0;
    
        //! Generate float global motion vector field
        virtual void GenerateGlobalMotionVectors(MvArray &,
                                                 OneDArray<float> &) = 0;                                             
    protected :
        //! Float motion vector array
        MvFloatArray m_mv_float;
    
        //! Encoder parameter set
        const EncoderParams & m_encparams;
    
        //
        int m_x_mvs, m_y_mvs;
    
    private :
    
    };
}

#endif
