// OldInterleavedStream.h : UbiSoft version 2 interleaved audio stream decoding
//

#pragma once
#include "LayeredStreamHelper.h"
#include "UbiFormats.h"

class CBufferDataStream;

// Provides UbiSoft version 2 interleaved audio stream decoding
class COldInterleavedStream : public CLayeredStreamHelper
{
protected:
	struct SOldInterleavedLayer;

protected:
	unsigned long m_BlockNumber;
	unsigned long m_TotalBytes;
	std::vector<SOldInterleavedLayer*> m_Layers;
	unsigned long m_SampleRate;
	bool m_Stereo;

protected:
	virtual bool DoDecodeLayer(unsigned long LayerIndex);
	virtual bool DoReadBlock();
	void DoRegisterParams();
	void Clear();

public:
	COldInterleavedStream(CDataStream* Input);
	virtual ~COldInterleavedStream();

	virtual bool InitializeHeader();
	virtual bool InitializeHeader(unsigned long SampleRate);
	virtual unsigned long GetSampleRate() const;
	virtual unsigned char GetChannels() const;
	virtual std::string GetFormatName() const;
	virtual EUbiFormat GetType(unsigned long Layer) const;
	virtual unsigned long GetTotalBytes() const;
	virtual unsigned long GetLayerCount() const;
	static bool LayerExtract(CDataStream* Input, unsigned long Layer, std::ostream& Output);

private:
	static void BufferCallback(CBufferDataStream& Stream, unsigned long Bytes, void* UserData);
};
