// RawPcmStream.cpp : Raw 16-bit audio stream decoding
//

#include "Pch.h"
#include "RawPcmStream.h"
#include "DataStream.h"
#include "AudioExceptions.h"

CRawPcmStream::CRawPcmStream(CDataStream* Input) :
	CStreamHelper(Input),
	m_Channels(2)
{
	return;
}

CRawPcmStream::~CRawPcmStream()
{
	return;
}

bool CRawPcmStream::InitializeHeader()
{
	return InitializeHeader(2);
}

bool CRawPcmStream::InitializeHeader(unsigned char Channels)
{
	// Check the parameters
	if(Channels==0)
	{
		throw(XUserException("The number of channels must not be 0"));
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
	m_Channels=Channels;

	m_Initialized=true;
	return true;
}

bool CRawPcmStream::DoDecodeBlock()
{
	// Read the audio into the output buffer
	unsigned long BufferLength=RecommendBufferLength();
	unsigned long ReadLength;
	PrepareOutputBuffer(BufferLength);
	ReadLength=m_InputStream->Read(m_OutputBuffer, BufferLength*2);
	m_OutputBufferUsed=ReadLength/2;
	return true;
}

unsigned long CRawPcmStream::GetSampleRate() const
{
	return 44100;
}

unsigned char CRawPcmStream::GetChannels() const
{
	return m_Channels;
}

std::string CRawPcmStream::GetFormatName() const
{
	return "raw";
}
