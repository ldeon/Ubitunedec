// StreamHelper.h : Contains some helper functions for streams
//

#pragma once
#include "AudioStream.h"

// Helper class for decoding streams
class CStreamHelper : public CAudioStream
{
protected:
	bool m_Initialized;
	short* m_OutputBuffer;
	unsigned long m_OutputBufferLength;
	unsigned long m_OutputBufferOffset;
	unsigned long m_OutputBufferUsed;

protected:
	virtual bool DoDecodeBlock()=0;
	virtual void PrepareOutputBuffer(unsigned long OutputBufferLength);
	
public:
	CStreamHelper(CDataStream* Input);
	virtual ~CStreamHelper();

	virtual bool IsInitialized() const;
	virtual bool Decode(short* Buffer, unsigned long& NumberSamples);
};
