/* ***** BEGIN LICENSE BLOCK *****
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
* Contributor(s):
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

/*
*
* $Author$
* $Revision$
* $Log$
* Revision 1.4  2004-05-25 02:39:23  chaoticcoyote
* Unnecessary qualification of some class members in frame.h and pic_io.h.
* ISO C++ forbids variable-size automatic arrays; fixed in pic_io.cpp
* Removed spurious semi-colons in me_utils.cpp
* Fixed out-of-order member constructors in seq_compress.h
*
* Revision 1.3  2004/05/12 08:35:34  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.2  2004/04/06 18:06:53  chaoticcoyote
* Boilerplate for Doxygen comments; testing ability to commit into SF CVS
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#ifndef _FRAME_H_
#define _FRAME_H_

#include "libdirac_common/common.h"

//! A class for encapsulating all the data relating to a frame.
/*!
	A class for encapsulating all the data relating to a frame - all the component data, 
	including upconverted data.
 */
class Frame{

public:

    //! Constructor
    /*!
        Constructor initialises the frame parameters and the data
     */	
	Frame(const FrameParams& fp);

	//! Copy constructor
	Frame(const Frame& cpy);//copy constructor

	//! Destructor
	virtual ~Frame();

	//! Assignment =
	Frame& operator=(const Frame& rhs);

	//gets and sets
	//! Gets the frame parameters
	const FrameParams& GetFparams() const {return fparams;}
	//! Returns the luma data array
	PicArray& Ydata() {return *Y_data;}
	//! Returns the U component
	PicArray& Udata() {return *U_data;}
	//! Returns the V component 
	PicArray& Vdata() {return *V_data;}
	//! Returns the luma data array
	const PicArray& Ydata() const {return *Y_data;}
	//! Returns the U component
	const PicArray& Udata() const {return *U_data;}
	//! Returns the V component 
	const PicArray& Vdata() const {return *V_data;}
	//! Returns a given component 
	PicArray& Data(CompSort cs);
	//! Returns a given component
	const PicArray& Data(CompSort cs) const;	

	//! Returns upconverted Y data
	PicArray& UpYdata();
	//! Returns upconverted U data
	PicArray& UpUdata();
	//! Returns upconverted V data
	PicArray& UpVdata();
	//! Returns a given upconverted component
	PicArray& UpData(CompSort cs);
	//! Returns upconverted Y data
	const PicArray& UpYdata() const;
	//! Returns upconverted U data
	const PicArray& UpUdata() const;
	//! Returns upconverted V data	
	const PicArray& UpVdata() const;
	//! Returns a given upconverted component
	const PicArray& UpData(CompSort cs) const;

    //! Clip the data to prevent overshoot
    /*!
        Clips the data to lie between 0 and 1020 (4*255) in 10-bit form to prevent overshoot/wraparound.
     */
	void Clip();
private:
	FrameParams fparams;
	PicArray* Y_data;//the 
	PicArray* U_data;//component
	PicArray* V_data;//data
	mutable PicArray* upY_data;//upconverted data. Mutable because we
	mutable PicArray* upU_data;//create them on the fly even in const
	mutable PicArray* upV_data;//functions.
	void Init();//initialises the object once the frame parameters have been set
	void ClearData();//delete all the data.Called by constructor and also by Init()	
	void ClipComponent(PicArray& pic_data);	
};


#endif
