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
* Revision 1.4  2004-06-08 16:03:15  timborer
* Files updated so that code compiles under Windows
* (previously broken under Windows).
* Colour matrix coefficients corrected in video conversion utilities
* Video conversion utilites now build with the rest of the code.
*
* Revision 1.3  2004/05/12 08:35:34  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.2  2004/04/11 22:50:46  chaoticcoyote
* Modifications to allow compilation by Visual C++ 6.0
* Changed local for loop declarations into function-wide definitions
* Replaced variable array declarations with new/delete of dynamic array
* Added second argument to allocator::alloc calls, since MS has no default
* Fixed missing and namespace problems with min, max, cos, and abs
* Added typedef unsigned int uint (MS does not have this)
* Added a few missing std:: qualifiers that GCC didn't require
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#include "libdirac_common/golomb.h"
#include "libdirac_common/bit_manager.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

using std::vector;

void UnsignedGolombCode(BasicOutputManager& bitman, const unsigned int val){
	unsigned int M=0;
	unsigned int info;
	unsigned int val2=val;

	val2++;
	while (val2>1){//get the log base 2 of val.
		val2>>=1;
		M++;		
	}
	info=val-(1<<M)+1;

	//add length M+1 prefix
	for (unsigned int I=1;I<=M;++I){
		bitman.OutputBit(0);
	}
	bitman.OutputBit(1);
	//add info bits
	for (unsigned int I=0;I<M;++I){
		bitman.OutputBit(info & (1<<I));		
	}

}
void UnsignedGolombCode(std::vector<bool>& bitvec, const unsigned int val){
	unsigned int M=0;
	unsigned int info;
	unsigned int val2=val;

	bitvec.clear();
	val2++;
	while (val2>1){//get the log base 2 of val.
		val2>>=1;
		M++;		
	}
	info=val-(1<<M)+1;

	//add length M+1 prefix
	for (unsigned int I=1;I<=M;++I){
		bitvec.push_back(0);
	}
	bitvec.push_back(1);

	//add info bits
	for (unsigned int I=0;I<M;++I){
		bitvec.push_back(info & (1<<I));
	}

}

void GolombCode(BasicOutputManager& bitman, const int val){

	//code the magnitude
	UnsignedGolombCode(bitman,(unsigned int) abs(val));

	//do sign
	if (val>0) bitman.OutputBit(1);
	else if (val<0) bitman.OutputBit(0);
}

void GolombCode(vector<bool>& bitvec, const int val){

	//code the magnitude
	UnsignedGolombCode(bitvec,(unsigned int) abs(val));

	//do sign
	if (val>0) bitvec.push_back(1);
	else if (val<0) bitvec.push_back(0);
}

unsigned int UnsignedGolombDecode(BitInputManager& bitman){	
	unsigned int M=0;
	unsigned int info=0;
	bool bit=0;
	unsigned int val=0;

	do{
		bit=bitman.InputBit();
		if (!bit)
			M++;
	}
	while(!bit && M<64);//terminate if the number is too big!

 	//now get the M info bits	
	for (unsigned int I=0;I<M;++I){
		bit=bitman.InputBit();
		if (bit)
			info|=(1<<I);
	}	
	val=(1<<M)-1+info;

	return val;
}

unsigned int UnsignedGolombDecode(const std::vector<bool>& bitvec){
	unsigned int	M=0;
	unsigned int info=0;
	bool bit=0;
	unsigned int val=0;

	unsigned int index=0;//index into bitvec

	do{
		bit=bitvec[++index];
		if (!bit)
			M++;
	}
	while(!bit && M<64);//terminate if the number is too big!

 	//now get the M info bits	
	for (unsigned int I=0;I<M;++I){
		bit=bitvec[++index];
		if (bit)
			info|=(1<<I);
	}	
	val=(1<<M)-1+info;

	return val;
}

int GolombDecode(BitInputManager& bitman){

	int val=int(UnsignedGolombDecode(bitman));
	bool bit;

 	//get the sign
	if (val!=0){
		bit=bitman.InputBit();
		if (!bit)
			val=-val;
	}
	return val;		
}

int GolombDecode(const vector<bool>& bitvec){

	int val=int(UnsignedGolombDecode(bitvec));
	bool bit;

 	//get sign
	if (val!=0){
		bit=bitvec[bitvec.size()-1];
		if (!bit)
			val=-val;
	}
	return val;		
}
