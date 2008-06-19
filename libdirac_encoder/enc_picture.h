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
* Portions created by the Initial Developer are Copyright (C) 2008.
* All Rights Reserved.
*
* Contributor(s): Thomas Davies (Original Author),
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

#ifndef _ENC_PICTURE_H_
#define _ENC_PICTURE_H_

#include <libdirac_common/picture.h>

namespace dirac
{

class EncPicture : public Picture
{
public:
    EncPicture( const PictureParams& pp );

    virtual ~EncPicture();

    //! Initialises a copy of the data arrays into the original data
    void SetOrigData();

    //! Returns a given component of the original data
    const PicArray& OrigData(CompSort c) const { return *m_orig_data[(int) c];}

   //! Returns a given upconverted component of the original data
    const PicArray& UpOrigData(CompSort cs) const;

    //! Returns a version of the data suitable for motion estimation
    const PicArray& DataForME(bool field_coding, CompSort c) const;

    //! Returns a version of the data suitable for subpel motion estimation
    const PicArray& UpDataForME(bool field_coding, CompSort c) const;

private:

    virtual void ClearData();

    //! Filters a (field) picture vertically to reduce aliasing for motion estimation purposes
    void AntiAliasFilter( PicArray& out_data, const PicArray& in_data ) const;

    //! Returns an anti-aliased version of the original data
    const PicArray& FiltData(CompSort c) const;

    //! Returns an upconverted anti-aliased version of the original data
    const PicArray& UpFiltData(CompSort c) const;


    void SetOrigData(const int c);

    PicArray* m_orig_data[3];
    mutable PicArray* m_orig_up_data[3];
    mutable PicArray* m_filt_data[3];
    mutable PicArray* m_filt_up_data[3];

};


}

#endif
