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

#include <util/instrumentation/libdirac_instrument/motion_arrows.h>

// constructor
DrawMotionArrows::DrawMotionArrows(Frame & frame, DrawFrameMotionParams & draw_params,
                                   const MvArray & mv, int mv_scale)
:
    DrawOverlay(frame, draw_params),
    m_mv_scale(mv_scale),
    m_blocks_per_arrow(0),
    m_arrows_per_block(0),
    m_mv(mv)
{}

// destructor
DrawMotionArrows::~DrawMotionArrows()
{}

// manages drawing of arrows, dependent on size of motion vector block
void DrawMotionArrows::DrawBlock(int j, int i)
{
    // no chroma in frame
    for (int y=j*m_draw_params.MvUVBlockY(); y<(j+1)*m_draw_params.MvUVBlockY(); ++y)
    {
        for (int x=i*m_draw_params.MvUVBlockX(); x<(i+1)*m_draw_params.MvUVBlockX(); ++x)
        {
            m_frame.Udata()[y][x] = 512;
            m_frame.Vdata()[y][x] = 512;
        }
    }

    // if motion vectors are divisible by 4 and are smaller than 16 pixels
    if ( (m_draw_params.MvYBlockY() % 4 == 0) && (m_draw_params.MvYBlockY() <= 16) )
    {
        m_blocks_per_arrow = 16 / m_draw_params.MvYBlockY();

        if ( (j==0 || (j % m_blocks_per_arrow)==0 ) && ( (i==0 || (i % m_blocks_per_arrow)==0 ) ) )
            DrawArrow(j, i, j, i);
    }
    // if motion vectors are divisible by 16 and are larger than 16 (unlikely!)
    else if ( (m_draw_params.MvYBlockY() % 16 == 0) && (m_draw_params.MvYBlockY() > 16) )
    {
        m_arrows_per_block = m_draw_params.MvYBlockY() / 16;

        for (int y=j; y<j+m_arrows_per_block; ++y)
        {
            for (int x=i; x<i+m_arrows_per_block; ++x)
                DrawArrow(y, x, y, x);
        }
    }
    // if motion vectors do not divide into a single arrow (or vice versa)
    else
    {
        int m_blocks_per_arrow = 0;
        while (m_blocks_per_arrow <= 16)
            m_blocks_per_arrow += m_draw_params.MvYBlockY();

        // calculate offset for TL corner of arrow
        int block_group_size = m_blocks_per_arrow * m_draw_params.MvYBlockY();
        int offset = int( (block_group_size - 16) / 2 );

        if ( (j==0 || (j % m_blocks_per_arrow)==0 ) && ( (i==0 || (i % m_blocks_per_arrow)==0 ) ) )
            DrawArrow(j, i, j+offset, i+offset);
    }
}

// draws a single 16 x 16 pixel arrow, taking the mean of motion vectors
void DrawMotionArrows::DrawArrow(int j, int i, int y_pos, int x_pos)
{
    // find average motion vector for block group
    int x_sum = 0, y_sum = 0;

    // loop over motion vector group
    for (int y=j; y<j+m_blocks_per_arrow; ++y)
    {
        for (int x=i; x<i+m_blocks_per_arrow; ++x)
        {
            x_sum += m_mv[y][x].x;
            y_sum -= m_mv[y][x].y;
        }
    }

    double x_avg = x_sum / (m_blocks_per_arrow * m_blocks_per_arrow * m_mv_scale);
    double y_avg = y_sum / (m_blocks_per_arrow * m_blocks_per_arrow * m_mv_scale);

    // get absolute angle
    double angle = std::atan(std::abs(x_avg) / std::abs(y_avg)) * (360 / 6.82);

    // arrow arrays representing angles 0 ~ 90 degrees are stored, therefore need to flip
    // them around if the angle is negative
    if (angle > -3.75 && angle <= 3.75)
        m_symbols.Arrow(m_symbols.Arrow0());

    else if ((angle > 3.75 && angle <= 11.25) || (angle < -3.75 && angle >= -11.25))
        m_symbols.Arrow(m_symbols.Arrow7_5());

    else if ((angle > 11.25 && angle <= 18.75) || (angle < -11.25 && angle >= -18.75))
        m_symbols.Arrow(m_symbols.Arrow15());

    else if ((angle > 18.75 && angle <= 26.25) || (angle < -18.75 && angle >= -26.25))
        m_symbols.Arrow(m_symbols.Arrow22_5());

    else if ((angle > 26.25 && angle <= 33.75) || (angle < -26.25 && angle >= -33.75))
        m_symbols.Arrow(m_symbols.Arrow30());

    else if ((angle > 33.75 && angle <= 41.25) || (angle < -33.75 && angle >= -41.25))
        m_symbols.Arrow(m_symbols.Arrow37_5());

    else if ((angle > 41.25 && angle <= 48.75) || (angle < -41.25 && angle >= -48.75))
        m_symbols.Arrow(m_symbols.Arrow45());

    else if ((angle > 48.75 && angle <= 56.25) || (angle < -48.75 && angle >= -56.25))
        m_symbols.Arrow(m_symbols.Arrow52_5());

    else if ((angle > 56.25 && angle <= 63.75) || (angle < -56.25 && angle >= -63.75))
        m_symbols.Arrow(m_symbols.Arrow60());

    else if ((angle > 63.75 && angle <= 71.25) || (angle < -63.75 && angle >= -71.25))
        m_symbols.Arrow(m_symbols.Arrow67_5());

    else if ((angle > 71.25 && angle <= 78.75) || (angle < -71.25 && angle >= -78.75))
        m_symbols.Arrow(m_symbols.Arrow75());

    else if ((angle > 78.75 && angle <= 86.25) || (angle < -78.75 && angle >= -86.25))
        m_symbols.Arrow(m_symbols.Arrow82_5());

    else if ((angle > 86.25 && angle <= 90) || (angle < -86.25 && angle >= -90))
        m_symbols.Arrow(m_symbols.Arrow90());

    bool flipH = false, flipV = false;

    // check sign
    if (x_avg < 0)
        flipH=true;

    if (y_avg < 0)
        flipV=true;

    // no motion
    if (y_avg == 0 && x_avg == 0)
        m_symbols.Arrow(m_symbols.ArrowNull());

    // special case if angle is exactly 0 or 90
    else if (y_avg == 0)
        m_symbols.Arrow(m_symbols.Arrow90());

    else if (x_avg == 0)
        m_symbols.Arrow(m_symbols.Arrow0());

    if (flipH && !flipV)
    {
        for (int ypx=0; ypx<16; ++ypx)
        {
            for (int xpx=0; xpx<16; ++xpx)
            {
                m_frame.Ydata()[(8*y_pos)+ypx][(8*x_pos) + xpx] += m_symbols.Arrow()[ypx][15-xpx] * 1024;
            }// xpx
        }// ypx
    }
    else if (!flipH && flipV)
    {
        for (int ypx=0; ypx<16; ++ypx)
        {
            for (int xpx=0; xpx<16; ++xpx)
            {
                m_frame.Ydata()[(8*y_pos) + ypx][(8*x_pos) + xpx] += m_symbols.Arrow()[15-ypx][xpx] * 1024;
            }// xpx
        }// ypx
    }
    else if (flipH && flipV)
    {
        for (int ypx=0; ypx<16; ++ypx)
        {
            for (int xpx=0; xpx<16; ++xpx)
            {
                m_frame.Ydata()[(8*y_pos) + ypx][(8*x_pos) + xpx] += m_symbols.Arrow()[15-ypx][15-xpx] * 1024;
            }// xpx
        }// ypx
    }
    else
    {
        for (int ypx=0; ypx<16; ++ypx)
        {
            for (int xpx=0; xpx<16; ++xpx)
            {
                m_frame.Ydata()[(8*y_pos) + ypx][(8*x_pos) + xpx] += m_symbols.Arrow()[ypx][xpx] * 1024;
            }// xpx
        }// ypx
    }
}

// no appropriate legend for overlay
void DrawMotionArrows::DrawLegend()
{}