// Old6Or4BitStream.h : UbiSoft Old 6-Or-4 Bit audio stream (XIII and SC1 PC) decoding
//

#include "Pch.h"
#include "Old6Or4BitStream.h"
#include "DataStream.h"
#include "DataExceptions.h"
#include "AudioExceptions.h"
#include "6BitAdpcm.h"

COld6Or4BitStream::COld6Or4BitStream(CDataStream* Input) :
	CStreamHelper(Input),
	m_SampleRate(36000),
	m_Channels(2),
	m_ByteBlockSize(0),
	m_ExpandedBuffer(NULL)
{
	return;
}

COld6Or4BitStream::~COld6Or4BitStream()
{
	delete[] m_ExpandedBuffer;
	m_ExpandedBuffer=NULL;
	return;
}

bool COld6Or4BitStream::InitializeHeader()
{
	return InitializeHeader(0);
}

bool COld6Or4BitStream::InitializeHeader(unsigned long SampleRate)
{
	// Clear old data
	m_OutputBufferOffset=0;
	m_OutputBufferUsed=0;
	m_ByteBlockSize=0;
	m_SamplesLeft=0;
	delete[] m_ExpandedBuffer;
	m_ExpandedBuffer=NULL;

	// Go back to the beginning of the file
	if(m_InputStream->CanSeekBackward())
	{
		m_InputStream->SeekToBeginning();
	}

	// Read the header
	m_InputStream->ExactRead(&m_Header, sizeof(m_Header));

	// Check the signature
	if(m_Header.Signature!=8)
	{
		throw(XFileException("File does not have the correct signature (should be 08000000)"));
	}

	// Check the number of bits
	if(m_Header.BitsPerSample!=4 && m_Header.BitsPerSample!=6)
	{
		throw(XFileException("File uses an unrecognized number of bits per sample"));
	}

	// Set the sample rate
	m_SampleRate=36000;
	if(m_Header.SampleRate<=96000 && m_Header.SampleRate>1000)
	{
		m_SampleRate=m_Header.SampleRate;
	}
	if(SampleRate)
	{
		m_SampleRate=SampleRate;
	}

	// Set the channels
	if(m_Header.Channels!=1 && m_Header.Channels!=2)
	{
		throw(XFileException("DecUbiSnd is unable to decode files with this number of channels"));
	}
	m_Channels=(unsigned char)m_Header.Channels;

	// Allocate an expanded buffer
	m_SamplesLeft = m_Header.SampleCount;
	m_ByteBlockSize = m_Header.BitsPerSample * m_Header.BlockSize / 4;
	if(m_Header.BitsPerSample==6)
	{
		m_ExpandedBuffer=new unsigned long[Get6BitAdpcmSamples(m_ByteBlockSize)];
	}
	else if(m_Header.BitsPerSample==4)
	{
		m_ExpandedBuffer=new unsigned long[Get4BitAdpcmSamples(m_ByteBlockSize)];
	}
	m_PersistHeader.Signature=0;
	m_Initialized=true;
	return true;
}

bool COld6Or4BitStream::DoDecodeBlock()
{
	// Check for the end of the file
	if(m_InputStream->IsEnd())
	{
		m_OutputBufferUsed=0;
		return true;
	}
	if(m_SamplesLeft<1)
	{
		m_OutputBufferUsed=0;
		return true;
	}

	unsigned long SampleCount=0;
	if(m_Header.BitsPerSample==6)
	{
		// Output buffer
		SampleCount=Get6BitAdpcmSamples(m_ByteBlockSize);
		PrepareOutputBuffer(SampleCount);

		if(m_Channels==1)
		{
			// Read the header
			S6BitAdpcmBlockHeader Header;
			if(m_InputStream->Read(&Header, sizeof(Header))<sizeof(Header))
			{
				m_OutputBufferUsed=0;
				return true;
			}
			if(Header.Signature!=2)
			{
				m_OutputBufferUsed=0;
				return true;
			}

			// Decode the first half of the block
			unsigned long Length;
			void* Buffer;
			Length=m_ByteBlockSize/2;
			Buffer=m_InputStream->Read(Length);
			m_InputStream->Ignore(1);
			Expand6BitAdpcmBlock(Buffer, m_ExpandedBuffer, SampleCount/2);
			DecompressMono6BitAdpcmBlock(Header, m_ExpandedBuffer, m_OutputBuffer, \
				SampleCount/2);

			// Decode the second half of the block
			Length=m_ByteBlockSize/2;
			Buffer=m_InputStream->Read(Length);
			m_InputStream->Ignore(1);
			Expand6BitAdpcmBlock(Buffer, m_ExpandedBuffer, SampleCount/2);
			DecompressMono6BitAdpcmBlock(Header, m_ExpandedBuffer, \
				m_OutputBuffer+(SampleCount/2), SampleCount/2);
		}
		else if(m_Channels==2)
		{
			// Read the headers
			S6BitAdpcmBlockHeader Left;
			S6BitAdpcmBlockHeader Right;
			if(m_InputStream->Read(&Left, sizeof(Left))<sizeof(Left))
			{
				m_OutputBufferUsed=0;
				return true;
			}
			if(m_InputStream->Read(&Right, sizeof(Right))<sizeof(Right))
			{
				m_OutputBufferUsed=0;
				return true;
			}
			if(Left.Signature!=2 || Right.Signature!=2)
			{
				m_OutputBufferUsed=0;
				return true;
			}

			// Decode the first half of the block
			unsigned long Length;
			void* Buffer;
			Length=m_ByteBlockSize/2;
			Buffer=m_InputStream->Read(Length);
			m_InputStream->Ignore(1);
			Expand6BitAdpcmBlock(Buffer, m_ExpandedBuffer, SampleCount/2);
			DecompressStereo6BitAdpcmBlock(Left, Right, m_ExpandedBuffer, m_OutputBuffer, \
				SampleCount/2);

			// Decode the second half of the block
			Length=m_ByteBlockSize/2;
			Buffer=m_InputStream->Read(Length);
			m_InputStream->Ignore(1);
			Expand6BitAdpcmBlock(Buffer, m_ExpandedBuffer, SampleCount/2);
			DecompressStereo6BitAdpcmBlock(Left, Right, m_ExpandedBuffer, \
				m_OutputBuffer+(SampleCount/2), SampleCount/2);
		}
	}
	else if(m_Header.BitsPerSample==4)
	{
		// Output buffer
		SampleCount=Get4BitAdpcmSamples(m_ByteBlockSize);
		PrepareOutputBuffer(SampleCount);

		if(m_Channels==1)
		{
			/*
			// Read the headers
			S4BitAdpcmBlockHeader Header;
			if(m_InputStream->Read(&Header, sizeof(Header))<sizeof(Header))
			{
				m_OutputBufferUsed=0;
				return true;
			}
			if(Header.Signature!=2)
			{
				m_OutputBufferUsed=0;
				return true;
			}
			*/
			if(m_PersistHeader.Signature!=2)
			{
				if(m_InputStream->Read(&m_PersistHeader, sizeof(m_PersistHeader))<sizeof(m_PersistHeader))
				{
					m_OutputBufferUsed=0;
					return true;
				}
				if(m_PersistHeader.Signature!=2)
				{
					m_OutputBufferUsed=0;
					return true;
				}
			}
			else
			{
				S4BitAdpcmBlockHeader Header;
				if(m_InputStream->Read(&Header, sizeof(Header))<sizeof(Header))
				{
					m_OutputBufferUsed=0;
					return true;
				}
				if(Header.Signature!=2)
				{
					m_OutputBufferUsed=0;
					return true;
				}
			}

			// Decode the first half of the block
			unsigned long Length;
			void* Buffer;
			Length=m_ByteBlockSize/2;
			Buffer=m_InputStream->Read(Length);
			m_InputStream->Ignore(1);
			Expand4BitAdpcmBlock(Buffer, m_ExpandedBuffer, SampleCount/2);
			DecompressMono4BitAdpcmBlock(m_PersistHeader, m_ExpandedBuffer, \
				m_OutputBuffer, SampleCount/2);

			// Decode the second half of the block
			Length=m_ByteBlockSize/2;
			Buffer=m_InputStream->Read(Length);
			m_InputStream->Ignore(1);
			Expand4BitAdpcmBlock(Buffer, m_ExpandedBuffer, SampleCount/2);
			DecompressMono4BitAdpcmBlock(m_PersistHeader, m_ExpandedBuffer, \
				m_OutputBuffer+(SampleCount/2), SampleCount/2);
		}
		else if(m_Channels==2)
		{
			// Read the headers
			S4BitAdpcmBlockHeader Left;
			S4BitAdpcmBlockHeader Right;
			if(m_InputStream->Read(&Left, sizeof(Left))<sizeof(Left))
			{
				m_OutputBufferUsed=0;
				return true;
			}
			if(m_InputStream->Read(&Right, sizeof(Right))<sizeof(Right))
			{
				m_OutputBufferUsed=0;
				return true;
			}
			if(Left.Signature!=2 || Right.Signature!=2)
			{
				m_OutputBufferUsed=0;
				return true;
			}

			// Decode the first half of the block
			unsigned long Length;
			void* Buffer;
			Length=m_ByteBlockSize/2;
			Buffer=m_InputStream->Read(Length);
			m_InputStream->Ignore(1);
			Expand4BitAdpcmBlock(Buffer, m_ExpandedBuffer, SampleCount/2);
			DecompressStereo4BitAdpcmBlock(Left, Right, m_ExpandedBuffer, \
				m_OutputBuffer, SampleCount/2);

			// Decode the second half of the block
			Length=m_ByteBlockSize/2;
			Buffer=m_InputStream->Read(Length);
			m_InputStream->Ignore(1);
			Expand4BitAdpcmBlock(Buffer, m_ExpandedBuffer, SampleCount/2);
			DecompressStereo4BitAdpcmBlock(Left, Right, m_ExpandedBuffer, \
				m_OutputBuffer+(SampleCount/2), SampleCount/2);
		}
	}

	// How much is actually left
	if(SampleCount>m_SamplesLeft)
	{
		m_OutputBufferUsed=m_SamplesLeft;
		m_SamplesLeft=0;
	}
	else
	{
		m_OutputBufferUsed=SampleCount;
		m_SamplesLeft-=SampleCount;
	}
	return true;
}

unsigned long COld6Or4BitStream::GetSampleRate() const
{
	return m_SampleRate;
}

unsigned char COld6Or4BitStream::GetChannels() const
{
	return m_Channels;
}

std::string COld6Or4BitStream::GetFormatName() const
{
	return "ubi_6or4";
}

unsigned char COld6Or4BitStream::GetBitsPerSample() const
{
	return (unsigned char)m_Header.BitsPerSample;
}

