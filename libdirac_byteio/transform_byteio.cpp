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
* Contributor(s): Anuradha Suraparaju (Original Author)
*                 Andrew Kennedy
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

#include <libdirac_byteio/transform_byteio.h>

using namespace dirac;

TransformByteIO::TransformByteIO(FrameParams& fparams,
                                 CodecParams& cparams):
ByteIO(),
m_fparams(fparams),
m_cparams(cparams),
m_default_cparams(cparams.GetVideoFormat(), fparams.GetFrameType(), true)
{
}

TransformByteIO::TransformByteIO(ByteIO &byte_io,
                                 FrameParams& fparams,
                                 CodecParams& cparams):
ByteIO(byte_io),
m_fparams(fparams),
m_cparams(cparams),
m_default_cparams(cparams.GetVideoFormat(), fparams.GetFrameType(), true)
{
}

TransformByteIO::~TransformByteIO()
{
    for(size_t index=0; index < m_component_list.size(); ++index)
       delete m_component_list[index];
}

void TransformByteIO::CollateByteStats(DiracByteStats& dirac_byte_stats)
{
    // set number of component bytes
    for(size_t index=0; index < m_component_list.size(); ++index)
        m_component_list[index]->CollateByteStats(dirac_byte_stats);
}

int TransformByteIO::GetSize() const
{
    //std::cerr << "Transform Size=" << ByteIO::GetSize() << std::endl;
    int size=0;
    for(size_t index=0; index < m_component_list.size(); ++index)
        size += m_component_list[index]->GetSize();
    return ByteIO::GetSize()+size;
}

const std::string TransformByteIO::GetBytes()
{
    std::string bytes;
    for(size_t index=0; index < m_component_list.size(); ++index)
        bytes += m_component_list[index]->GetBytes();
    return ByteIO::GetBytes()+bytes;
}

void TransformByteIO::Output()
{
    // FIXME - outputting all default parameters at the moment
    // Zero Transform flag
    OutputBit(false);
    // Non-default Wavelet flag
    OutputBit(false);
    
    // Non-default Depth flag
    if (m_cparams.TransformDepth() == m_default_cparams.TransformDepth())
    {
        OutputBit(false);
    }
    else
    {
        OutputBit(true);
        OutputVarLengthUint(m_cparams.TransformDepth());
    }
    
    // Spatial Partition flag
    OutputBit(m_cparams.SpatialPartition());
    if (m_cparams.SpatialPartition())
    {
        // non-default partition flag
        OutputBit(!m_cparams.DefaultSpatialPartition());
        if (!m_cparams.DefaultSpatialPartition())
        {
            // non-default partitoning
            OutputVarLengthUint(m_cparams.MaxXBlocks());
            OutputVarLengthUint(m_cparams.MaxYBlocks());
        }
        // Multiple quantisers flag
        OutputBit(m_cparams.MultiQuants());
    // Flush output for bend alignment
    }
    ByteAlignOutput();
}

void TransformByteIO::Input()
{
    // Byte Alignment
    ByteAlignInput();

    // Zero transform flag
    m_cparams.SetZeroTransform(InputBit());
    if (m_cparams.ZeroTransform())
        return;

    // Transform filter
    if (InputBit())
        m_cparams.SetTransformFilter(static_cast<WltFilter>(InputVarLengthUint()));
    else
        m_cparams.SetTransformFilter(m_default_cparams.TransformFilter());

    // transform depth
    if (InputBit())
         m_cparams.SetTransformDepth(InputVarLengthUint());
    else
        m_cparams.SetTransformDepth(m_default_cparams.TransformDepth());

    // Spatial partition flag
    m_cparams.SetSpatialPartition(InputBit());

    if (m_cparams.SpatialPartition())
    {
        // Is default spatial partitioning being used
        m_cparams.SetDefaultSpatialPartition(!InputBit());
        if (!m_cparams.DefaultSpatialPartition())
        {
            // if not using default spatial partitioning
            // max horiz coeff blocks
            m_cparams.SetMaxXBlocks(InputVarLengthUint());
            // max vertical coeff blocks
            m_cparams.SetMaxYBlocks(InputVarLengthUint());
        }
        // Multiple quantisers flag
        m_cparams.SetMultiQuants(InputBit());
    }
    // Byte Alignment
    ByteAlignInput();
}

void TransformByteIO::AddComponent(ComponentByteIO* component_byteio)
{
   m_component_list.push_back(component_byteio);
}


//-------------private---------------------------------------------------------------
