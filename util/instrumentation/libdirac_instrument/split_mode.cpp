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

#include <util/instrumentation/libdirac_instrument/split_mode.h>

// constructor
DrawSplitMode::DrawSplitMode(Frame & frame, DrawFrameMotionParams & draw_params, const TwoDArray<int> & mode)
:
    DrawOverlay(frame, draw_params),
    m_mode(mode)
{}

// destructor
DrawSplitMode::~DrawSplitMode()
{}

// colours a motion vector block according to split mode
void DrawSplitMode::DrawBlock(int j, int i)
{
    int power = 0, U = 0, V = 0;
    // get split level
    if (m_mode[int(j/4)][int(i/4)] == 0)
        power=0;
    else if (m_mode[int(j/4)][int(i/4)] == 1)
        power=400;
    else if (m_mode[int(j/4)][int(i/4)] == 2)
        power=800;

    GetPowerUV(power, U, V);
    DrawMvBlockUV(j, i, U+500, V+500);

}

// displays colours representing splitting mode
void DrawSplitMode::DrawLegend()
{
    // blank background
    for (int ypx=m_frame.Ydata().LastY()-48; ypx<=m_frame.Ydata().LastY(); ++ypx)
    {
        for (int xpx=7; xpx>=0; --xpx)
            m_frame.Ydata()[ypx][xpx]=500; // grey
    }

    int U=0, V=0;

    GetPowerUV(800, U, V); // mode 2
    DrawBlockUV(m_frame.Udata().LastY()-(48/m_draw_params.ChromaFactorY())+1, 0, U+500, V+500);
    DrawBlockUV(m_frame.Udata().LastY()-(40/m_draw_params.ChromaFactorY())+1, 0, U+500, V+500);
    
    GetPowerUV(400, U, V); // mode 1
    DrawBlockUV(m_frame.Udata().LastY()-(32/m_draw_params.ChromaFactorY())+1, 0, U+500, V+500);
    DrawBlockUV(m_frame.Udata().LastY()-(24/m_draw_params.ChromaFactorY())+1, 0, U+500, V+500);
    
    GetPowerUV(0, U, V); // mode 0
    DrawBlockUV(m_frame.Udata().LastY()-(16/m_draw_params.ChromaFactorY())+1, 0, U+500, V+500);
    DrawBlockUV(m_frame.Udata().LastY()-(8/m_draw_params.ChromaFactorY())+1, 0, U+500, V+500);

    // black horizontal lines
    for (int xpx=15; xpx>=0; --xpx)
    {
        m_frame.Ydata()[m_frame.Ydata().LastY()-48][xpx]=0;
        m_frame.Ydata()[m_frame.Ydata().LastY()-32][xpx]=0;
        m_frame.Ydata()[m_frame.Ydata().LastY()-16][xpx]=0;
    }

    // draw '2 1 0' label
    for (int digit=m_frame.Ydata().LastY()-47; digit<m_frame.Ydata().LastY(); digit+=16)
    {
        if (digit==m_frame.Ydata().LastY()-47)
            DrawCharacter(m_symbols.Number2(), digit, 8);
        else if (digit==m_frame.Ydata().LastY()-31)
            DrawCharacter(m_symbols.Number1(), digit, 8);
        else if (digit==m_frame.Ydata().LastY()-15)
            DrawCharacter(m_symbols.Number0(), digit, 8);
    }

}

