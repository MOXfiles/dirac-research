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

#include <util/instrumentation/libdirac_instrument/motion_colour_arrows.h>

// constructor
DrawMotionColourArrows::DrawMotionColourArrows(Frame & frame, DrawFrameMotionParams & draw_params,
                                               const MvArray & mv, int mv_scale, int mv_clip)
:
    DrawMotionArrows(frame, draw_params, mv, mv_scale),
    m_mv_clip(mv_clip)
{}

// destructor
DrawMotionColourArrows::~DrawMotionColourArrows()
{}

// manages call to DrawMotionArrows::DrawArrow() and colours motion vector blocks
void DrawMotionColourArrows::DrawBlock(int j, int i)
{
    // if motion vectors are divisible by 4 and are smaller than 16 pixels
    if ( (m_draw_params.MvYBlockY() % 4 == 0) && (m_draw_params.MvYBlockY() <= 16) )
    {
        m_blocks_per_arrow = 16 / m_draw_params.MvYBlockY();

        if ( (j==0 || (j % m_blocks_per_arrow)==0 ) && ( (i==0 || (i % m_blocks_per_arrow)==0 ) ) )
        {
            DrawArrow(j, i, j, i);

            // find average motion vector for block group
            int x_sum = 0, y_sum = 0;

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
            double power = (1000 / m_mv_clip) * std::sqrt((x_avg*x_avg)+(y_avg*y_avg));
            
            int U = 0, V = 0;
            GetPowerUV((int)power, U, V);

            for (int y=j; y<j+m_blocks_per_arrow; ++y)
            {
                for (int x=i; x<i+m_blocks_per_arrow; ++x)
                {
                    DrawMvBlockUV(y, x, U+512, V+512);
                }
            }

        }   
    }
    // if motion vectors are divisible by 16 and are larger than 16 (unlikely!)
    else if ( (m_draw_params.MvYBlockY() % 16 == 0) && (m_draw_params.MvYBlockY() > 16) )
    {
        m_arrows_per_block = m_draw_params.MvYBlockY() / 16;

        for (int y=j; y<j+m_arrows_per_block; ++y)
        {
            for (int x=i; x<i+m_arrows_per_block; ++x)
            {
                DrawArrow(y, x, y, x);

                int U = 0, V = 0;
                double power = (1000 / m_mv_clip) * std::sqrt(((double)m_mv[y][x].x*(double)m_mv[y][x].x) +
                                                              ((double)m_mv[y][x].y*(double)m_mv[y][x].y));
                GetPowerUV((int)power, U, V);
                DrawMvBlockUV(y, x, U+512, V+512);
            }
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
        {
            DrawArrow(j, i, j+offset, i+offset);

            // find average motion vector for block group
            int x_sum = 0, y_sum = 0;

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
            double power = (1000 / m_mv_clip) * std::sqrt((x_avg*x_avg)+(y_avg*y_avg));

            int U = 0, V = 0;
            GetPowerUV((int)power, U, V);

            for (int y=j; y<j+m_blocks_per_arrow; ++y)
            {
                for (int x=i; x<i+m_blocks_per_arrow; ++x)
                {
                    DrawMvBlockUV(y, x, U+512, V+512);
                }
            }
        }
    }


}

// draws power bar colour legend
void DrawMotionColourArrows::DrawLegend()
{
    DrawPowerBar(0, m_mv_clip);
}
