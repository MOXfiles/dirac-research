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
* Revision 1.8  2004-05-19 14:04:44  tjdwave
* Changed YUV output to output lines instead of bytes, according to patch provided by Malcolm Parsons
*
* Revision 1.7  2004/05/19 09:16:49  tjdwave
* Replaced zero-padding with edge-padding to eliminate colour-fringeing at low bitrates. Mod to set padded values to 0 when compensating frames.
*
* Revision 1.5  2004/05/14 17:25:43  stuart_hc
* Replaced binary header files with ASCII text format to achieve cross-platform interoperability.
* Rearranged PicOutput constructor to permit code reuse from picheader/headmain.cpp
*
* Revision 1.4  2004/05/12 08:35:34  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.3  2004/04/12 01:57:46  chaoticcoyote
* Fixed problem Intel C++ had in finding xparam headers on Linux
* Solved Segmentation Fault bug in pic_io.cpp
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
* Revision 0.1.0  2004/02/20 09:36:09  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#include "libdirac_common/pic_io.h"


/**************************************Output***********************************/

PicOutput::PicOutput(const char* output_name, const SeqParams& sp, bool write_header_only)
: sparams(sp)
{
	op_head_ptr = NULL;
	op_pic_ptr = NULL;

	OpenHeader(output_name);
	if (! write_header_only)
		OpenYUV(output_name);
}

void PicOutput::OpenHeader(const char* output_name)
{
	char output_name_hdr[FILENAME_MAX];

	strncpy(output_name_hdr, output_name, sizeof(output_name_hdr));
	strcat(output_name_hdr, ".hdr");

	//header output
	op_head_ptr = new std::ofstream(output_name_hdr,std::ios::out | std::ios::binary);

	if (!(*op_head_ptr))
		std::cerr<<std::endl<<"Can't open output header file for output: "<<output_name_hdr<<std::endl;
}

void PicOutput::OpenYUV(const char* output_name)
{
	char output_name_yuv[FILENAME_MAX];

	strncpy(output_name_yuv,output_name, sizeof(output_name_yuv));
	strcat(output_name_yuv,".yuv");

	//picture output
	op_pic_ptr = new std::ofstream(output_name_yuv,std::ios::out | std::ios::binary);

	if (!(*op_pic_ptr))
		std::cerr<<std::endl<<"Can't open output picture data file for output: "<<output_name_yuv<<std::endl;
}

PicOutput::~PicOutput()
{
	if (op_head_ptr && *op_head_ptr)
	{
		op_head_ptr->close();
		delete op_head_ptr;
	}
	if (op_pic_ptr && *op_pic_ptr)
	{
		op_pic_ptr->close();
		delete op_pic_ptr;
	}
}

//write a human-readable picture header as separate file
void PicOutput::WritePicHeader()
{
	if (!op_head_ptr || !*op_head_ptr)
		return;

// The obsolete header format will disappear in a future release
#ifdef OBSOLETE_HEADER_FMT
	int head_data[7];

	//Write the chroma format
	head_data[0]=sparams.cformat;

	//Y component breadth and height
	head_data[1]=sparams.xl;
	head_data[2]=sparams.yl;

	//Number of frames
	head_data[3]=sparams.zl;

	//interlaced or not
	head_data[4]=sparams.interlace;

	//top-field first or not (only relevant if interlaced)
	head_data[5]=sparams.topfieldfirst;

	//frame-rate code (needed for display)
	head_data[6]=sparams.framerate;

	op_head_ptr->write((char*) &head_data,sizeof head_data);
#else
	*op_head_ptr << sparams.cformat << std::endl;
	*op_head_ptr << sparams.xl << std::endl;
	*op_head_ptr << sparams.yl << std::endl;
	*op_head_ptr << sparams.zl << std::endl;
	*op_head_ptr << sparams.interlace << std::endl;
	*op_head_ptr << sparams.topfieldfirst << std::endl;
	*op_head_ptr << sparams.framerate << std::endl;
#endif
}

void PicOutput::WriteNextFrame(const Frame& myframe)
{
	WriteComponent(myframe.Ydata(),Y);
	if (sparams.cformat!=Yonly){
		WriteComponent(myframe.Udata(),U);
		WriteComponent(myframe.Vdata(),V);
	}
}

void PicOutput::WriteComponent(const PicArray& pic_data,const CompSort& cs)
{
	//initially set up for 10-bit data input, rounded to 8 bits on file output
	//This will throw out any padding to the right and bottom of a frame

	int xl,yl;
	if (cs==Y){
		xl=sparams.xl;
		yl=sparams.yl;
	}
	else{
		if (sparams.cformat==format411){
			xl=sparams.xl/4;
			yl=sparams.yl;
		}
		else if (sparams.cformat==format420){
			xl=sparams.xl/2;
			yl=sparams.yl/2;
		}
		else if (sparams.cformat==format422){
			xl=sparams.xl/2;
			yl=sparams.yl;
		}
		else{
			xl=sparams.xl;
			yl=sparams.yl;
		}
	}

	unsigned char* tempc=new unsigned char[xl];
	ValueType tempv;

	if (*op_pic_ptr){
		for (int J=0;J<yl;++J){
			for (int I=0;I<xl;++I){
				tempv=pic_data[J][I]+2;
				tempv>>=2;
				tempc[I]=(unsigned char) tempv;
			}//I
			op_pic_ptr->write((char*) tempc,xl);
		}//J
	}
	else
		std::cerr<<std::endl<<"Can't open picture data file for writing";

	delete[] tempc;
}

/**************************************Input***********************************/

PicInput::PicInput(const char* input_name):
xpad(0),
ypad(0)
{

	char input_name_yuv[FILENAME_MAX];
	char input_name_hdr[FILENAME_MAX];

	strncpy(input_name_yuv, input_name, sizeof(input_name_yuv));
	strncpy(input_name_hdr, input_name, sizeof(input_name_hdr));
	strcat(input_name_yuv, ".yuv");
	strcat(input_name_hdr, ".hdr");

	ip_head_ptr = new std::ifstream(input_name_hdr,std::ios::in | std::ios::binary);	//header output
	ip_pic_ptr = new std::ifstream(input_name_yuv,std::ios::in | std::ios::binary);	//picture output

	if (!(*ip_head_ptr))
		std::cerr<<std::endl<<"Can't open input header file: "<<input_name_hdr<<std::endl;
	if (!(*ip_pic_ptr))
		std::cerr<<std::endl<<"Can't open input picture data file: "<<input_name_yuv<<std::endl;
}

PicInput::~PicInput()
{
	ip_pic_ptr->close();
	ip_head_ptr->close();
	delete ip_pic_ptr;
	delete ip_head_ptr;
}

void PicInput::SetPadding(const int xpd, const int ypd)
{
	xpad=xpd;
	ypad=ypd;
}

void PicInput::ReadNextFrame(Frame& myframe)
{
	ReadComponent(myframe.Ydata(),Y);
	if (sparams.cformat!=Yonly){
		ReadComponent(myframe.Udata(),U);
		ReadComponent(myframe.Vdata(),V);
	}
}

//read a picture header from a separate file
void PicInput::ReadPicHeader()
{
	if (! *ip_head_ptr)
		return;

// The obsolete header format will disappear in a future release
#ifdef OBSOLETE_HEADER_FMT
	int head_data[7];
	ip_head_ptr->read((char*) &head_data,sizeof head_data);
	sparams.cformat=ChromaFormat(head_data[0]);
	sparams.xl=head_data[1];
	sparams.yl=head_data[2];
	sparams.zl=head_data[3];
	sparams.interlace=bool(head_data[4]);
	sparams.topfieldfirst=bool(head_data[5]);
	sparams.framerate=head_data[6];
#else
	int		int_cformat;
	*ip_head_ptr >> int_cformat;
	sparams.cformat = (ChromaFormat)int_cformat;
	*ip_head_ptr >> sparams.xl >> sparams.yl >> sparams.zl;
	*ip_head_ptr >> sparams.interlace >> sparams.topfieldfirst >> sparams.framerate;
#endif
}

bool PicInput::End() const
{
	return ip_pic_ptr->eof();
}

void PicInput::ReadComponent(PicArray& pic_data, const CompSort& cs)
{
	if (! *ip_pic_ptr)
	{
		std::cerr<<"Can't read component from picture data"<<std::endl;
		return;
	}

	//initially set up for 8-bit file input expanded to 10 bits for array output

	int xl,yl;
	if (cs==Y){
		xl=sparams.xl;
		yl=sparams.yl;
	}
	else{
		if (sparams.cformat==format411){
			xl=sparams.xl/4;
			yl=sparams.yl;
		}
		else if (sparams.cformat==format420){
			xl=sparams.xl/2;
			yl=sparams.yl/2;
		}
		else if (sparams.cformat==format422){
			xl=sparams.xl/2;
			yl=sparams.yl;
		}
		else{
			xl=sparams.xl;
			yl=sparams.yl;
		}
	}

	unsigned char temp[xl];//array big enough for one line
	for (int J=0;J<yl;++J){
		ip_pic_ptr->read((char*) &temp, sizeof(temp));
		for (int I=0;I<xl;++I){
			pic_data[J][I]=(ValueType) temp[I];
			pic_data[J][I]<<=2;
		}//I
		//pad the columns on the rhs using the edge value
		for (int I=xl;I<pic_data.length(0);++I){
			pic_data[J][I]=pic_data[J][xl-1];
		}//I
	}//J
	//now do the padded lines, using the last true line
	for (int J=yl;J<pic_data.length(1);++J){
		for (int I=0;I<pic_data.length(0);++I)
			pic_data[J][I]=pic_data[yl-1][I];
	}//J
}
