// OggVorbisStream.cpp : An Ogg Vorbis stream
//

#include "Pch.h"

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "AudioExceptions.h"
#include "OggVorbisStream.h"
#include "Adpcm.h"
#include "DataStream.h"

// Ogg Vorbis callbacks
size_t DataStream_Read(void* Buffer, size_t Size, size_t Count, void* Pointer)
{
	CDataStream* Stream=(CDataStream*)Pointer;
	if(Stream==NULL)
	{
		return 0;
	}
	return Stream->Read(Buffer, (unsigned long)Size*Count);
}

int DataStream_Seek(void* Pointer, ogg_int64_t Offset, int Origin)
{
	CDataStream* Stream=(CDataStream*)Pointer;
	if(Stream==NULL)
	{
		return -1;
	}
	if(!Stream->CanSeekBackward())
	{
		return -1;
	}
	if(Origin==SEEK_SET)
	{
		Stream->SeekAbsolute((unsigned long)Offset);
	}
	else if(Origin==SEEK_CUR)
	{
		Stream->SeekRelative((long)Offset);
	}
	else if(Origin==SEEK_END)
	{
		Stream->SeekBackward((unsigned long)Offset);
	}
	return 0;
}

long DataStream_Tell(void* Pointer)
{
	CDataStream* Stream=(CDataStream*)Pointer;
	if(Stream==NULL)
	{
		return -1;
	}
	return Stream->Tell();
}

COggVorbisStream::COggVorbisStream(CDataStream* Input) :
	CStreamHelper(Input)
{
	return;
}

COggVorbisStream::~COggVorbisStream()
{
	Clear();
	return;
}

bool COggVorbisStream::InitializeHeader()
{
	// Make sure there is no leftover data
	Clear();

	// Create the callbacks structure
	ov_callbacks Callbacks;
	Callbacks.read_func=DataStream_Read;
	Callbacks.seek_func=DataStream_Seek;
	Callbacks.close_func=NULL;
	Callbacks.tell_func=DataStream_Tell;

	// Open the file
	int Result;
	Result=ov_open_callbacks(m_InputStream, &m_File, NULL, 0, Callbacks);
	switch(Result)
	{
		case 0:
		break;
		case OV_ENOTVORBIS:
			throw(XFileException("The bitstream doesn't contain any Vorbis data"));
		case OV_EVERSION:
			throw(XFileException("Vorbis version mismatch"));
		case OV_EBADHEADER:
			throw(XFileException("Invalid Vorbis bitstream header"));
		default:
			throw(XFileException("Unspecified Vorbis error"));
	}

	// We're initialized
	m_Initialized=true;
	return true;
}

bool COggVorbisStream::DoDecodeBlock()
{
	// Make sure it's valid
	if(!m_Initialized)
	{
		throw(XProgramException("The stream has not been initialized"));
	}

	// Read into the buffer
	long Result;
	int Bitstream;
	PrepareOutputBuffer(RecommendBufferLength());
	Result=ov_read(&m_File, (char*)m_OutputBuffer, RecommendBufferLength()*2, 0, 2, 1, &Bitstream);
	if(Result==OV_HOLE)
	{
		throw(XFileException("Interruption in Vorbis data"));
	}
	else if(Result==OV_EBADLINK)
	{
		throw(XFileException("Invalid stream section"));
	}
	m_OutputBufferUsed=Result/2;
	//std::cout << m_OutputBufferUsed << std::endl;
	return true;
}

unsigned long COggVorbisStream::GetSampleRate() const
{
	// Make sure it's valid
	if(!m_Initialized)
	{
		throw(XProgramException("The file has not been initialized"));
	}

	// Get the Vorbis information
	vorbis_info *Info;
	Info=ov_info((OggVorbis_File*)&m_File, -1);
	return Info->rate;
}

unsigned char COggVorbisStream::GetChannels() const
{
	// Make sure it's valid
	if(!m_Initialized)
	{
		throw(XProgramException("The file has not been initialized"));
	}

	// Get the Vorbis information
	vorbis_info *Info;
	Info=ov_info((OggVorbis_File*)&m_File, -1);
	return Info->channels;
}

std::string COggVorbisStream::GetFormatName() const
{
	return "ogg";
}

void COggVorbisStream::Clear()
{
	// Close Ogg Vorbis
	if(m_Initialized)
	{
		ov_clear(&m_File);
		m_Initialized=false;
	}
	m_OutputBufferOffset=0;
	m_OutputBufferUsed=0;
	return;
}
