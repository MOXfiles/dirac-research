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
* Revision 1.2  2004-04-06 18:06:53  chaoticcoyote
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

#ifndef _ARRAYS_H_
#define _ARRAYS_H_

//basic array types used for pictures etc

#include <memory>
#include <cstddef>
#include <stdexcept>
#include <iostream>

typedef short ValueType;
typedef int CalcValueType;


//! 
/*!

 */
class range{
public:

    //! 
    /*!

     */
	range(int s, int e):fst(s),lst(e){}

    //! 
    /*!

     */
	int& first(){return fst;}

    //! 
    /*!

     */
	int& last(){return lst;}
private:
	int fst,lst;
};

//////////////////////////////
//One-Dimensional Array type//
//////////////////////////////


//! 
/*!

 */
template <class T> class OneDArray{
public:

    //! 
    /*!

     */
	OneDArray(){init(0);}

    //! 
    /*!

     */
	OneDArray(int len){init(len);}

    //! 
    /*!

     */
	OneDArray(range r){init(r);}

    //! 
    /*!

     */
	~OneDArray(){
		free_ptr();
	}

    //! 
    /*!

     */
	OneDArray(const OneDArray<T>& Cpy);

    //! 
    /*!

     */
	OneDArray<T>& operator=(const OneDArray<T>& rhs);	

    //! 
    /*!

     */
	void resize(int l);


    //! 
    /*!

     */
	inline T& operator[](int pos){return ptr[pos-fst];}

    //! 
    /*!

     */
	int length(const int n){if (n==0) return l; else return 0;}

    //! 
    /*!

     */
	int length(){return l;}

    //! 
    /*!

     */
	int first(const int n){if (n==0) return fst; else return 0;}

    //! 
    /*!

     */
	int first(){return fst;}

    //! 
    /*!

     */
	int lbound(const int n){return first(n);}

    //! 
    /*!

     */
	int last(const int n){if (n==0) return lst; else return -1;}

    //! 
    /*!

     */
	int last(){return lst;}

    //! 
    /*!

     */
	int ubound(const int n){return last(n);}	
private:
	void init(int len);
	void init(range r);
	void free_ptr();	

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
		init(l);
	else
		init(range(fst,lst));
	for (int I=0;I<l;++I)
		*(ptr+I)=*(Cpy.ptr+I);
}

template <class T>
OneDArray<T>& OneDArray<T>::operator=(const OneDArray<T>& rhs){
	if (&rhs!=this){
		free_ptr();
		fst=rhs.fst;
		lst=rhs.lst;
		l=rhs.l;			
		if (fst==0)
			init(l);
		else
			init(range(fst,lst));
		for (int I=0;I<l;++I)
			*(ptr+I)=*(rhs.ptr+I);			

	}
	return *this;
}

template <class T> 
void OneDArray<T>::resize(int l){	
	free_ptr();
	init(l);
}

//private member functions//
////////////////////////////

template <class T>
void OneDArray<T>::init(int len){
	l=len;
	fst=0;
	lst=l-1;
	if (l>0){
		//ptr=new T[l];
		ptr=alloc.allocate(l);
	}
	else {
		l=0;
		fst=0;
		lst=-1;
	}
}		

template <class T>
void OneDArray<T>::init(range r){
	T tmp_val;
	fst=r.first();
	lst=r.last();
	l=lst-fst+1; 
	if (l>0){
		//ptr=new T[l];
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
void OneDArray<T>::free_ptr(){
	if (l>0){
		//delete ptr;
		int I=l;
		while (I!=0)
			alloc.destroy(&ptr[--I]);
		alloc.deallocate(ptr,l);		

	}
}


//////////////////////////////
//Two-Dimensional Array type//
//////////////////////////////


//! 
/*!

 */
template <class T> class TwoDArray{
	typedef T* element_type;
public:

    //! 
    /*!

     */
	TwoDArray(){init(0,0);}

    //! 
    /*!

     */
	TwoDArray(int len0,int len1){init(len0,len1);}

    //! 
    /*!

     */
	virtual ~TwoDArray(){
		free_data();	
	}

    //! 
    /*!

     */
	TwoDArray(const TwoDArray<T>& Cpy);

    //! 
    /*!

     */
	TwoDArray<T>& operator=(const TwoDArray<T>& rhs);

    //! 
    /*!

     */
	void resize(int len0, int len1);	


    //! 
    /*!

     */
	inline element_type& operator[](const int pos){return array_of_rows[pos];}

    //! 
    /*!

     */
	inline T**& data(){return array_of_rows;}

    //! 
    /*!

     */
	int length(const int n){
		if (n==0) return l0; 
		else if (n==1) return l1;
		else return 0;}

    //! 
    /*!

     */
	int first(const int n){
		if (n==0) return first0;
		else if (n==1) return first1; 
		else return 0;}

    //! 
    /*!

     */
	int lbound(const int n){return first(n);}

    //! 
    /*!

     */
	int last(const int n){
		if (n==0) return last0;
		else if (n==1) return last1; 
		else return -1;}

    //! 
    /*!

     */
	int ubound(const int n){return last(n);}	

private:
	void init(int len0,int len1);
	void free_data();	

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
		init(l0,l1);
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
		free_data();
		first0=rhs.first0;
		first1=rhs.first1;			
		last0=rhs.last0;
		last1=rhs.last1;
		l0=last0-first0+1;
		l1=last1-first1+1;		
		if (first0==0 && first1==0)
			init(l0,l1);
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
	free_data();
	init(len0,len1);
}

//private member functions//
////////////////////////////

template <class T>
void TwoDArray<T>::init(int len0,int len1){
	l0=len0; l1=len1;
	first0=0;first1=0;
	last0=l0-1;last1=l1-1;
	if (l1>0){
		//		array_of_rows=new element_type[l1];
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
void TwoDArray<T>::free_data(){
	if (l1>0){
		if (l0>0) {
			for (int J=0;J<l1;++J){
				delete[] array_of_rows[J];
			}
		}
		//		delete array_of_rows;					
		int I=l1;
		while (I!=0)
			alloc.destroy(&array_of_rows[--I]);
		alloc.deallocate(array_of_rows,l1);
	}	
}

#endif
