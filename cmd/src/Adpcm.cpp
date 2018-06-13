// ADPCM.cpp : Decompresses ADPCM waveforms
//

#include "Pch.h"
#include "Adpcm.h"

// The index table
static int IndexTable[]={-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

// The step table
static int StepTable[]={
7, 8, 9, 10, 11, 12, 13, 14,
16, 17, 19, 21, 23, 25, 28, 31,
34, 37, 41, 45, 50, 55, 60, 66,
73, 80, 88, 97, 107, 118, 130, 143,
157, 173, 190, 209, 230, 253, 279, 307,
337, 371, 408, 449, 494, 544, 598, 658,
724, 796, 876, 963, 1060, 1166, 1282, 1411,
1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
32767};

// Decompress mono waveforms
bool DecompressMonoAdpcm(SAdpcmMonoParam* Param)
{
	// Error checking
	if(Param==NULL || Param->InputBuffer==NULL || Param->OutputBuffer==NULL || Param->InputLength==0)
	{
		return false;
	}

	// Set up some variables
	unsigned char* Input=Param->InputBuffer;
	short* Output=Param->OutputBuffer;
	int LastSample=Param->FirstSample;
	int LastIndex=Param->FirstIndex;
	int LastStep=StepTable[LastIndex];

	// Loop through the data decompressing it
	for(unsigned int i=0;i<Param->InputLength;i++)
	{
		unsigned char Code;
		int Difference;
		int CurrentSample;

		// First
		Code=Input[i]>>4;
		LastIndex=LastIndex+IndexTable[Code];
		if(LastIndex>88)
		{
			LastIndex=88;
		}
		else if(LastIndex<0)
		{
			LastIndex=0;
		}
		Difference=(((Code&0x07)*2+1)*LastStep) >> 3;
		if(Code&0x08)
		{
			Difference=-Difference;
		}
		CurrentSample=LastSample+Difference;
		if(CurrentSample>32767)
		{
			CurrentSample=32767;
		}
		else if(CurrentSample<-32768)
		{
			CurrentSample=-32768;
		}
		*Output=(short)CurrentSample;
		Output++;
		LastSample=CurrentSample;
		LastStep=StepTable[LastIndex];

		// Second
		Code=Input[i]&0x0F;
		LastIndex=LastIndex+IndexTable[Code];
		if(LastIndex>88)
		{
			LastIndex=88;
		}
		else if(LastIndex<0)
		{
			LastIndex=0;
		}
		Difference=(((Code&0x07)*2+1)*LastStep) >> 3;
		if(Code&0x08)
		{
			Difference=-Difference;
		}
		CurrentSample=LastSample+Difference;
		if(CurrentSample>32767)
		{
			CurrentSample=32767;
		}
		else if(CurrentSample<-32768)
		{
			CurrentSample=-32768;
		}
		*Output=(short)CurrentSample;
		Output++;
		LastSample=CurrentSample;
		LastStep=StepTable[LastIndex];
	}

	// Save some variables
	Param->FirstSample=(short)LastSample;
	Param->FirstIndex=(unsigned char)LastIndex;
	return true;
}

// Decompress stereo waveforms
bool DecompressStereoAdpcm(SAdpcmStereoParam* Param)
{
	// Error checking
	if(Param==NULL || Param->InputBuffer==NULL || Param->OutputBuffer==NULL || Param->InputLength==0)
	{
		return false;
	}

	// Set up some variables
	unsigned char* Input=Param->InputBuffer;
	short* Output=Param->OutputBuffer;
	int LastLeftSample=Param->FirstLeftSample;
	int LastRightSample=Param->FirstRightSample;
	int LastLeftIndex=Param->FirstLeftIndex;
	int LastRightIndex=Param->FirstRightIndex;
	int LastLeftStep=StepTable[LastLeftIndex];
	int LastRightStep=StepTable[LastRightIndex];

	// Loop through the data decompressing it
	for(unsigned int i=0;i<Param->InputLength;i++)
	{
		unsigned char Code;
		int Difference;
		int CurrentSample;

		// Left
		Code=Input[i]>>4;
		LastLeftIndex=LastLeftIndex+IndexTable[Code];
		if(LastLeftIndex>88)
		{
			LastLeftIndex=88;
		}
		else if(LastLeftIndex<0)
		{
			LastLeftIndex=0;
		}
		Difference=(((Code&0x07)*2+1)*LastLeftStep)>>3;
		if(Code&0x08)
		{
			Difference=-Difference;
		}
		CurrentSample=LastLeftSample+Difference;
		if(CurrentSample>32767)
		{
			CurrentSample=32767;
		}
		else if(CurrentSample<-32768)
		{
			CurrentSample=-32768;
		}
		*Output=(short)CurrentSample;
		Output++;
		LastLeftSample=CurrentSample;
		LastLeftStep=StepTable[LastLeftIndex];

		// Right
		Code=Input[i]&0x0F;
		LastRightIndex=LastRightIndex+IndexTable[Code];
		if(LastRightIndex>88)
		{
			LastRightIndex=88;
		}
		else if(LastRightIndex<0)
		{
			LastRightIndex=0;
		}
		Difference=(((Code&0x07)*2+1)*LastRightStep)>>3;
		if(Code&0x08)
		{
			Difference=-Difference;
		}
		CurrentSample=LastRightSample+Difference;
		if(CurrentSample>32767)
		{
			CurrentSample=32767;
		}
		else if(CurrentSample<-32768)
		{
			CurrentSample=-32768;
		}
		*Output=(short)CurrentSample;
		Output++;
		LastRightSample=CurrentSample;
		LastRightStep=StepTable[LastRightIndex];
	}

	// Save some variables
	Param->FirstLeftSample=(short)LastLeftSample;
	Param->FirstRightSample=(short)LastRightSample;
	Param->FirstLeftIndex=(unsigned char)LastLeftIndex;
	Param->FirstRightIndex=(unsigned char)LastRightIndex;
	return true;
}
