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

#include <util/instrumentation/libdirac_instrument/motion_colour.h>

// constructor
DrawMotionColour::DrawMotionColour(Frame & frame, DrawFrameMotionParams & draw_params, const MvArray & mv,
                                   int mv_scale, int mv_clip)
:
    DrawOverlay(frame, draw_params),
    m_mv_scale(mv_scale),
    m_mv_clip(mv_clip),
    m_mv(mv)
{}

// destructor
DrawMotionColour::~DrawMotionColour()
{}

// colours motion vector block appropriately
void DrawMotionColour::DrawBlock(int j, int i)
{
    DrawMvBlockUV(j, i, (((m_mv[j][i].x) * (500/m_mv_clip)) / m_mv_scale) + 500,
                        (((m_mv[j][i].y) * (500/m_mv_clip)) / m_mv_scale) + 500);
}

// draws colour wheel legend
void DrawMotionColour::DrawLegend()
{
    int y_start = m_frame.Ydata().LastY()-30;
    
    for (int ypx=y_start+1, y=-15; ypx<=y_start+30; ++ypx, y+=m_draw_params.ChromaFactorY())
    {
        // white background
        for (int xpx=1; xpx<40; ++xpx)
        {
            m_frame.Ydata()[ypx][xpx]=500;
        }

        // crosshair vertical line
        m_frame.Ydata()[ypx][21]=350;
    }

    // colour in the rectangle
    for (int ypx=m_frame.Udata().LastY(), y=15; ypx>m_frame.Udata().LastY()-(30/m_draw_params.ChromaFactorY());
         --ypx, y-=m_draw_params.ChromaFactorY())
    {
        for (int xpx=40/m_draw_params.ChromaFactorX(), x=20; xpx>=0; --xpx, x-=m_draw_params.ChromaFactorX())
        {
            m_frame.Udata()[ypx][xpx]=(x*25)+500;
            m_frame.Vdata()[ypx][xpx]=(y*25)+500;
        }
    }

    // crosshair horizontal linem_
    for (int xpx=0; xpx<40; ++xpx)
    {
        m_frame.Ydata()[y_start+16][xpx]=350;
    }

    // vertical black line
    for (int ypx=y_start+1; ypx<=y_start+30; ++ypx)
    {
        m_frame.Ydata()[ypx][41]=0;
        m_frame.Ydata()[ypx][0]=0;
    }

    // horizontal black line
    for (int xpx=0; xpx<=41; ++xpx)
    {
        m_frame.Ydata()[y_start][xpx]=0;
        m_frame.Ydata()[m_frame.Ydata().LastY()][xpx]=0;
    }

    // display the clip value
    DrawValue(m_mv_clip, m_frame.Ydata().LastY()-46, 0);
}
