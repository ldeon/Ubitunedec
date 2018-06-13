// OggVorbisStream.h : An Ogg Vorbis stream
//

#pragma once
#include <vorbis/vorbisfile.h>
#include "StreamHelper.h"

// Provides Ogg Vorbis audio stream decoding
class COggVorbisStream : public CStreamHelper
{
protected:
	OggVorbis_File m_File;

protected:
	virtual bool DoDecodeBlock();
	void Clear();

public:
	COggVorbisStream(CDataStream* Input);
	virtual ~COggVorbisStream();

	virtual bool InitializeHeader();
	virtual unsigned long GetSampleRate() const;
	virtual unsigned char GetChannels() const;
	virtual std::string GetFormatName() const;
};
