// RawCompressedStream.h : UbiSoft raw compressed audio stream decoding
//

#pragma once
#include "StreamHelper.h"

// Provides UbiSoft raw compressed audio stream decoding
class CRawCompressedStream : public CStreamHelper
{
protected:
	bool m_Stereo;
	short m_LeftSample;
	short m_RightSample;
	unsigned char m_LeftIndex;
	unsigned char m_RightIndex;

protected:
	virtual bool DoDecodeBlock();

public:
	CRawCompressedStream(CDataStream* Input);
	virtual ~CRawCompressedStream();

	virtual bool InitializeHeader();
	virtual bool InitializeHeader(unsigned char Channels, short LeftSample=0, unsigned char LeftIndex=0, short RightSample=0, unsigned char RightIndex=0);
	virtual unsigned long GetSampleRate() const;
	virtual unsigned char GetChannels() const;
	virtual std::string GetFormatName() const;
};
