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
* Contributor(s): Richard Felton (Original Author), Thomas Davies
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

#include <libdirac_common/upconvert.h>
#include <iostream>

//Up-convert by a factor of two.
void UpConverter::DoUpConverter(const PicArray &OldImage, PicArray &NewImage)
{

    xOld = OldImage.LengthX();
    yOld = OldImage.LengthY();
    xNew = NewImage.LengthX();
    yNew = NewImage.LengthY();    //assumes NewImage has twice the x and y length of the OldImage. 
                                //TBC: What to do if this is wrong?

    //Variables that will be used by the filter calculations
    int sum;
    int LinePos;
    int ColPos;

    //There are three y loops to cope with the leading edge, middle 
    //and trailing edge of each column.
    ColPos = 0;
    for(int y = 0; y < Stage_I_Size; ++y){
        LinePos = 0;
        //We are filtering each column but doing it bit by bit.
        //This means our main loop is in the x direction and
        //there is a much greater chance the data we need will
        //be in the cache.
        for(int x = 0; x < xOld; x++){

            //Copy a Pixel from the original image.
            NewImage[ColPos][LinePos] = OldImage[y][x];
            ++ColPos;

            //Work out the next pixel from filtered values.
            //Excuse the complicated ternary stuff but it sorts out the edge
            sum =  (OldImage[y][x] + OldImage[y+1][x])*StageI_I;
            sum += (OldImage[((y-1)>=0)?(y-1):0][x] + OldImage[y+2][x])*StageI_II;
            sum += (OldImage[((y-2)>=0)?(y-1):0][x] + OldImage[y+3][x])*StageI_III;
            sum += (OldImage[((y-3)>=0)?(y-1):0][x] + OldImage[y+4][x])*StageI_IV;
            sum += (OldImage[((y-4)>=0)?(y-1):0][x] + OldImage[y+5][x])*StageI_V;
            sum += (OldImage[((y-5)>=0)?(y-1):0][x] + OldImage[y+6][x])*StageI_VI;

            NewImage[ColPos][LinePos] = sum >> Stage_I_Shift;
            --ColPos;

            //Write to every other line so we can now filter each column whilst
            //the data is still in the cache.
            LinePos+=2;
        }
        //Speaking of which - the row loop.
        rowLoop(NewImage, ColPos);
    }
    //This loop is like the last one but it deals with the center
    //section of the image and so the ternary operations are dropped
    //from the filter section.
    for(int y = Stage_I_Size; y < yOld - Stage_I_Size; ++y){
        LinePos = 0;
        for(int x = 0; x < xOld; x++){

            NewImage[ColPos][LinePos]=OldImage[y][x];
            ++ColPos;

            sum =  (OldImage[y][x]   + OldImage[y+1][x])*StageI_I;
            sum += (OldImage[y-1][x] + OldImage[y+2][x])*StageI_II;
            sum += (OldImage[y-2][x] + OldImage[y+3][x])*StageI_III;
            sum += (OldImage[y-3][x] + OldImage[y+4][x])*StageI_IV;
            sum += (OldImage[y-4][x] + OldImage[y+5][x])*StageI_V;
            sum += (OldImage[y-5][x] + OldImage[y+6][x])*StageI_VI;            
            NewImage[ColPos][LinePos] = sum >> Stage_I_Shift;
            --ColPos;
            LinePos+=2;
        }
        rowLoop(NewImage, ColPos);
    }    
    //Another similar loop! - this time we are dealing with
    //the trailing edge so the ternary stuff is back in the
    //filter calcs but in the second parameter.    
    for(int y = yOld - Stage_I_Size; y < yOld; ++y){
        LinePos = 0;
        for(int x = 0; x < xOld; x++){

            NewImage[ColPos][LinePos]=OldImage[y][x];
            ++ColPos;
            sum =  (OldImage[y][x]     + OldImage[((y+1)<yOld)?(y+1):(yOld-1)][x])*StageI_I;
            sum += (OldImage[y - 1][x] + OldImage[((y+2)<yOld)?(y+2):(yOld-1)][x])*StageI_II;
            sum += (OldImage[y - 2][x] + OldImage[((y+3)<yOld)?(y+3):(yOld-1)][x])*StageI_III;
            sum += (OldImage[y - 3][x] + OldImage[((y+4)<yOld)?(y+4):(yOld-1)][x])*StageI_IV;
            sum += (OldImage[y - 4][x] + OldImage[((y+5)<yOld)?(y+5):(yOld-1)][x])*StageI_V;
            sum += (OldImage[y - 5][x] + OldImage[((y+6)<yOld)?(y+6):(yOld-1)][x])*StageI_VI;
            NewImage[ColPos][LinePos] = sum >> Stage_I_Shift;

            --ColPos;
            LinePos+=2;
        }
        rowLoop(NewImage, ColPos);
    }
}


//The loop over the columns is the same every time so lends itself to isolation
//as an individual function.
void UpConverter::rowLoop(PicArray &NewImage, int &ColPos){
    //Calculation variable
    int sum;
    short CP;

    //Leading row Edge
    //Note the factor of two difference as we only want to fill in every other
    //line as the others have already been created
    for(int i = 0; i < 2; ++i){
        for(int x = 0; x < (2*Stage_I_Size); x+=2){
            CP = ColPos+i;
            sum =  (NewImage[CP][((x)>=0)?(x):0]     + NewImage[CP][x+2])*StageI_I;
            sum += (NewImage[CP][((x-2)>=0)?(x-2):0] + NewImage[CP][x+4])*StageI_II;
            sum += (NewImage[CP][((x-4)>=0)?(x-4):0] + NewImage[CP][x+6])*StageI_III;
            sum += (NewImage[CP][((x-6)>=0)?(x-6):0] + NewImage[CP][x+8])*StageI_IV;
            sum += (NewImage[CP][((x-8)>=0)?(x-8):0] + NewImage[CP][x+10])*StageI_V;
            sum += (NewImage[CP][((x-10)>=0)?(x-10):0] + NewImage[CP][x+12])*StageI_VI;
            NewImage[CP][x+1] = sum >> Stage_I_Shift;
        }
        //Middle of column
        for(int x = (2*Stage_I_Size); x < xNew - (2*Stage_I_Size); x+=2){
            sum =  (NewImage[CP][x]   + NewImage[CP][x+2])*StageI_I;
            sum += (NewImage[CP][x-2] + NewImage[CP][x+4])*StageI_II;
            sum += (NewImage[CP][x-4] + NewImage[CP][x+6])*StageI_III;
            sum += (NewImage[CP][x-6] + NewImage[CP][x+8])*StageI_IV;
            sum += (NewImage[CP][x-8] + NewImage[CP][x+10])*StageI_V;
            sum += (NewImage[CP][x-10] + NewImage[CP][x+12])*StageI_VI;

            NewImage[ColPos+i][x+1] = sum >> Stage_I_Shift;
        }
        //Trailing column edge
        for(int x = xNew - (2*Stage_I_Size); x < xNew; x+=2){
            sum =  (NewImage[CP][x]   + NewImage[CP][(((x+2)<xNew)?(x+2):(xNew-2))])*StageI_I;
            sum += (NewImage[CP][x-2] + NewImage[CP][(((x+4)<xNew)?(x+4):(xNew-2))])*StageI_II;
            sum += (NewImage[CP][x-4] + NewImage[CP][(((x+6)<xNew)?(x+6):(xNew-2))])*StageI_III;
            sum += (NewImage[CP][x-6] + NewImage[CP][(((x+8)<xNew)?(x+8):(xNew-2))])*StageI_IV;
            sum += (NewImage[CP][x-8] + NewImage[CP][(((x+10)<xNew)?(x+10):(xNew-2))])*StageI_V;
            sum += (NewImage[CP][x-10] + NewImage[CP][(((x+12)<xNew)?(x+12):(xNew-2))])*StageI_VI;
            NewImage[CP][x+1] = sum >> Stage_I_Shift;
        }
    }
    //Adjust this variable so the x loop has the correct starting point
    //when it runs next.
    ColPos+=2;
}
