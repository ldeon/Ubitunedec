// Interleaved9Stream.h : UbiSoft version 9 interleaved audio stream decoding
//

#pragma once
#include "LayeredStreamHelper.h"
#include "UbiFormats.h"

class CBufferDataStream;

// Provides UbiSoft version 8 interleaved audio stream decoding
class CInterleaved9Stream : public CLayeredStreamHelper
{
public:
	enum EVariant
	{
		EV_A
	};

protected:
	struct SInterleavedLayer;

protected:
	EVariant m_Variant;
	unsigned long m_BlockNumber;
	unsigned long m_TotalBlocks;
	std::vector<SInterleavedLayer*> m_Layers;
	unsigned long m_SampleRate;
	unsigned char m_Channels;

protected:
	virtual bool DoDecodeLayer(unsigned long LayerIndex);
	virtual bool DoReadBlock();
	void DoRegisterParams();
	void Clear();

public:
	CInterleaved9Stream(CDataStream* Input);
	virtual ~CInterleaved9Stream();

	virtual bool InitializeHeader();
	virtual bool InitializeHeader(unsigned long SampleRate);
	virtual bool InitializeHeader(unsigned long SampleRate, unsigned char PcmChannels);
	virtual unsigned long GetSampleRate() const;
	virtual unsigned char GetChannels() const;
	virtual std::string GetFormatName() const;
	virtual EUbiFormat GetType(unsigned long Layer) const;
	virtual unsigned long GetNumberBlocks() const;
	virtual unsigned long GetLayerCount() const;
	static bool LayerExtract(CDataStream* Input, unsigned long Layer, std::ostream& Output);

private:
	static void BufferCallback(CBufferDataStream& Stream, unsigned long Bytes, void* UserData);
};
