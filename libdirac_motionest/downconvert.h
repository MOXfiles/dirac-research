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
* Revision 1.4  2004-05-12 08:35:34  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.3  2004/04/11 22:50:46  chaoticcoyote
* Modifications to allow compilation by Visual C++ 6.0
* Changed local for loop declarations into function-wide definitions
* Replaced variable array declarations with new/delete of dynamic array
* Added second argument to allocator::alloc calls, since MS has no default
* Fixed missing and namespace problems with min, max, cos, and abs
* Added typedef unsigned int uint (MS does not have this)
* Added a few missing std:: qualifiers that GCC didn't require
*
* Revision 1.2  2004/04/05 03:05:02  chaoticcoyote
* Updated Doxygen API documentation comments
* Test to see if Scott's CVS is now working correctly
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#ifndef _DOWNCONVERT_H_
#define _DOWNCONVERT_H_

#include "libdirac_common/common.h"

//! A class for fast downconversion of picture data
/*!
    A class for fast down-conversion of picture data. The picture data is downconverted by a factor of two in each dimension,
	using fast filtering techniques. The filter is a half-band filter designed to trade off frequency response, ringiness
	and aliasing.
 */
class DownConverter{

public:

	//! Constructor
	DownConverter();
	//! Destructor
	~DownConverter(){};

	//! A function to do the actual down-conversion 
	/*!
		A function to do the actual downconversion. 
		/param	old_data	the picture data to be downconverted
		/param	new_data	the resulting down-converted data. The array must be of the correct size.
     */
	void DoDownConvert(const PicArray& old_data, PicArray& new_data);

private:
	//Copy constructor
	DownConverter(const DownConverter& cpy);//private, body-less: class should not be copied
	//Assignment=
	DownConverter& operator=(const DownConverter& rhs);//private, body-less: class should not be assigned

	//Applies the filter to a single column
	void RowLoop(int &colpos,const PicArray& old_data,PicArray& new_data);

	//Variables to keep the loops in check
	int yOld;
	int xOld;

	ValueType* row_buffer;

	//Define filter parameters
	static const int Stage_I_Size = 6;
	static const int StageI_I = 86;
	static const int StageI_II = 46;
	static const int StageI_III = 4;
	static const int StageI_IV = -8; 
	static const int StageI_V = -4;
	static const int StageI_VI = 4;
	static const int StageI_Shift = 8;
};

#endif
