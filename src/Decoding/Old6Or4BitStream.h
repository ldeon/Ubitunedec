// Old6Or4BitStream.h : UbiSoft Old 6-Or-4 Bit audio stream (XIII and SC1 PC) decoding
//

#pragma once
#include "StreamHelper.h"

#include "6BitAdpcm.h"

//class CBufferDataStream;

// Provides UbiSoft Old 6-Or-4 Bit audio stream decoding
class COld6Or4BitStream : public CStreamHelper
{
protected:
	// TODO: pragma pack?
	struct SFileHeader
	{
		uint32_t Signature;
		uint32_t SampleCount;
		uint32_t Unknown1;
		uint32_t Unknown2;
		uint32_t BlockSize;
		uint32_t Unknown4;
		uint32_t SampleRate; // Maybe
		uint32_t Unknown5;
		uint32_t Unknown6;
		uint32_t BitsPerSample;
		uint32_t Unknown7;
		uint32_t Channels;
	};

protected:
	SFileHeader m_Header;
	unsigned long m_SampleRate;
	unsigned char m_Channels;
	unsigned long m_ByteBlockSize;
	unsigned long m_SamplesLeft;
	unsigned long* m_ExpandedBuffer;

	S4BitAdpcmBlockHeader m_PersistHeader;

protected:
	virtual bool DoDecodeBlock();

public:
	COld6Or4BitStream(CDataStream* Input);
	virtual ~COld6Or4BitStream();

	virtual bool InitializeHeader();
	virtual bool InitializeHeader(unsigned long SampleRate);
	virtual unsigned long GetSampleRate() const;
	virtual unsigned char GetChannels() const;
	virtual std::string GetFormatName() const;
	virtual unsigned char GetBitsPerSample() const;

};
