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

#ifndef _CONTEXT_H_
#define _CONTEXT_H_

//Context classes, used in arithmetic (de)coding
/////////////////////////////////////////////////

#include "arrays.h"
#include <algorithm>

#define MyCodeTypeMax 0xffff
#define MyCodeTypeMSB (0xffff + 1) >> 1
#define MyCodeTypeSecondMSB (0xffff + 1) >> 2

typedef unsigned short int MyCodeType;
typedef unsigned long MyCalcType;

class Triple{

public:
	Triple():Start(0),Stop(0),Weight(0){}
	inline Triple& operator=(const Triple& rhs){Start=rhs.Start;Stop=rhs.Stop;Weight=rhs.Weight;return *this;}
	MyCodeType Start;	
	MyCodeType Stop;
	MyCodeType Weight;	
};


class Context{

public:
	Context() {set_counts(1,1);}
	Context(int cnt0,int cnt1) {set_counts(cnt0,cnt1);}

	inline void set_counts(int cnt0,int cnt1){count0=cnt0;count1=cnt1;set_triples();}
	inline MyCodeType get_count0(){return count0;}	
	inline MyCodeType get_count1(){return count1;}	
	inline void IncrCount(const bool& Symbol,const int& amnt);
	inline void HalveCounts();	
	inline MyCodeType& Weight(){return trip0.Weight;}
	inline Triple& GetTriple(const bool& symbol){return (symbol ? trip1 : trip0);}
	inline bool GetSymbol(const int& num, Triple& trip_val){
		if (num<trip0.Stop){
			trip_val=trip0;
			return false;//ie zero
		}
		else{
			trip_val=trip1;
			return true;//ie 1
		}
	} 

private:
	MyCodeType count0;
	MyCodeType count1;
	Triple trip0;
	Triple trip1;
	inline void set_triples();

};

inline void Context::set_triples(){
	//updates triples given counts
	trip0.Start=0;
	trip0.Stop=trip0.Start+count0;
	trip0.Weight=count0+count1;	

	trip1.Start=trip0.Stop;
	trip1.Stop=trip1.Start+count1;
	trip1.Weight=trip0.Weight;	
}

inline void Context::IncrCount(const bool& Symbol,const int& amnt){
	if (Symbol) 
		count1 += amnt; 
	else 
		count0 += amnt;
	set_triples();
}
inline void Context::HalveCounts(){
	count0>>=1;
	count0++;
	count1>>=1;
	count1++;
	set_triples();
}

#endif
