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
* Revision 1.3  2004-05-12 08:35:34  tjdwave
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
* Downconversion by Richard Felton, BBC Research and Development.
*/

#include "libdirac_motionest/downconvert.h"

DownConverter::DownConverter(){
}


//General function - does some admin and calls the correct function
void DownConverter::DoDownConvert(const PicArray& old_data, PicArray& new_data){
	//Down-convert by a factor of two.
	row_buffer= new ValueType[old_data.length(0)];	
		//Variables that will be used by the filter calculations
	int sum;
	int colpos;

	//There are three y loops to cope with the leading edge, middle 
	//and trailing edge of each column.
	colpos=0;
	for(int y = 0; y < Stage_I_Size*2; y+=2,colpos++){
		//We are filtering each column but doing it bit by bit.
		//This means our main loop is in the x direction and
		//there is a much greater chance the data we need will
		//be in the cache.
		for(int x = 0; x < old_data.length(0); x++){			
			//In down conversion we interpolate every pixel
			//so there is no copying.
			//Excuse the complicated ternary stuff but it sorts out the edge
			sum =  (old_data[y][x] + old_data[y+1][x])*StageI_I;
			sum += (old_data[((y-1)>=0)?(y-1):0][x] + old_data[y+2][x])*StageI_II;
			sum += (old_data[((y-2)>=0)?(y-2):0][x] + old_data[y+3][x])*StageI_III;
			sum += (old_data[((y-3)>=0)?(y-3):0][x] + old_data[y+4][x])*StageI_IV;
			sum += (old_data[((y-4)>=0)?(y-4):0][x] + old_data[y+5][x])*StageI_V;
			sum += (old_data[((y-5)>=0)?(y-5):0][x] + old_data[y+6][x])*StageI_VI;
			sum += 1<<(StageI_Shift-1);//do rounding right
			row_buffer[x] = sum >> StageI_Shift;
		}
		//Speaking of which - the row loop.
		RowLoop(colpos,old_data,new_data);
	}

	//This loop is like the last one but it deals with the center
	//section of the image and so the ternary operations are dropped
	//from the filter section.
	for(int y = Stage_I_Size*2; y < old_data.length(1) - Stage_I_Size*2; y+=2,colpos++){

		for(int x = 0; x < old_data.length(0); x++){

			sum =  (old_data[y][x]   + old_data[y+1][x])*StageI_I;
			sum += (old_data[y-1][x] + old_data[y+2][x])*StageI_II;
			sum += (old_data[y-2][x] + old_data[y+3][x])*StageI_III;
			sum += (old_data[y-3][x] + old_data[y+4][x])*StageI_IV;
			sum += (old_data[y-4][x] + old_data[y+5][x])*StageI_V;
			sum += (old_data[y-5][x] + old_data[y+6][x])*StageI_VI;
			sum += 1<<(StageI_Shift-1);//do rounding right
			row_buffer[x] = sum >> StageI_Shift;
		}
		RowLoop(colpos,old_data,new_data);
	}

	//Another similar loop! - this time we are dealing with
	//the trailing edge so the ternary stuff is back in the
	//filter calcs but in the second parameter.
	int yOld=old_data.length(1);	
	for(int y = old_data.length(1) - (Stage_I_Size*2); y < old_data.length(1); y+=2,colpos++){
		for(int x = 0; x < old_data.length(0); x++){

			sum =  (old_data[y][x]   + old_data[((y+1)<yOld)?(y+1):(yOld-1)][x])*StageI_I;
			sum += (old_data[y-1][x] + old_data[((y+2)<yOld)?(y+2):(yOld-1)][x])*StageI_II;
			sum += (old_data[y-2][x] + old_data[((y+3)<yOld)?(y+3):(yOld-1)][x])*StageI_III;
			sum += (old_data[y-3][x] + old_data[((y+4)<yOld)?(y+4):(yOld-1)][x])*StageI_IV;
			sum += (old_data[y-4][x] + old_data[((y+5)<yOld)?(y+5):(yOld-1)][x])*StageI_V;
			sum += (old_data[y-5][x] + old_data[((y+6)<yOld)?(y+6):(yOld-1)][x])*StageI_VI;
			sum += 1<<(StageI_Shift-1);//do rounding right
			row_buffer[x] = sum >> StageI_Shift;
		}
		RowLoop(colpos,old_data,new_data);
	}
	delete[] row_buffer;
}


//The loop over the columns is the same every time so lends itself to isolation
//as an individual function.
void DownConverter::RowLoop(int &colpos,const PicArray& old_data,PicArray& new_data){

 	//Calculation variables
	int sum;
	int xOld=old_data.length(0);
	int linepos=0;	

 	//Leading Column Edge
 	//Similar loops to the x case in ByHalf_opto, for explanation look there.
 	//Note the factor of two difference as we only want to fill in every other
 	//line as the others have already been created by the line loops.

	for(int x = 0; x < (2*Stage_I_Size); x+=2,linepos++){
		sum =  (row_buffer[((x)>=0)?(x):0]     + row_buffer[x+1])*StageI_I;
		sum += (row_buffer[((x-1)>=0)?(x-1):0] + row_buffer[x+2])*StageI_II;
		sum += (row_buffer[((x-2)>=0)?(x-2):0] + row_buffer[x+3])*StageI_III;
		sum += (row_buffer[((x-3)>=0)?(x-3):0] + row_buffer[x+4])*StageI_IV;
		sum += (row_buffer[((x-4)>=0)?(x-4):0] + row_buffer[x+5])*StageI_V;
		sum += (row_buffer[((x-5)>=0)?(x-5):0] + row_buffer[x+6])*StageI_VI;
		sum += 1<<(StageI_Shift-1);//do rounding right
		new_data[colpos][linepos] = sum >> StageI_Shift;
	}
 	//Middle of column
	for(int x = (2*Stage_I_Size); x < xOld - (2*Stage_I_Size); x+=2,linepos++){
		sum =  (row_buffer[x]   + row_buffer[x+1])*StageI_I;
		sum += (row_buffer[x-1] + row_buffer[x+2])*StageI_II;
		sum += (row_buffer[x-2] + row_buffer[x+3])*StageI_III;
		sum += (row_buffer[x-3] + row_buffer[x+4])*StageI_IV;
		sum += (row_buffer[x-4] + row_buffer[x+5])*StageI_V;
		sum += (row_buffer[x-5] + row_buffer[x+6])*StageI_VI;
		sum += 1<<(StageI_Shift-1);//do rounding right
		new_data[colpos][linepos] = sum >> StageI_Shift;
	}
 	//Trailing column edge
	for(int x = xOld - (2*Stage_I_Size); x< xOld; x+=2,linepos++){
		sum =  (row_buffer[x]   + row_buffer[((x+1)<xOld)?(x+1):(xOld-1)])*StageI_I;
		sum += (row_buffer[x-1] + row_buffer[((x+2)<xOld)?(x+2):(xOld-1)])*StageI_II;
		sum += (row_buffer[x-2] + row_buffer[((x+3)<xOld)?(x+3):(xOld-1)])*StageI_III;
		sum += (row_buffer[x-3] + row_buffer[((x+4)<xOld)?(x+4):(xOld-1)])*StageI_IV;
		sum += (row_buffer[x-4] + row_buffer[((x+5)<xOld)?(x+5):(xOld-1)])*StageI_V;
		sum += (row_buffer[x-5] + row_buffer[((x+6)<xOld)?(x+6):(xOld-1)])*StageI_VI;
		sum += 1<<(StageI_Shift-1);//do rounding right
		new_data[colpos][linepos] = sum >> StageI_Shift;
	}
}
