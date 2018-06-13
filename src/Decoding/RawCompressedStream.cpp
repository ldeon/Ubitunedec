// RawCompressedStream.cpp : UbiSoft raw compressed audio stream decoding
//

#include "Pch.h"
#include "RawCompressedStream.h"
#include "Adpcm.h"
#include "DataStream.h"
#include "AudioExceptions.h"

CRawCompressedStream::CRawCompressedStream(CDataStream* Input) :
	CStreamHelper(Input),
	m_Stereo(true),
	m_LeftSample(0),
	m_LeftIndex(0),
	m_RightSample(0),
	m_RightIndex(0)
{
	return;
}

CRawCompressedStream::~CRawCompressedStream()
{
	return;
}

bool CRawCompressedStream::InitializeHeader()
{
	return InitializeHeader(2);
}

bool CRawCompressedStream::InitializeHeader(unsigned char Channels, short LeftSample, unsigned char LeftIndex, short RightSample, unsigned char RightIndex)
{
	// Check the parameters
	if(Channels<1 || Channels>2)
	{
		throw(XUserException("The number of channels must 1 or 2"));
	}

	// Clear old data
	m_OutputBufferOffset=0;
	m_OutputBufferUsed=0;

	// Go back to the beginning of the file
	if(m_InputStream->CanSeekBackward())
	{
		m_InputStream->SeekToBeginning();
	}

	// Set some variables
	m_LeftSample=LeftSample;
	m_RightSample=RightSample;
	m_LeftIndex=LeftIndex;
	m_RightIndex=RightIndex;
	if(Channels==1)
	{
		m_Stereo=false;
	}
	else if(Channels==2)
	{
		m_Stereo=true;
	}

	m_Initialized=true;
	return true;
}

bool CRawCompressedStream::DoDecodeBlock()
{
	// Prepare the buffers
	unsigned char* Buffer;
	unsigned long BufferLength=RecommendBufferLength()/2;
	PrepareOutputBuffer(RecommendBufferLength());
	Buffer=(unsigned char*)m_InputStream->Read(BufferLength);

	// Calculate how many samples are needed
	m_OutputBufferUsed=BufferLength*2;
	if(m_OutputBufferUsed<1)
	{
		return true;
	}

	// Do the decompression
	if(!m_Stereo)
	{
		SAdpcmMonoParam Param;
		Param.InputBuffer=Buffer;
		Param.InputLength=BufferLength;
		Param.OutputBuffer=m_OutputBuffer;
		Param.FirstSample=m_LeftSample;
		Param.FirstIndex=m_LeftIndex;
		DecompressMonoAdpcm(&Param);
		m_LeftSample=Param.FirstSample;
		m_LeftIndex=Param.FirstIndex;
		m_RightSample=0;
		m_RightIndex=0;
	}
	else
	{
		SAdpcmStereoParam Param;
		Param.InputBuffer=Buffer;
		Param.InputLength=BufferLength;
		Param.OutputBuffer=m_OutputBuffer;
		Param.FirstLeftSample=m_LeftSample;
		Param.FirstLeftIndex=m_LeftIndex;
		Param.FirstRightSample=m_RightSample;
		Param.FirstRightIndex=m_RightIndex;
		DecompressStereoAdpcm(&Param);
		m_LeftSample=Param.FirstLeftSample;
		m_LeftIndex=Param.FirstLeftIndex;
		m_RightSample=Param.FirstRightSample;
		m_RightIndex=Param.FirstRightIndex;
	}
	return true;
}

unsigned long CRawCompressedStream::GetSampleRate() const
{
	return 44100;
}

unsigned char CRawCompressedStream::GetChannels() const
{
	return m_Stereo ? 2 : 1;
}

std::string CRawCompressedStream::GetFormatName() const
{
	return "ubi_raw";
}
