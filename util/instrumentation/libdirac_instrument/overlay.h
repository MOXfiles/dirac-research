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

#ifndef __OVERLAY_H__
#define __OVERLAY_H__

#include <libdirac_common/common.h>
#include <libdirac_common/motion.h>
#include <libdirac_common/frame.h>
#include <util/instrumentation/libdirac_instrument/motion_colour.h>
#include <util/instrumentation/libdirac_instrument/motion_colour_arrows.h>
#include <util/instrumentation/libdirac_instrument/sad.h>
#include <util/instrumentation/libdirac_instrument/split_mode.h>
#include <util/instrumentation/libdirac_instrument/pred_mode.h>
#include <util/instrumentation/libdirac_instrument/overlay_symbols.h>

//! Enumeration of options for instrumentation overlay
enum OverlayOption
{
    motion_arrows, motion_colour, motion_colour_arrows, split_mode, SAD, pred_mode
    //global_motion_arrows, global_motion_colour, global_motion_colour_arrows, global_motion_difference
};

//! Class holding instrumentation overlay information
class OverlayParams
{
public:

    ////////////////////////////////////////////////////////////
    //                                                        //
    //    Assumes default constructor, copy constructor       //
    //    and assignment =                                    //
    //                                                        //
    ////////////////////////////////////////////////////////////
    
    //! Get functions...
    //! Returns instrumentation command line option
    const OverlayOption Option() const {return m_option;}

    //! Returns which reference is to be used
    const int Reference() const {return m_ref;}

    //! Returns true if input frame background is used, false if grey
    const int Background() const {return m_bg;}

    //! Returns true if colour legend is displayed
    const int Legend() const {return m_legend;}

    //! Returns clip value for motion vectors
    const int MvClip() const {return m_mv_clip;}

    //! Returns clip value for sad
    const int SADClip() const {return m_sad_clip;}
    
    //! Set functions...
    //! Set type of instrumentation
    void SetOption(OverlayOption o) {m_option=o;}

    //! Set which reference frame is to be used
    void SetReference(int r) {m_ref=r;}

    //! Set whether input frame or grey is used as background
    void SetBackground(bool b) {m_bg=b;}

    //! Set display of colour legend
    void SetLegend(bool l) {m_legend=l;}

    //! Set clip for motion vectors
    void SetMvClip(int c) {m_mv_clip=c;}

    //! Set clip for sad
    void SetSADClip(int c) {m_sad_clip=c;}
    
private:
    //! Instrumentation command line option
    OverlayOption m_option;

    //! Reference frame
    int m_ref;

    //! Background - original luma or mid-grey
    bool m_bg;

    //! Instrumentation legend off
    bool m_legend;

    //! Motion vector clip value
    int m_mv_clip;

    //! SAD clip value
    int m_sad_clip;
};

//! Class managing instrumentation overlay
class Overlay
{
public:
    //! constructor
    Overlay(const OverlayParams &, Frame &);

    //! Destructor
    ~Overlay();

    ////////////////////////////////////////////////////////////
    //                                                        //
    //    Assumes default copy constructor and assignment =   //
    //                                                        //
    ////////////////////////////////////////////////////////////

    void ProcessFrame(const MEData &, const OLBParams &);

    void ProcessFrame();
    
private:

    //! Manages overlay based on command-line option
    /*
        Main overlay is carried out on a motion vector block by block basis
    */
    void DoOverlay(const MEData &);

    //! Calculates chroma sample factors
    /*
        Difference picture formats use different chroma resolutions with respect to luma
    */
    void CalculateFactors(const ChromaFormat &);

    //! Calculates if frame requires padding
    /*
        Frames must have an integer number of macroblocks, more macroblocks may have been used
        during encoding and hence the frame must be padded now in order for the correct
        macroblock (and motion vector block) size to be calculated
    */
    void PadFrame(const MEData &);

    //! Temporal scaling factor for motion vectors
    int m_mv_scale;

    //! Local copy of reference offset
    int m_ref;

    //! Overlay parameters
    const OverlayParams m_oparams;

    //! Local reference to frame
    Frame & m_frame;

    //! Parameters for drawing frame overlays
    DrawFrameMotionParams m_draw_params;
};

#endif

