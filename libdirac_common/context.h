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
* Revision 1.3  2004/04/11 22:54:13  chaoticcoyote
* Additional comments
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

#ifndef _CONTEXT_H_
#define _CONTEXT_H_

//Context classes, used in arithmetic (de)coding
/////////////////////////////////////////////////

#include "libdirac_common/arrays.h"
#include <algorithm>

#define MyCodeTypeMax 0xffff
#define MyCodeTypeMSB (0xffff + 1) >> 1
#define MyCodeTypeSecondMSB (0xffff + 1) >> 2

typedef unsigned short int MyCodeType;
typedef unsigned long MyCalcType;

//! A class for encapsulating interval fractions for use in arithmetic coding.
/*!
     A class for encapsulating a subinterval of the unit interval [0,1) (0<=x<1) as a Start value,
	 a Stop value (numerators) and a Weight value (the denominator). The interval is the to be
	 interpreted as [Start/Weight,Stop/Weight).
 */
class Triple{

public:

    //! Constructor.
	Triple():Start(0),Stop(0),Weight(0){}

    //! Assignment= 
	inline Triple& operator=(const Triple& rhs){Start=rhs.Start;Stop=rhs.Stop;Weight=rhs.Weight;return *this;}

    //! The start value. 
	MyCodeType Start;	

    //! The stop value.Should be >=Start. 
	MyCodeType Stop;

    //! The denominator for interpreting Start, Stop. Should be >=Start,Stop. 
	MyCodeType Weight;	
};

//! A class for binary contexts.
/*!
	A class for binary contexts. Stores probabilities for 0 and 1 in terms of counts of numbers of 
	occurrences, and also as Triples partitioning the interval [0,1) into two parts [0,p) and [p,1). 
 */
class Context{

public:
    //! Default Constructor.
    /*!
        Default constructor initialises counts to 1 each of 0 and 1.
     */	
	Context() {SetCounts(1,1);}

    //! Constructor.
    /*!
        Constructor initialises the counts to those set.
     */	
	Context(int cnt0,int cnt1) {SetCounts(cnt0,cnt1);}

	//! Copy constructor
	Context(const Context& cpy): 
	count0(cpy.count0),
	count1(cpy.count1),
	trip0(cpy.trip0),
	trip1(cpy.trip1){}

	//! Assignment=
	Context& operator=(const Context& rhs){count0=rhs.count0;count1=rhs.count1;trip0=rhs.trip0;trip1=rhs.trip1;return *this;}
	//! Destructor
	~Context(){}

    //! Sets the counts according to the input.
    /*!
        Sets the counts, and then the triples to reflect the counts.
     */	
	inline void SetCounts(int cnt0,int cnt1){count0=cnt0;count1=cnt1;set_triples();}
	//! Returns the count of zeroes.
	inline MyCodeType GetCount0() const {return count0;}	
	//! Returns the count of ones.
	inline MyCodeType GetCount1() const {return count1;}	

    //! Increment the count.
    /*!
        Increment the count of Symbol by amnt.
		/param	Symbol	the symbol whose count is to be incremented (false=0, true=1)
		/param	amnt	the amount to increment by
     */	
	inline void IncrCount(const bool& Symbol,const int& amnt);

	 //! Divide the counts by 2, making sure neither ends up 0.
	inline void HalveCounts();	
	//! Return the weight, equal to the count of 0 plus the count of 1.	
	inline MyCodeType Weight() const {return trip0.Weight;}
   	//! Return the triple associated with Symbol.	
	inline const Triple& GetTriple(const bool& symbol) const {return (symbol ? trip1 : trip0);}

    //! Given a number, return the corresponding symbol and triple.
    /*!
        Given a number, which should be in the range [0,Weight) return the corresponding symbol.
		The range [0,Weight) is partitioned into portions [0,count0), [count0,Weight) corresponding
		to 0 and 1.
     */	
	inline bool GetSymbol(const int& num, Triple& trip_val) const {
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
