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
                         const int xlen,
                         const int ylen,
                         unsigned int luma_depth,
                         unsigned int chroma_depth,
                         bool using_ac) :
    m_ref_count(0),
    m_pparams(cf, xlen, ylen, luma_depth, chroma_depth),
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
                         const int xlen,
                         const int ylen,
                         unsigned int luma_depth,
                         unsigned int chroma_depth,
                         bool interlace,
                         bool using_ac) :
    m_ref_count(0),
    m_pparams(cf,xlen, ylen, luma_depth, chroma_depth),
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
    for (size_t i=0 ; i<m_pic_data.size() ; ++i)
    {
        delete m_pic_data[i];
    }//i

    // next create new arrays, copying from the initialising buffer
    m_pic_data.resize(cpy.m_pic_data.size());
    m_pic_in_use.resize(cpy.m_pic_in_use.size());
    for (size_t i=0 ; i<m_pic_data.size() ; ++i){
        m_pic_data[i] = new Picture( *(cpy.m_pic_data[i]) );
        m_pic_in_use[i] = cpy.m_pic_in_use[i];
    }//i

    // now copy the map
    m_pnum_map = cpy.m_pnum_map;

    // and the internal picture parameters
    m_pparams = cpy.m_pparams;

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
        for (size_t i=0 ; i<m_pic_data.size() ; ++i)
        {
            delete m_pic_data[i];
        }//i

        // next create new arrays, copying from the rhs
        m_pic_data.resize(rhs.m_pic_data.size());
        m_pic_in_use.resize(rhs.m_pic_in_use.size());
        for (size_t i=0 ; i<m_pic_data.size() ; ++i)
        {
            m_pic_data[i] = new Picture( *(rhs.m_pic_data[i]) );
            m_pic_in_use[i] = rhs.m_pic_in_use[i];
        }//i

        // now copy the map
        m_pnum_map = rhs.m_pnum_map;

        // and the internal picture parameters
        m_pparams = rhs.m_pparams;

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
    for (size_t i=0 ; i<m_pic_data.size() ;++i)
        delete m_pic_data[i];
}

Picture& PictureBuffer::GetPicture( const unsigned int pnum )
{//get picture with a given picture number, NOT with a given position in the buffer.
 //If the picture number does not occur, the first picture in the buffer is returned.

    std::map<unsigned int,unsigned int>::iterator it = m_pnum_map.find(pnum);

    unsigned int pos = 0;
    if (it != m_pnum_map.end())
        pos = it->second;

    return *(m_pic_data[pos]);
}

const Picture& PictureBuffer::GetPicture( const unsigned int pnum ) const
{    //as above, but const version

    std::map<unsigned int,unsigned int>::const_iterator it = m_pnum_map.find(pnum);

    unsigned int pos=0;
    if (it != m_pnum_map.end())
        pos = it->second;

    return *(m_pic_data[pos]);
}

Picture& PictureBuffer::GetPicture( const unsigned int pnum, bool& is_present )
{//get picture with a given picture number, NOT with a given position in the buffer.
 //If the picture number does not occur, the first picture in the buffer is returned.

    std::map<unsigned int,unsigned int>::iterator it = m_pnum_map.find(pnum);

    unsigned int pos = 0;
    if (it != m_pnum_map.end())
    {
        is_present = true;
        pos = it->second;
    }
    else
        is_present=false;

    return *(m_pic_data[pos]);
}

const Picture& PictureBuffer::GetPicture( const unsigned int pnum, bool& is_present ) const
{    //as above, but const version

    std::map<unsigned int,unsigned int>::const_iterator it = m_pnum_map.find(pnum);

    unsigned int pos=0;
    if (it != m_pnum_map.end())
    {
        is_present = true;
        pos = it->second;
    }
    else
        is_present=false;

    return *(m_pic_data[pos]);
}

bool PictureBuffer::IsPictureAvail( const unsigned int pnum ) const
{

    std::map<unsigned int,unsigned int>::const_iterator it = m_pnum_map.find(pnum);

    if (it != m_pnum_map.end())
        return true;
    else
        return false;
}

PicArray& PictureBuffer::GetComponent( const unsigned int pnum , CompSort c)
{//as GetPicture, but returns corresponding component

    std::map<unsigned int,unsigned int>::iterator it = m_pnum_map.find(pnum);

    unsigned int pos = 0;
    if (it!=m_pnum_map.end())
        pos = it->second;

    if (c == U_COMP)
        return m_pic_data[pos]->Udata();
    else if (c == V_COMP)
        return m_pic_data[pos]->Vdata();
    else
        return m_pic_data[pos]->Ydata();
}

const PicArray& PictureBuffer::GetComponent( const unsigned int pnum , CompSort c ) const
{//as above, but const version

    std::map<unsigned int,unsigned int>::const_iterator it = m_pnum_map.find(pnum);

    // FIXME - how do we handle the condition when a picture matching pnum is
    // not found???
    unsigned int pos = 0;

    if (it!=m_pnum_map.end())
        pos = it->second;

    if (c==U_COMP)
        return m_pic_data[pos]->Udata();
    else if (c==V_COMP)
        return m_pic_data[pos]->Vdata();
    else
        return m_pic_data[pos]->Ydata();
}

// as GetPicture, but returns corresponding upconverted component
PicArray& PictureBuffer::GetUpComponent(const unsigned int pnum, CompSort c){
    std::map<unsigned int,unsigned int>::iterator it = m_pnum_map.find(pnum);

    unsigned int pos = 0;
    if (it!=m_pnum_map.end())
        pos = it->second;

    if (c == U_COMP)
        return m_pic_data[pos]->UpUdata();
    else if (c == V_COMP)
        return m_pic_data[pos]->UpVdata();
    else
        return m_pic_data[pos]->UpYdata();

}

const PicArray& PictureBuffer::GetUpComponent(const unsigned int pnum, CompSort c) const {//as above, but const version
    std::map<unsigned int,unsigned int>::const_iterator it=m_pnum_map.find(pnum);

    unsigned int pos = 0;
    if (it!=m_pnum_map.end())
        pos = it->second;

    if (c == U_COMP)
        return m_pic_data[pos]->UpUdata();
    else if (c == V_COMP)
        return m_pic_data[pos]->UpVdata();
    else
        return m_pic_data[pos]->UpYdata();

}

std::vector<int> PictureBuffer::Members() const
{
    std::vector<int> members( 0 );
    for (unsigned int i=0; i<m_pic_data.size(); ++i )
    {
        if ( m_pic_in_use[i] == true )
        {
            const PictureParams& fparams = m_pic_data[i]->GetPparams();
            members.push_back( fparams.PictureNum() );
        }
    }// i

    return members;
}


void PictureBuffer::PushPicture(const unsigned int pic_num)
{// Put a new picture onto the top of the stack using built-in picture parameters
 // with picture number pic_num


    // if picture is present - return
    if (IsPictureAvail(pic_num))
        return;

    m_pparams.SetPictureNum(pic_num);
    if ( m_pparams.PicSort().IsRef() )
        m_ref_count++;

    int new_pic_pos = -1;
    // First check if an unused picture is available in the buffer
    for (int i = 0; i < (int)m_pic_in_use.size(); ++i)
    {
        if (m_pic_in_use[i] == false)
        {
            new_pic_pos = i;
            m_pic_data[i]->ReconfigPicture(m_pparams);
            m_pic_in_use[i] = true;
            break;
        }
    }
    if (new_pic_pos == -1)
    {
        // No unused frames in buffer. Allocate a new picture
        Picture* fptr = new Picture(m_pparams);
        // add the picture to the buffer
        m_pic_data.push_back(fptr);
        m_pic_in_use.push_back(true);
        new_pic_pos = m_pic_data.size()-1;
    }

    // put the picture number into the index table
    std::pair<unsigned int,unsigned int> temp_pair(m_pparams.PictureNum() , new_pic_pos);
    m_pnum_map.insert(temp_pair);
}

void PictureBuffer::PushPicture( const PictureParams& pp )
{// Put a new picture onto the top of the stack

    // if picture is present - return
    if (IsPictureAvail(pp.PictureNum()))
        return;

    if ( pp.PicSort().IsRef() )
        m_ref_count++;

    int new_pic_pos = -1;
    // First check if an unused picture is available in the buffer
    for (int i = 0; i < (int)m_pic_in_use.size(); ++i)
    {
        if (m_pic_in_use[i] == false)
        {
            new_pic_pos = i;
            m_pic_data[i]->ReconfigPicture(pp);
            m_pic_in_use[i] = true;
            break;
        }
    }
    if (new_pic_pos == -1)
    {
        // No unused frames in buffer. Allocate a new picture
        Picture* pptr = new Picture(pp);
        // add the picture to the buffer
        m_pic_data.push_back(pptr);
        new_pic_pos = m_pic_data.size()-1;
        m_pic_in_use.push_back(true);
    }
    // put the picture number into the index table
    std::pair<unsigned int,unsigned int> temp_pair(pp.PictureNum() , new_pic_pos);
    m_pnum_map.insert(temp_pair);
}

void PictureBuffer::PushPicture( const Picture& picture )
{
    int pnum = picture.GetPparams().PictureNum();
    SetPictureParams( pnum );
    PushPicture(pnum);

    bool is_present;

    Picture &f = GetPicture(picture.GetPparams().PictureNum(), is_present);
    if(is_present)
        picture.CopyContents(f);
}

void PictureBuffer::Remove(const unsigned int pos)
{//remove picture pnum from the buffer, shifting everything above down

    const PictureParams& fparams = m_pic_data[pos]->GetPparams();

    if ( m_pic_in_use[pos] == true && fparams.PicSort().IsRef() )
        m_ref_count--;

    std::pair<unsigned int,unsigned int>* tmp_pair;

    if (pos<m_pic_data.size())
    {
        //flag that picture is no longer in use
        m_pic_in_use[pos] = false;

         //make a new map
        m_pnum_map.clear();
        for (size_t i=0 ; i<m_pic_data.size() ; ++i)
        {
            if (m_pic_in_use[i] == true)
            {
                tmp_pair = new std::pair<unsigned int,unsigned int>( m_pic_data[i]->GetPparams().PictureNum() , i);
                m_pnum_map.insert(*tmp_pair);
                delete tmp_pair;
            }
        }//i
    }
}


void PictureBuffer::SetRetiredPictureNum(const int show_pnum, const int current_coded_pnum)
{
    if ( IsPictureAvail(current_coded_pnum))
    {
        PictureParams &fparams = GetPicture(current_coded_pnum).GetPparams();
        fparams.SetRetiredPictureNum(-1);
        for (size_t i=0 ; i<m_pic_data.size() ; ++i)
        {
            if (m_pic_in_use[i] == true && (m_pic_data[i]->GetPparams().PictureNum() + m_pic_data[i]->GetPparams().ExpiryTime() ) <= show_pnum)
            {
                // Only _reference_ frames can be retired - the
                // decoder will retire non-reference frames as they are displayed
                if (m_pic_data[i]->GetPparams().PicSort().IsRef() )
                {
                    fparams.SetRetiredPictureNum(m_pic_data[i]->GetPparams().PictureNum()); 
                    break;
                }
            }
        }//i
    }
}
void PictureBuffer::CleanAll(const int show_pnum, const int current_coded_pnum)
{// clean out all frames that have expired
    if (IsPictureAvail(current_coded_pnum))
    {
        for (size_t i=0 ; i<m_pic_data.size() ; ++i)
        {
            if (m_pic_in_use[i] == true && (m_pic_data[i]->GetPparams().PictureNum() + m_pic_data[i]->GetPparams().ExpiryTime() ) <= show_pnum)
                Remove(i);
        }//i
    }
}

void PictureBuffer::CleanRetired(const int show_pnum, const int current_coded_pnum)
{// clean out all frames that have expired
    if ( IsPictureAvail(current_coded_pnum) )
    {
        PictureParams &fparams = GetPicture(current_coded_pnum).GetPparams();
        // Remove Reference picture specified in retired picture number.
        if (fparams.PicSort().IsRef() && fparams.RetiredPictureNum()>= 0)
            Clean(fparams.RetiredPictureNum());
        fparams.SetRetiredPictureNum(-1);
        // Remove non-reference frames that have expired
        for (size_t i=0 ; i<m_pic_data.size() ; ++i)
        {
            if (m_pic_in_use[i] == true && (m_pic_data[i]->GetPparams().PictureNum() + m_pic_data[i]->GetPparams().ExpiryTime() ) <= show_pnum && m_pic_data[i]->GetPparams().PicSort().IsNonRef())
                Remove(i);
        }//i
    }
}

void PictureBuffer::Clean(const int pnum)
{// clean out all frames that have expired
    for (size_t i=0 ; i<m_pic_data.size() ; ++i)
    {
        if (m_pic_in_use[i] == true && m_pic_data[i]->GetPparams().PictureNum() == pnum)
        {
            Remove(i);
        }
    }//i
}

void PictureBuffer::SetPictureParams( const unsigned int pnum )
{
    m_pparams.SetUsingAC( m_using_ac);
    if (!m_interlace)
        SetProgressiveParams(pnum);
    else
        SetInterlacedParams(pnum);
}

void PictureBuffer::SetProgressiveParams( const unsigned int pnum )
{
    // Set the picture parameters, given the GOP set-up and the picture number in display order
    // This function can be ignored by setting the picture parameters directly if required

    m_pparams.SetPictureNum( pnum );
    m_pparams.SetRetiredPictureNum( -1 );
    m_pparams.Refs().clear();

    if ( m_gop_len>0 )
    {

        if ( pnum % m_gop_len == 0)
        {
            if (m_gop_len > 1)
                m_pparams.SetPicSort( PictureSort::IntraRefPictureSort());
            else // I-picture only coding
            {
                m_pparams.SetPicSort( PictureSort::IntraNonRefPictureSort());
            }    
            // I picture expires after we've coded the next I picture
            m_pparams.SetExpiryTime( m_gop_len );
        }
        else if (pnum % m_L1_sep == 0)
        {
            m_pparams.SetPicSort( PictureSort::InterRefPictureSort());

            // Ref the previous I or L1 picture
            m_pparams.Refs().push_back( pnum - m_L1_sep );

            // if we don't have the first L1 picture ...
            if ((pnum-m_L1_sep) % m_gop_len>0)
                // ... other ref is the prior I picture
                m_pparams.Refs().push_back( ( pnum/m_gop_len ) * m_gop_len  );

            // Expires after the next L1 or I picture
            m_pparams.SetExpiryTime( m_L1_sep );
        }
        else if ((pnum+1) % m_L1_sep == 0)
        {
            m_pparams.SetPicSort( PictureSort::InterNonRefPictureSort());

            // .. and the previous picture
            m_pparams.Refs().push_back(pnum-1);
            // Refs are the next I or L1 picture ...
            m_pparams.Refs().push_back(pnum+1);

            m_pparams.SetExpiryTime( 1 );
        }
        else
        {
            m_pparams.SetPicSort( PictureSort::InterRefPictureSort());

            // .. and the previous picture
            m_pparams.Refs().push_back(pnum-1);
            // Refs are the next I or L1 picture ...
            m_pparams.Refs().push_back(((pnum/m_L1_sep)+1)*m_L1_sep);

            m_pparams.SetExpiryTime( 22 );
        }

    }
    else{
        if (pnum==0)
        {
            m_pparams.SetPicSort( PictureSort::IntraRefPictureSort());

            m_pparams.SetExpiryTime( 1<<30 );//ie never
        }
        else if (pnum % m_L1_sep==0)
        {
            m_pparams.SetPicSort( PictureSort::InterRefPictureSort());

            m_pparams.Refs().push_back(0);//picture 0 is the I picture

            if (pnum != m_L1_sep)//we don't have the first L1 picture
                m_pparams.Refs().push_back(pnum-m_L1_sep);//other ref is the prior L1 picture

            //expires after the next L1 or I picture
            m_pparams.SetExpiryTime( m_L1_sep );
        }
        else
        {
            m_pparams.SetPicSort( PictureSort::InterNonRefPictureSort());

            m_pparams.Refs().push_back((pnum/m_L1_sep)*m_L1_sep);
            m_pparams.Refs().push_back(((pnum/m_L1_sep)+1)*m_L1_sep);
            m_pparams.SetExpiryTime( 1 );    //L2 frames could expire directly after being coded, but putting in a delay of 1
                                        //allows for picture-skipping to be done, since the picture will still be around to
                                        //be used if the next picture is skipped.
        }
    }
}

void PictureBuffer::SetInterlacedParams( const unsigned int pnum )
{
    // Set the picture parameters, given the GOP set-up and the picture number in display order
    // This function can be ignored by setting the picture parameters directly if required

    m_pparams.SetPictureNum( pnum );
    m_pparams.SetRetiredPictureNum( -1 );
    m_pparams.Refs().clear();


    if ( m_gop_len>0 )
    {

        if ( (pnum/2) % m_gop_len == 0)
        {
            // Field 1 is Intra Field
            if (m_gop_len > 1)
            {
                m_pparams.SetPicSort( PictureSort::IntraRefPictureSort());
                // I picture expires after we've coded the next I picture
                m_pparams.SetExpiryTime( m_gop_len * 2);
                if (m_interlace && pnum%2)
                {
                    m_pparams.SetPicSort( PictureSort::InterRefPictureSort());
                    // Ref the previous I field
                    m_pparams.Refs().push_back( pnum-1 );
                }
            }
            else
            {
                // I-picture only coding
                m_pparams.SetPicSort( PictureSort::IntraNonRefPictureSort());
                m_pparams.SetExpiryTime( m_gop_len );
            }
        }
        else if ((pnum/2) % m_L1_sep == 0)
        {
            m_pparams.SetPicSort( PictureSort::InterRefPictureSort());

            if (pnum%2)
            {
                // Field 2
                // Ref the first field of same picture
                m_pparams.Refs().push_back( pnum - 1);
                // Ref the previous field 2 of I or L1 picture
                m_pparams.Refs().push_back( pnum - m_L1_sep*2 );
            }
            else
            {
                // Field 1
                // Ref the field 1 of previous I or L1 picture
                m_pparams.Refs().push_back( pnum - m_L1_sep*2 );
                // Ref the field 2 of previous I or L1 picture
                m_pparams.Refs().push_back( pnum - m_L1_sep*2 + 1 );
            }

            // Expires after the next L1 or I picture
            m_pparams.SetExpiryTime( (m_L1_sep+1)*2-1 );
        }
        else if ((pnum/2+1) % m_L1_sep == 0)
        {
            // Bi-directional non-reference fields.
            m_pparams.SetPicSort( PictureSort::InterNonRefPictureSort());

            // .. and the same parity field of the previous picture
            m_pparams.Refs().push_back(pnum-1*2);
            // Refs are the same parity fields in the next I or L1 picture ...
            m_pparams.Refs().push_back(pnum+1*2);

            m_pparams.SetExpiryTime( 1 );
        }
        else
        {
            // Bi-directional reference fields.
            m_pparams.SetPicSort( PictureSort::InterRefPictureSort());

            // .. and the same parity field of the previous picture
            m_pparams.Refs().push_back(pnum-1*2);
            // Refs are the same parity fields in the next I or L1 picture ...
            m_pparams.Refs().push_back((((pnum/2)/m_L1_sep+1)*m_L1_sep)*2+(pnum%2));

            m_pparams.SetExpiryTime( 4 );
        }

    }
    else{
        if (pnum/2==0)
        {
            m_pparams.SetPicSort( PictureSort::IntraRefPictureSort());

            m_pparams.SetExpiryTime( 1<<30 );//ie never
        }
        else if (pnum/2 % m_L1_sep==0)
        {
            m_pparams.SetPicSort( PictureSort::InterRefPictureSort());

            m_pparams.Refs().push_back(0);//picture 0 is the I picture

            if (pnum/2 != m_L1_sep)//we don't have the first L1 picture
                m_pparams.Refs().push_back(pnum-m_L1_sep*2);//other ref is the prior L1 picture

            //expires after the next L1 or I picture
            m_pparams.SetExpiryTime( m_L1_sep * 2 );
        }
        else
        {
            m_pparams.SetPicSort( PictureSort::InterNonRefPictureSort());

            m_pparams.Refs().push_back((pnum/m_L1_sep)*m_L1_sep);
            m_pparams.Refs().push_back(((pnum/m_L1_sep)+1)*m_L1_sep);
            m_pparams.SetExpiryTime( 1 );    //L2 frames could expire directly after being coded, but putting in a delay of 1
                                        //allows for picture-skipping to be done, since the picture will still be around to
                                        //be used if the next picture is skipped.
        }
    }
}
