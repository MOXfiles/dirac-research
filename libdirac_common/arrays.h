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

#ifndef _ARRAYS_H_
#define _ARRAYS_H_

//basic array types used for pictures etc

#include <memory>
#include <cstddef>
#include <stdexcept>
#include <iostream>

typedef short ValueType;
typedef int CalcValueType;

//! Range type. 
/*!
	Range type encapsulating a closed range of values [first,last]. Used to initialies OneDArrays.
 */
class Range{
public:
	//! Constructor
    /*!
		Constructor taking a start and an end point for the range.
     */
	Range(int s, int e):fst(s),lst(e){}
	//! Returns the start of the range.
	const int first() const {return fst;}
	//! Returns the end point of the range.
	const int last() const {return lst;}
private:
	int fst,lst;
};

//////////////////////////////
//One-Dimensional Array type//
//////////////////////////////

//! A template class for one-dimensional arrays.
/*!
	A template class for one-D arrays. Can be used wherever built-in arrays are used, and 
	eliminates the need for explicit memory (de-)allocation. Also supports arrays not 
	based at zero.
 */
template <class T> class OneDArray{
public:
    //! Default constructor.
    /*!
		Default constructor produces an empty array.
     */	
	OneDArray(){Init(0);}

    //! 'Length' constructor.
    /*!
		Length constructor produces a zero-based array.
     */	
	OneDArray(int len){Init(len);}

   //! Range constructor
    /*!
		Range constructor produces an array with values indexed within the range parameters.
		/param	r	a range of indexing values.
     */		
	OneDArray(Range r){Init(r);}

	//! Destructor.
    /*!
		Destructor frees the data allocated in the constructors.
     */
	~OneDArray(){
		FreePtr();
	}

	//! Copy constructor.
    /*!
		Copy constructor copies both data and metadata.
     */
	OneDArray(const OneDArray<T>& Cpy);

	//! Assignment=
    /*!
		Assignment= assigns both data and metadata.
     */
	OneDArray<T>& operator=(const OneDArray<T>& rhs);	

	//! Resize the array, throwing away the current data.
	void resize(int l);

	//! Element access.
	T& operator[](int pos){return ptr[pos-fst];}

	//! Element access.
	const T& operator[](int pos) const {return ptr[pos-fst];}

    //! Returns the length of the array if n=0, 0 otherwise.
	int length(const int n)const {if (n==0) return l; else return 0;}

    //! Returns the length of the array.	
	int length() const {return l;}

    //! Returns the index of the first element if n=0, 0 otherwise.	
	int first(const int n) const {if (n==0) return fst; else return 0;}

    //! Returns the index of the first element.	
	int first() const {return fst;}

    //! Returns the index of the first element, Blitz-style.	
	int lbound(const int n) const {return first(n);}

    //! Returns the index of the last element if n=0, 0 otherwise.	
	int last(const int n) const {if (n==0) return lst; else return -1;}

    //! Returns the index of the last element.	
	int last() const {return lst;}

    //! Returns the index of the last element, Blitz-style.
	int ubound(const int n) const {return last(n);}	
private:
	void Init(int len);
	void Init(Range r);
	void FreePtr();	

	std::allocator<T> alloc;
	int fst, lst;
	int l;
	T* ptr;
};

//public member functions//
///////////////////////////

template <class T>
OneDArray<T>::OneDArray(const OneDArray<T>& Cpy){
	fst=Cpy.fst;
	lst=Cpy.lst;
	l=lst-fst+1;

	if (fst==0)		
		Init(l);
	else
		Init(Range(fst,lst));
	for (int I=0;I<l;++I)
		*(ptr+I)=*(Cpy.ptr+I);
}

template <class T>
OneDArray<T>& OneDArray<T>::operator=(const OneDArray<T>& rhs){
	if (&rhs!=this){
		FreePtr();
		fst=rhs.fst;
		lst=rhs.lst;
		l=rhs.l;			
		if (fst==0)
			Init(l);
		else
			Init(Range(fst,lst));
		for (int I=0;I<l;++I)
			*(ptr+I)=*(rhs.ptr+I);			

	}
	return *this;
}

template <class T> 
void OneDArray<T>::resize(int l){	
	FreePtr();
	Init(l);
}

//private member functions//
////////////////////////////

template <class T>
void OneDArray<T>::Init(int len)
{
	l=len;
	fst=0;
	lst=l-1;
	if (l>0)
	{
		ptr=alloc.allocate(l);
	}
	else {
		l=0;
		fst=0;
		lst=-1;
	}
}		

template <class T>
void OneDArray<T>::Init(Range r)
{
	T tmp_val;
	fst=r.first();
	lst=r.last();
	l=lst-fst+1; 
	if (l>0){
		ptr=alloc.allocate(l);
		std::uninitialized_fill(ptr,ptr+l,tmp_val);
	}
	else {
		l=0;
		fst=0;
		lst=-1;
	}
}

template <class T>
void OneDArray<T>::FreePtr()
{
	if (l>0)
	{
		int I=l;
		while (I!=0)
			alloc.destroy(&ptr[--I]);
		alloc.deallocate(ptr,l);		

	}
}


//////////////////////////////
//Two-Dimensional Array type//
//////////////////////////////

//! A template class for two-dimensional arrays.
/*!
	A template class to do two-d arrays, so that explicit memory (de-)allocation is not required. Only
	zero-based arrays are currently supported so that access is fast. The array is viewed as a 
	(vertical) array of (horizontal) arrays. Accessing elements along a row is therefore much faster
	than accessing them along a column.
 */
template <class T> class TwoDArray{
	typedef T* element_type;
public:

    //! Default constructor.
    /*!
		Default constructor creates an empty array.
     */	
	TwoDArray(){Init(0,0);}

    //! Constructor.
    /*!
		The constructor creates an array of width /param len0 and length /param len1.
     */	
	TwoDArray(int len0,int len1){Init(len0,len1);}

    //! Destructor.
    /*!
		Destructor frees the data allocated in the constructor.
     */	
	virtual ~TwoDArray(){
		FreeData();	
	}

    //! Copy constructor.
    /*!
		Copy constructor copies data and metadata.
     */	
	TwoDArray(const TwoDArray<T>& Cpy);

    //! Assignment =
    /*!
		Assignement = assigns both data and metadata.
     */	
	TwoDArray<T>& operator=(const TwoDArray<T>& rhs);

    //! Resizes the array, deleting the current data.	
	void resize(int len0, int len1);	

    //! Element access.
    /*!
		Accesses the rows of the arrays, which are returned in the form of pointers to the row data
		NOT OneDArray objects.
     */	
	inline element_type& operator[](const int pos){return array_of_rows[pos];}

   //! Element access.
    /*!
		Accesses the rows of the arrays, which are returned in the form of pointers to the row data
		NOT OneDArray objects.
     */	
	inline const element_type& operator[](const int pos) const {return array_of_rows[pos];}

    //! Returns the raw data [deprecated]	
	inline T**& data(){return array_of_rows;}

    //! Returns the width if n=0, the height if n=1, 0 otherwise.	
	int length(const int n) const {
		if (n==0) return l0; 
		else if (n==1) return l1;
		else return 0;}

    //! Returns the index of the first element of a row/column (n=0/1) or 0 (n>1)	
	int first(const int n) const {
		if (n==0) return first0;
		else if (n==1) return first1; 
		else return 0;}

    //! Returns the index of the first element of a row/column (n=0/1) or 0 (n>1), Blitz style.	
	int lbound(const int n) const {return first(n);}

    //! Returns the index of the last element of a row/column (n=0/1) or 0 (n>1)	
	int last(const int n) const {
		if (n==0) return last0;
		else if (n==1) return last1; 
		else return -1;}

    //! Returns the index of the last element of a row/column (n=0/1) or 0 (n>1), Blitz style.	
	int ubound(const int n) const {return last(n);}	

private:
	void Init(int len0,int len1);
	void FreeData();	

	std::allocator<element_type> alloc;	
	int first0,first1,last0,last1;
	int l0,l1;
	element_type* array_of_rows;
};

//public member functions//
///////////////////////////

template <class T>
TwoDArray<T>::TwoDArray(const TwoDArray<T>& Cpy){
	first0=Cpy.first0;
	first1=Cpy.first1;		
	last0=Cpy.last0;
	last1=Cpy.last1;		
	l0=last0-first0+1;
	l1=last1-first1+1;		
	if (first0==0 && first1==0)		
		Init(l0,l1);
	else{
			//based 2D arrays not yet supported	
	}
	for (int J=0;J<l1;++J){
		for (int I=0;I<l0;++I){
			*(array_of_rows[J]+I)=*((Cpy.array_of_rows)[J]+I);	
		}				
	}
}

template <class T>
TwoDArray<T>& TwoDArray<T>::operator=(const TwoDArray<T>& rhs){
	if (&rhs!=this){
		FreeData();

		first0 = rhs.first0;
		first1 = rhs.first1;			

		last0 = rhs.last0;
		last1 = rhs.last1;
		l0 = last0-first0+1;
		l1 = last1-first1+1;		
		if (first0 == 0 && first1 == 0)
			Init(l0,l1);
		else{
				//based 2D arrays not yet supported
		}
		for (int J=0;J<l1;++J){
			for (int I=0;I<l0;++I){
				*(array_of_rows[J]+I)=*((rhs.array_of_rows)[J]+I);	
			}				
		}
	}
	return *this;
}

template <class T>
void TwoDArray<T>::resize(int len0, int len1){
	FreeData();
	Init(len0,len1);
}

//private member functions//
////////////////////////////

template <class T>
void TwoDArray<T>::Init(int len0,int len1){
	l0=len0; l1=len1;
	first0=0;first1=0;
	last0=l0-1;last1=l1-1;
	if (l1>0)
	{
		array_of_rows=alloc.allocate(l1);
		if (l0>0){
			for (int J=0;J<l1;++J){
				array_of_rows[J]=new T[l0];
			}
		}
		else{
			l0=0;
			first0=0;
			last0=-1;
		}
	}
	else {
		l0=0;l1=0;
		first0=0;first1=0;
		last0=-1;last1=-1;
	}
}

template <class T>
void TwoDArray<T>::FreeData(){
	if (l1>0)
	{
		if (l0>0) 
		{
			for (int J=0 ; J<l1 ; ++J)
			{
				delete[] array_of_rows[J];
			}//J
		}
		int I=l1;
		while (I!=0)
			alloc.destroy(&array_of_rows[--I]);
		alloc.deallocate(array_of_rows,l1);
	}	
}

#endif
