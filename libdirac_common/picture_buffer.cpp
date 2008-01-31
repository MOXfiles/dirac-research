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
* Contributor(s): Thomas Davies (Original Author),
*                 Scott R Ladd,
*                 Anuradha Suraparaju
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

#include <libdirac_common/picture_buffer.h>
#include <algorithm>
using namespace dirac;

//Simple constructor for decoder operation
PictureBuffer::PictureBuffer() :
    m_ref_count(0),
    m_num_L1(0),
    m_L1_sep(1),
    m_gop_len(0),
    m_interlace(false),
    m_using_ac(false)
{}

//Simple constructor for general operation
PictureBuffer::PictureBuffer(ChromaFormat cf,
                         const int orig_xlen,
                         const int orig_ylen,
                         const int dwt_xlen,
                         const int dwt_ylen,
                         const int dwt_cxlen,
                         const int dwt_cylen,
                         unsigned int luma_depth,
                         unsigned int chroma_depth,
                         bool using_ac) :
    m_ref_count(0),
    m_fparams(cf, orig_xlen, orig_ylen, dwt_xlen, dwt_ylen, dwt_cxlen, dwt_cylen, luma_depth, chroma_depth),
    m_num_L1(0),
    m_L1_sep(1),
    m_gop_len(0),
    m_interlace(false),
    m_using_ac(using_ac)
{}

//Constructor setting GOP parameters for use with a standard GOP
PictureBuffer::PictureBuffer(ChromaFormat cf,
                         const int numL1,
                         const int L1sep,
                         const int orig_xlen,
                         const int orig_ylen,
                         const int dwt_xlen,
                         const int dwt_ylen,
                         const int dwt_cxlen,
                         const int dwt_cylen,
                         unsigned int luma_depth,
                         unsigned int chroma_depth,
                         bool interlace,
                         bool using_ac) :
    m_ref_count(0),
    m_fparams(cf,orig_xlen, orig_ylen, dwt_xlen, dwt_ylen, dwt_cxlen, dwt_cylen, luma_depth, chroma_depth),
    m_num_L1(numL1),
    m_L1_sep(L1sep),
    m_interlace(interlace),
    m_using_ac(using_ac)
{
    if (m_num_L1>0)
    {// conventional GOP coding
        m_gop_len = (m_num_L1+1)*m_L1_sep;
    }
    else if (m_num_L1==0)
    {// I-picture only coding
        m_gop_len = 1;
        m_L1_sep = 0;
    }
    else
    {// don't have a proper GOP, only an initial I-picture
        m_gop_len = 0;
    }
}

//Copy constructor. Why anyone would need this I don't know.
PictureBuffer::PictureBuffer(const PictureBuffer& cpy)
    {
    // first delete all frames in the current buffer
    for (size_t i=0 ; i<m_frame_data.size() ; ++i)
    {
        delete m_frame_data[i];
    }//i

    // next create new arrays, copying from the initialising buffer
    m_frame_data.resize(cpy.m_frame_data.size());
    m_frame_in_use.resize(cpy.m_frame_in_use.size());
    for (size_t i=0 ; i<m_frame_data.size() ; ++i){
        m_frame_data[i] = new Picture( *(cpy.m_frame_data[i]) );
        m_frame_in_use[i] = cpy.m_frame_in_use[i];
    }//i

    // now copy the map
    m_fnum_map = cpy.m_fnum_map;

    // and the internal picture parameters
    m_fparams = cpy.m_fparams;

    // and the reference count
    m_ref_count = cpy.m_ref_count;

    // and the gop structure
    m_num_L1 = cpy.m_num_L1;
    m_L1_sep = cpy.m_L1_sep;
    m_gop_len = cpy.m_gop_len;
    m_interlace = cpy.m_interlace;
    m_using_ac = cpy.m_using_ac;
}

//Assignment=. Not sure why this would be used either.
PictureBuffer& PictureBuffer::operator=(const PictureBuffer& rhs){
    if (&rhs!=this)
    {
        // delete all the frames in the lhs buffer
        for (size_t i=0 ; i<m_frame_data.size() ; ++i)
        {
            delete m_frame_data[i];
        }//i

        // next create new arrays, copying from the rhs
        m_frame_data.resize(rhs.m_frame_data.size());
        m_frame_in_use.resize(rhs.m_frame_in_use.size());
        for (size_t i=0 ; i<m_frame_data.size() ; ++i)
        {
            m_frame_data[i] = new Picture( *(rhs.m_frame_data[i]) );
            m_frame_in_use[i] = rhs.m_frame_in_use[i];
        }//i

        // now copy the map
        m_fnum_map = rhs.m_fnum_map;

        // and the internal picture parameters
        m_fparams = rhs.m_fparams;

        // and the reference count
        m_ref_count = rhs.m_ref_count;

        // and the interlace flag
        m_interlace = rhs.m_interlace;
    
        // and the arithmetic flag
        m_using_ac = rhs.m_using_ac;
    }
    return *this;
}

//Destructor
PictureBuffer::~PictureBuffer()
{
    for (size_t i=0 ; i<m_frame_data.size() ;++i)
        delete m_frame_data[i];
}

Picture& PictureBuffer::GetPicture( const unsigned int fnum )
{//get picture with a given picture number, NOT with a given position in the buffer.
 //If the picture number does not occur, the first picture in the buffer is returned.

    std::map<unsigned int,unsigned int>::iterator it = m_fnum_map.find(fnum);

    unsigned int pos = 0;
    if (it != m_fnum_map.end())
        pos = it->second;

    return *(m_frame_data[pos]);
}

const Picture& PictureBuffer::GetPicture( const unsigned int fnum ) const
{    //as above, but const version

    std::map<unsigned int,unsigned int>::const_iterator it = m_fnum_map.find(fnum);

    unsigned int pos=0;
    if (it != m_fnum_map.end())
        pos = it->second;

    return *(m_frame_data[pos]);
}

Picture& PictureBuffer::GetPicture( const unsigned int fnum, bool& is_present )
{//get picture with a given picture number, NOT with a given position in the buffer.
 //If the picture number does not occur, the first picture in the buffer is returned.

    std::map<unsigned int,unsigned int>::iterator it = m_fnum_map.find(fnum);

    unsigned int pos = 0;
    if (it != m_fnum_map.end())
    {
        is_present = true;
        pos = it->second;
    }
    else
        is_present=false;

    return *(m_frame_data[pos]);
}

const Picture& PictureBuffer::GetPicture( const unsigned int fnum, bool& is_present ) const
{    //as above, but const version

    std::map<unsigned int,unsigned int>::const_iterator it = m_fnum_map.find(fnum);

    unsigned int pos=0;
    if (it != m_fnum_map.end())
    {
        is_present = true;
        pos = it->second;
    }
    else
        is_present=false;

    return *(m_frame_data[pos]);
}

bool PictureBuffer::IsPictureAvail( const unsigned int fnum ) const
{

    std::map<unsigned int,unsigned int>::const_iterator it = m_fnum_map.find(fnum);

    if (it != m_fnum_map.end())
        return true;
    else
        return false;
}

PicArray& PictureBuffer::GetComponent( const unsigned int fnum , CompSort c)
{//as GetPicture, but returns corresponding component

    std::map<unsigned int,unsigned int>::iterator it = m_fnum_map.find(fnum);

    unsigned int pos = 0;
    if (it!=m_fnum_map.end())
        pos = it->second;

    if (c == U_COMP)
        return m_frame_data[pos]->Udata();
    else if (c == V_COMP)
        return m_frame_data[pos]->Vdata();
    else
        return m_frame_data[pos]->Ydata();
}

const PicArray& PictureBuffer::GetComponent( const unsigned int fnum , CompSort c ) const
{//as above, but const version

    std::map<unsigned int,unsigned int>::const_iterator it = m_fnum_map.find(fnum);

    // FIXME - how do we handle the condition when a picture matching fnum is
    // not found???
    unsigned int pos = 0;

    if (it!=m_fnum_map.end())
        pos = it->second;

    if (c==U_COMP)
        return m_frame_data[pos]->Udata();
    else if (c==V_COMP)
        return m_frame_data[pos]->Vdata();
    else
        return m_frame_data[pos]->Ydata();
}

// as GetPicture, but returns corresponding upconverted component
PicArray& PictureBuffer::GetUpComponent(const unsigned int fnum, CompSort c){
    std::map<unsigned int,unsigned int>::iterator it = m_fnum_map.find(fnum);

    unsigned int pos = 0;
    if (it!=m_fnum_map.end())
        pos = it->second;

    if (c == U_COMP)
        return m_frame_data[pos]->UpUdata();
    else if (c == V_COMP)
        return m_frame_data[pos]->UpVdata();
    else
        return m_frame_data[pos]->UpYdata();

}

const PicArray& PictureBuffer::GetUpComponent(const unsigned int fnum, CompSort c) const {//as above, but const version
    std::map<unsigned int,unsigned int>::const_iterator it=m_fnum_map.find(fnum);

    unsigned int pos = 0;
    if (it!=m_fnum_map.end())
        pos = it->second;

    if (c == U_COMP)
        return m_frame_data[pos]->UpUdata();
    else if (c == V_COMP)
        return m_frame_data[pos]->UpVdata();
    else
        return m_frame_data[pos]->UpYdata();

}

std::vector<int> PictureBuffer::Members() const
{
    std::vector<int> members( 0 );
    for (unsigned int i=0; i<m_frame_data.size(); ++i )
    {
        if ( m_frame_in_use[i] == true )
        {
            const PictureParams& fparams = m_frame_data[i]->GetPparams();
            members.push_back( fparams.PictureNum() );
        }
    }// i

    return members;
}


void PictureBuffer::PushPicture(const unsigned int frame_num)
{// Put a new picture onto the top of the stack using built-in picture parameters
 // with picture number frame_num


    // if picture is present - return
    if (IsPictureAvail(frame_num))
        return;

    m_fparams.SetPictureNum(frame_num);
    if ( m_fparams.PicSort().IsRef() )
        m_ref_count++;

    int new_frame_pos = -1;
    // First check if an unused picture is available in the buffer
    for (int i = 0; i < (int)m_frame_in_use.size(); ++i)
    {
        if (m_frame_in_use[i] == false)
        {
            new_frame_pos = i;
            m_frame_data[i]->ReconfigFrame(m_fparams);
            m_frame_in_use[i] = true;
            break;
        }
    }
    if (new_frame_pos == -1)
    {
        // No unused frames in buffer. Allocate a new picture
        Picture* fptr = new Picture(m_fparams);
        // add the picture to the buffer
        m_frame_data.push_back(fptr);
        m_frame_in_use.push_back(true);
        new_frame_pos = m_frame_data.size()-1;
    }

    // put the picture number into the index table
    std::pair<unsigned int,unsigned int> temp_pair(m_fparams.PictureNum() , new_frame_pos);
    m_fnum_map.insert(temp_pair);
}

void PictureBuffer::PushPicture( const PictureParams& fp )
{// Put a new picture onto the top of the stack

    // if picture is present - return
    if (IsPictureAvail(fp.PictureNum()))
        return;

    if ( fp.PicSort().IsRef() )
        m_ref_count++;

    int new_frame_pos = -1;
    // First check if an unused picture is available in the buffer
    for (int i = 0; i < (int)m_frame_in_use.size(); ++i)
    {
        if (m_frame_in_use[i] == false)
        {
            new_frame_pos = i;
            m_frame_data[i]->ReconfigFrame(fp);
            m_frame_in_use[i] = true;
            break;
        }
    }
    if (new_frame_pos == -1)
    {
        // No unused frames in buffer. Allocate a new picture
        Picture* fptr = new Picture(fp);
        // add the picture to the buffer
        m_frame_data.push_back(fptr);
        new_frame_pos = m_frame_data.size()-1;
        m_frame_in_use.push_back(true);
    }
    // put the picture number into the index table
    std::pair<unsigned int,unsigned int> temp_pair(fp.PictureNum() , new_frame_pos);
    m_fnum_map.insert(temp_pair);
}

void PictureBuffer::PushPicture( const Picture& picture )
{
    int fnum = picture.GetPparams().PictureNum();
    SetPictureParams( fnum );
    PushPicture(fnum);

    bool is_present;

    Picture &f = GetPicture(picture.GetPparams().PictureNum(), is_present);
    if(is_present)
        picture.CopyContents(f);
}

void PictureBuffer::Remove(const unsigned int pos)
{//remove picture fnum from the buffer, shifting everything above down

    const PictureParams& fparams = m_frame_data[pos]->GetPparams();

    if ( m_frame_in_use[pos] == true && fparams.PicSort().IsRef() )
        m_ref_count--;

    std::pair<unsigned int,unsigned int>* tmp_pair;

    if (pos<m_frame_data.size())
    {
        //flag that picture is no longer in use
        m_frame_in_use[pos] = false;

         //make a new map
        m_fnum_map.clear();
        for (size_t i=0 ; i<m_frame_data.size() ; ++i)
        {
            if (m_frame_in_use[i] == true)
            {
                tmp_pair = new std::pair<unsigned int,unsigned int>( m_frame_data[i]->GetPparams().PictureNum() , i);
                m_fnum_map.insert(*tmp_pair);
                delete tmp_pair;
            }
        }//i
    }
}


void PictureBuffer::SetRetiredPictureNum(const int show_fnum, const int current_coded_fnum)
{
    if ( IsPictureAvail(current_coded_fnum))
    {
        PictureParams &fparams = GetPicture(current_coded_fnum).GetPparams();
        fparams.SetRetiredPictureNum(-1);
        for (size_t i=0 ; i<m_frame_data.size() ; ++i)
        {
            if (m_frame_in_use[i] == true && (m_frame_data[i]->GetPparams().PictureNum() + m_frame_data[i]->GetPparams().ExpiryTime() ) <= show_fnum)
            {
                // Only _reference_ frames can be retired - the
                // decoder will retire non-reference frames as they are displayed
                if (m_frame_data[i]->GetPparams().PicSort().IsRef() )
                {
                    fparams.SetRetiredPictureNum(m_frame_data[i]->GetPparams().PictureNum()); 
                    break;
                }
            }
        }//i
    }
}
void PictureBuffer::CleanAll(const int show_fnum, const int current_coded_fnum)
{// clean out all frames that have expired
    if (IsPictureAvail(current_coded_fnum))
    {
        for (size_t i=0 ; i<m_frame_data.size() ; ++i)
        {
            if (m_frame_in_use[i] == true && (m_frame_data[i]->GetPparams().PictureNum() + m_frame_data[i]->GetPparams().ExpiryTime() ) <= show_fnum)
                Remove(i);
        }//i
    }
}

void PictureBuffer::CleanRetired(const int show_fnum, const int current_coded_fnum)
{// clean out all frames that have expired
    if ( IsPictureAvail(current_coded_fnum) )
    {
        PictureParams &fparams = GetPicture(current_coded_fnum).GetPparams();
        // Remove Reference picture specified in retired picture number.
        if (fparams.PicSort().IsRef() && fparams.RetiredPictureNum()>= 0)
            Clean(fparams.RetiredPictureNum());
        fparams.SetRetiredPictureNum(-1);
        // Remove non-reference frames that have expired
        for (size_t i=0 ; i<m_frame_data.size() ; ++i)
        {
            if (m_frame_in_use[i] == true && (m_frame_data[i]->GetPparams().PictureNum() + m_frame_data[i]->GetPparams().ExpiryTime() ) <= show_fnum && m_frame_data[i]->GetPparams().PicSort().IsNonRef())
                Remove(i);
        }//i
    }
}

void PictureBuffer::Clean(const int fnum)
{// clean out all frames that have expired
    for (size_t i=0 ; i<m_frame_data.size() ; ++i)
    {
        if (m_frame_in_use[i] == true && m_frame_data[i]->GetPparams().PictureNum() == fnum)
        {
            Remove(i);
        }
    }//i
}

void PictureBuffer::SetPictureParams( const unsigned int fnum )
{
    m_fparams.SetUsingAC( m_using_ac);
    if (!m_interlace)
        SetProgressiveFrameParams(fnum);
    else
        SetInterlacedFrameParams(fnum);
}

void PictureBuffer::SetProgressiveFrameParams( const unsigned int fnum )
{
    // Set the picture parameters, given the GOP set-up and the picture number in display order
    // This function can be ignored by setting the picture parameters directly if required

    m_fparams.SetPictureNum( fnum );
    m_fparams.SetRetiredPictureNum( -1 );
    m_fparams.Refs().clear();

    if ( m_gop_len>0 )
    {

        if ( fnum % m_gop_len == 0)
        {
            if (m_gop_len > 1)
                m_fparams.SetPicSort( PictureSort::IntraRefPictureSort());
            else // I-picture only coding
            {
                m_fparams.SetPicSort( PictureSort::IntraNonRefPictureSort());
            }    
            // I picture expires after we've coded the next I picture
            m_fparams.SetExpiryTime( m_gop_len );
        }
        else if (fnum % m_L1_sep == 0)
        {
            m_fparams.SetPicSort( PictureSort::InterRefPictureSort());

            // Ref the previous I or L1 picture
            m_fparams.Refs().push_back( fnum - m_L1_sep );

            // if we don't have the first L1 picture ...
            if ((fnum-m_L1_sep) % m_gop_len>0)
                // ... other ref is the prior I picture
                m_fparams.Refs().push_back( ( fnum/m_gop_len ) * m_gop_len  );

            // Expires after the next L1 or I picture
            m_fparams.SetExpiryTime( m_L1_sep );
        }
        else if ((fnum+1) % m_L1_sep == 0)
        {
            m_fparams.SetPicSort( PictureSort::InterNonRefPictureSort());

            // .. and the previous picture
            m_fparams.Refs().push_back(fnum-1);
            // Refs are the next I or L1 picture ...
            m_fparams.Refs().push_back(fnum+1);

            m_fparams.SetExpiryTime( 1 );
        }
        else
        {
            m_fparams.SetPicSort( PictureSort::InterRefPictureSort());

            // .. and the previous picture
            m_fparams.Refs().push_back(fnum-1);
            // Refs are the next I or L1 picture ...
            m_fparams.Refs().push_back(((fnum/m_L1_sep)+1)*m_L1_sep);

            m_fparams.SetExpiryTime( 1 );
        }

    }
    else{
        if (fnum==0)
        {
            m_fparams.SetPicSort( PictureSort::IntraRefPictureSort());

            m_fparams.SetExpiryTime( 1<<30 );//ie never
        }
        else if (fnum % m_L1_sep==0)
        {
            m_fparams.SetPicSort( PictureSort::InterRefPictureSort());

            m_fparams.Refs().push_back(0);//picture 0 is the I picture

            if (fnum != m_L1_sep)//we don't have the first L1 picture
                m_fparams.Refs().push_back(fnum-m_L1_sep);//other ref is the prior L1 picture

            //expires after the next L1 or I picture
            m_fparams.SetExpiryTime( m_L1_sep );
        }
        else
        {
            m_fparams.SetPicSort( PictureSort::InterNonRefPictureSort());

            m_fparams.Refs().push_back((fnum/m_L1_sep)*m_L1_sep);
            m_fparams.Refs().push_back(((fnum/m_L1_sep)+1)*m_L1_sep);
            m_fparams.SetExpiryTime( 1 );    //L2 frames could expire directly after being coded, but putting in a delay of 1
                                        //allows for picture-skipping to be done, since the picture will still be around to
                                        //be used if the next picture is skipped.
        }
    }
}

void PictureBuffer::SetInterlacedFrameParams( const unsigned int fnum )
{
    // Set the picture parameters, given the GOP set-up and the picture number in display order
    // This function can be ignored by setting the picture parameters directly if required

    m_fparams.SetPictureNum( fnum );
    m_fparams.SetRetiredPictureNum( -1 );
    m_fparams.Refs().clear();


    if ( m_gop_len>0 )
    {

        if ( (fnum/2) % m_gop_len == 0)
        {
            // Field 1 is Intra Field
            if (m_gop_len > 1)
            {
                m_fparams.SetPicSort( PictureSort::IntraRefPictureSort());
                // I picture expires after we've coded the next I picture
                m_fparams.SetExpiryTime( m_gop_len * 2);
                if (m_interlace && fnum%2)
                {
                    m_fparams.SetPicSort( PictureSort::InterRefPictureSort());
                    // Ref the previous I field
                    m_fparams.Refs().push_back( fnum-1 );
                }
            }
            else
            {
                // I-picture only coding
                m_fparams.SetPicSort( PictureSort::IntraNonRefPictureSort());
                m_fparams.SetExpiryTime( m_gop_len );
            }
        }
        else if ((fnum/2) % m_L1_sep == 0)
        {
            m_fparams.SetPicSort( PictureSort::InterRefPictureSort());

            if (fnum%2)
            {
                // Field 2
                // Ref the first field of same picture
                m_fparams.Refs().push_back( fnum - 1);
                // Ref the previous field 2 of I or L1 picture
                m_fparams.Refs().push_back( fnum - m_L1_sep*2 );
            }
            else
            {
                // Field 1
                // Ref the field 1 of previous I or L1 picture
                m_fparams.Refs().push_back( fnum - m_L1_sep*2 );
                // Ref the field 2 of previous I or L1 picture
                m_fparams.Refs().push_back( fnum - m_L1_sep*2 + 1 );
            }

            // Expires after the next L1 or I picture
            m_fparams.SetExpiryTime( (m_L1_sep+1)*2-1 );
        }
        else if ((fnum/2+1) % m_L1_sep == 0)
        {
            // Bi-directional non-reference fields.
            m_fparams.SetPicSort( PictureSort::InterNonRefPictureSort());

            // .. and the same parity field of the previous picture
            m_fparams.Refs().push_back(fnum-1*2);
            // Refs are the same parity fields in the next I or L1 picture ...
            m_fparams.Refs().push_back(fnum+1*2);

            m_fparams.SetExpiryTime( 1 );
        }
        else
        {
            // Bi-directional reference fields.
            m_fparams.SetPicSort( PictureSort::InterRefPictureSort());

            // .. and the same parity field of the previous picture
            m_fparams.Refs().push_back(fnum-1*2);
            // Refs are the same parity fields in the next I or L1 picture ...
            m_fparams.Refs().push_back((((fnum/2)/m_L1_sep+1)*m_L1_sep)*2+(fnum%2));

            m_fparams.SetExpiryTime( 2 );
        }

    }
    else{
        if (fnum/2==0)
        {
            m_fparams.SetPicSort( PictureSort::IntraRefPictureSort());

            m_fparams.SetExpiryTime( 1<<30 );//ie never
        }
        else if (fnum/2 % m_L1_sep==0)
        {
            m_fparams.SetPicSort( PictureSort::InterRefPictureSort());

            m_fparams.Refs().push_back(0);//picture 0 is the I picture

            if (fnum/2 != m_L1_sep)//we don't have the first L1 picture
                m_fparams.Refs().push_back(fnum-m_L1_sep*2);//other ref is the prior L1 picture

            //expires after the next L1 or I picture
            m_fparams.SetExpiryTime( m_L1_sep * 2 );
        }
        else
        {
            m_fparams.SetPicSort( PictureSort::InterNonRefPictureSort());

            m_fparams.Refs().push_back((fnum/m_L1_sep)*m_L1_sep);
            m_fparams.Refs().push_back(((fnum/m_L1_sep)+1)*m_L1_sep);
            m_fparams.SetExpiryTime( 1 );    //L2 frames could expire directly after being coded, but putting in a delay of 1
                                        //allows for picture-skipping to be done, since the picture will still be around to
                                        //be used if the next picture is skipped.
        }
    }
}
