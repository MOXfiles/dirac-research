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

#ifndef _FRAME_H_
#define _FRAME_H_

//pic_file stuff
#include "common.h"


//! 
/*!

 */
class Frame{

public:


    //! 
    /*!
        
     */
	//constructors and destructors
	Frame(FrameParams fp): fparams(fp),Y_data(0),U_data(0),V_data(0),upY_data(0),upU_data(0),upV_data(0){Init();}

    //! 
    /*!
        
     */
	virtual ~Frame();


    //! 
    /*!
        
     */
	//initialisation/reinitialisation
	void Init();


    //! 
    /*!
        
     */
	//gets and sets
	FrameParams& GetFparams() {return fparams;}

    //! 
    /*!
        
     */
	void SetFrameSort(FrameSort fsort){fparams.fsort=fsort;}

    //! 
    /*!
        
     */
	PicArray& Ydata(){return *Y_data;}//get the

    //! 
    /*!
        
     */
	PicArray& Udata(){return *U_data;}//various

    //! 
    /*!
        
     */
	PicArray& Vdata(){return *V_data;}//components

    //! 
    /*!
        
     */
	PicArray& Data(CompSort cs){
		if (cs==U) return *U_data; 
		else if (cs==V) return *V_data; 
		else return *Y_data;}	

    //! 
    /*!
        
     */
	void SetY(PicArray& in_array);//set the

    //! 
    /*!
        
     */
	void SetU(PicArray& in_array);//various

    //! 
    /*!
        
     */
	void SetV(PicArray& in_array);//components


    //! 
    /*!
        
     */
	PicArray& UpData(CompSort cs){
		if (cs==U) return UpUdata(); 
		else if (cs==V) return UpVdata(); 
		else return UpYdata();}	

    //! 
    /*!
        
     */
	PicArray& UpYdata();//get upconverted

    //! 
    /*!
        
     */
	PicArray& UpUdata();//versions of the

    //! 
    /*!
        
     */
	PicArray& UpVdata();//various components


    //! 
    /*!
        
     */
	//other functions
	void Clip();//clip the data in the frame
private:
	FrameParams fparams;
	PicArray* Y_data;
	PicArray* U_data;
	PicArray* V_data;		
	PicArray* upY_data;
	PicArray* upU_data;
	PicArray* upV_data;		
	void ClearData();//delete all the data.Called by constructor and also by Init()	
	void ClipComponent(PicArray& pic_data);	
};


#endif
