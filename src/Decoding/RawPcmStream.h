// RawPcmStream.h : Raw 16-bit audio stream decoding
//

#pragma once
#include "StreamHelper.h"

// Provides raw 16-bit audio stream decoding
class CRawPcmStream : public CStreamHelper
{
protected:
	unsigned char m_Channels;

protected:
	virtual bool DoDecodeBlock();

public:
	CRawPcmStream(CDataStream* Input);
	virtual ~CRawPcmStream();

	virtual bool InitializeHeader();
	virtual bool InitializeHeader(unsigned char Channels);
	virtual unsigned long GetSampleRate() const;
	virtual unsigned char GetChannels() const;
	virtual std::string GetFormatName() const;
};
