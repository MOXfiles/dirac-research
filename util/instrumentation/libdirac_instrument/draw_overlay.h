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

#ifndef _BLOCK_OVERLAY_H_
#define _BLOCK_OVERLAY_H_

#include <libdirac_common/motion.h>
#include <libdirac_common/frame.h>
#include <util/instrumentation/libdirac_instrument/overlay_symbols.h>

//! Class encapsulating parameters for drawing the frame
class DrawFrameMotionParams
{
public :

    //! Default constuctor, does nothing
    DrawFrameMotionParams() {}

    //! Default destructor, does nothing
    ~DrawFrameMotionParams() {}
    //! Gets...
    //! Returns luma motion vector block height
    const int MvYBlockY() const {return m_mv_Y_block_y;}
    
    //! Returns luma motion vector block width
    const int MvYBlockX() const {return m_mv_Y_block_x;}

    //! Returns chroma motion vector height
    const int MvUVBlockY() const {return m_mv_UV_block_y;}

    //! Returns chroma motion vector width
    const int MvUVBlockX() const {return m_mv_UV_block_x;}

    //! Returns chroma - luma vertical smaple ratio
    const int ChromaFactorY() const {return m_chroma_factor_y;}
    
    //! Returns chroma - luma horizontal sample ratio
    const int ChromaFactorX() const {return m_chroma_factor_x;}

    //! Returns original picture height
    const int PicY() const {return m_pic_y;}

    //! Returns original picture width
    const int PicX() const {return m_pic_x;}

    //! Sets...
    //! Set luma motion vector block height    
    void SetMvYBlockY(int y) {m_mv_Y_block_y=y;}
    
    //! Set luma motion vector block width
    void SetMvYBlockX(int x) {m_mv_Y_block_x=x;}
    
    //! Set chroma motion vector block height
    void SetMvUVBlockY(int y) {m_mv_UV_block_y=y;}
    
    //! Set chroma motion vector block width
    void SetMvUVBlockX(int x) {m_mv_UV_block_x=x;}
    
    //! Set chroma - luma vertical sample ratio
    void SetChromaFactorY(int y) {m_chroma_factor_y=y;}
    
    //! Set chroma - luma horizontal sample ratio
    void SetChromaFactorX(int x) {m_chroma_factor_x=x;}

    //! Set original picture height
    void SetPicY(int y) {m_pic_y=y;}

    //! Set original picture width
    void SetPicX(int x) {m_pic_x=x;}
    
private :

    //! Motion vector block dimensions - luma
    int m_mv_Y_block_y, m_mv_Y_block_x;
    
    //! Motion vector block dimensions - chroma
    int m_mv_UV_block_y, m_mv_UV_block_x;
    
    //! Chroma - luma sample ratio
    int m_chroma_factor_y, m_chroma_factor_x;

    //! Original picture dimensions
    int m_pic_y, m_pic_x;
};

//! Base class for block overlay objects
/*
    Base class for block overlay objects with pure
    virtual functions to define sub-class interface
*/
class DrawOverlay
{
public :
    //! Constructor
    DrawOverlay(Frame &, DrawFrameMotionParams &);
    
    //! Destructor
    virtual ~DrawOverlay();
    
    //! Carries out overlay for single block
    virtual void DrawBlock(int, int)=0;
    
    //! Draws overlay legend
    virtual void DrawLegend()=0;

    ////////////////////////////////////////////////////////////
    //                                                        //
    //    Assumes default copy constructor and assignment =   //
    //                                                        //
    ////////////////////////////////////////////////////////////

    //! Draws frame numbers for both references
    void DrawReferenceNumbers(int, int);
    
    //! Draws frame number for chosen reference
    void DrawReferenceNumber(int, int);
    
    //! Draws current frame number
    void DrawFrameNumber(int);
    
    //! Draws a character / number / symbol
    void DrawCharacter(const PicArray &, int, int);

    //! Returns reference to symbols object
    const OverlaySymbols & Symbols() const {return m_symbols;}
    
protected :


    //! Calculates U and V for particular value normalised to 1000
    void GetPowerUV(int, int &, int &);
    
    //! Draws power bar legend with given limits
    void DrawPowerBar(int, int);
    
    //! Draws value
    void DrawValue(int, int, int);
    
    //! Colours motion vector block referenced by motion vector
    void DrawMvBlockUV(int, int, int, int);
    
    //! Colours an 8x8 block referenced by TL chroma pixel
    void DrawBlockUV(int, int, int, int);
    
    //! Frame data
    Frame & m_frame;
    
    //! Block parameters and chroma scaling
    DrawFrameMotionParams & m_draw_params;
    
    //! Symbols
    OverlaySymbols m_symbols;
    
private :

};

#endif
