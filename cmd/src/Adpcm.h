// ADPCM.h : Decompresses ADPCM waveforms
//

#pragma once

// Mono decompression parameters
struct SAdpcmMonoParam
{
	unsigned char* InputBuffer;
	unsigned long InputLength;
	short* OutputBuffer;

	short FirstSample;
	unsigned char FirstIndex;
};

// Stereo decompression parameters
struct SAdpcmStereoParam
{
	unsigned char* InputBuffer;
	unsigned long InputLength;
	short* OutputBuffer;

	short FirstLeftSample;
	short FirstRightSample;
	unsigned char FirstLeftIndex;
	unsigned char FirstRightIndex;
};

// Decompress mono waveforms
bool DecompressMonoAdpcm(SAdpcmMonoParam* Param);

// Decompress stereo waveforms
bool DecompressStereoAdpcm(SAdpcmStereoParam* Param);
