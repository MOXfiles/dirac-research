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
* Revision 1.1  2004-03-11 17:45:43  timborer
* Initial revision
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#include "golomb.h"
#include "bit_manager.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

using std::vector;

void GolombCode(BasicOutputManager& bitman, const int val){

	int M=0;
	int info;
	int val2=abs(val);

	val2++;
	while (val2>1){//get the log base 2 of val.
		val2>>=1;
		M++;		
	}
	info=abs(val)-(1<<M)+1;

	//add length M+1 prefix
	for (int I=1;I<=M;++I){
		bitman.OutputBit(0);
	}
	bitman.OutputBit(1);
	//add info bits
	for (int I=0;I<M;++I){
		bitman.OutputBit(info & (1<<I));		
	}
	//do sign
	if (val>0){
		bitman.OutputBit(1);
	}
	else if (val<0){
		bitman.OutputBit(0);
	}
}



void GolombCode(vector<bool>& bitvec, const int val){

	int M;
	int info;
	bitvec.clear();

	M=int(floor(log(double(abs(val)+1))/log(2.0)));
	info=abs(val)-(1<<M)+1;

	//add length M+1 prefix
	for (int I=1;I<=M;++I){
		bitvec.push_back(0);
	}
	bitvec.push_back(1);

	//add info bits
	for (int I=0;I<M;++I){
		bitvec.push_back(info & (1<<I));
	}

	//do sign
	if (val>0){
		bitvec.push_back(1);
	}
	else if (val<0){
		bitvec.push_back(0);
	}
}


int GolombDecode(BitInputManager& bitman){

	int	M=0;
	int info=0;
	bool bit=0;
	int val=0;

	do{
		bit=bitman.InputBit();
		if (!bit)
			M++;
	}
	while(!bit && M<64);//terminate if the number is too big!

 	//now get the M info bits	
	for (int I=0;I<M;++I){
		bit=bitman.InputBit();
		if (bit)
			info|=(1<<I);
	}	
	val=(1<<M)-1+info;

 	//finally, get sign
	if (val!=0){
		bit=bitman.InputBit();
		if (!bit)
			val=-val;
	}
	return val;		
}

int GolombDecode(const vector<bool>& bitvec){

	int	M=0;
	int info=0;
	bool bit=0;
	int val=0;

	int index=0;//index into bitvec

	do{
		bit=bitvec[++index];
		if (!bit)
			M++;
	}
	while(!bit && M<64);//terminate if the number is too big!

 	//now get the M info bits	
	for (int I=0;I<M;++I){
		bit=bitvec[++index];
		if (bit)
			info|=(1<<I);
	}	
	val=(1<<M)-1+info;

 	//finally, get sign
	if (val!=0){
		bit=bitvec[++index];
		if (!bit)
			val=-val;
	}
	return val;		
}
