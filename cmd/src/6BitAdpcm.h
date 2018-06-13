// 6BitAdpcm.h : Decompresses UbiSoft's 6-bit ADPCM
//

#pragma once


struct S6BitAdpcmBlockHeader
{
	uint32_t Signature;
	uint32_t LastIndex1;
	uint32_t Unknown2;
	uint32_t Unknown3;
	uint32_t Unknown4;
	uint32_t Unknown5;
	uint32_t Unknown6;
	uint32_t Unknown7;
	uint32_t LastSample;
	uint32_t Unknown9;
	uint32_t LastIndex2;
	uint32_t Unknown11;
	uint32_t Unknown12;
};

struct S4BitAdpcmBlockHeader
{
	uint32_t Signature;
	uint32_t Unknown1;
	uint32_t Unknown2;
	uint32_t Unknown3;
	uint32_t Unknown4;
	uint32_t Unknown5;
	uint32_t Unknown6;
	uint32_t Unknown7;
	uint32_t Unknown8;
	uint32_t Unknown9;
	uint32_t Unknown10;
	uint32_t Unknown11;
	uint32_t Unknown12;
};

// Get the number of samples in a specified number of bytes 6-bit
unsigned long Get6BitAdpcmSamples(unsigned long Bytes);

// Get the number of samples in a specified number of bytes 4-bit
unsigned long Get4BitAdpcmSamples(unsigned long Bytes);

// Expand a 6-bit block
void Expand6BitAdpcmBlock(void* Source, unsigned long* Dest, unsigned long Count);

// Expand a 4-bit block
void Expand4BitAdpcmBlock(void* Source, unsigned long* Dest, unsigned long Count);

// Decompress a stereo 6-bit block
void DecompressStereo6BitAdpcmBlock(S6BitAdpcmBlockHeader& Left, \
									S6BitAdpcmBlockHeader& Right, \
									unsigned long* Expanded, short* Output, \
									unsigned long SampleCount);

// Decompress a mono 6-bit block
void DecompressMono6BitAdpcmBlock(S6BitAdpcmBlockHeader& Header, \
								  unsigned long* Expanded, short* Output, \
								  unsigned long SampleCount);

// Decompress a stereo 4-bit block
void DecompressStereo4BitAdpcmBlock(S4BitAdpcmBlockHeader& Left, \
									S4BitAdpcmBlockHeader& Right, \
									unsigned long* Expanded, short* Output, \
									unsigned long SampleCount);

// Decompress a mono 4-bit block
void DecompressMono4BitAdpcmBlock(S4BitAdpcmBlockHeader& Header, \
								  unsigned long* Expanded, short* Output, \
								  unsigned long SampleCount);
