// BufferDataStream.cpp : A data stream from buffers
//

#include "Pch.h"
#include "BufferDataStream.h"
#include "DataExceptions.h"

#ifndef min
#define min(_A, _B) ((_A)<(_B) ? (_A) : (_B))
#endif

CBufferDataStream::CBufferDataStream(unsigned long InternalBufferSize) :
	m_Buffer(NULL),
	m_BufferLength(0),
	m_BufferOffset(0),
	m_BufferUsed(0),
	m_StreamEnd(false),
	m_NeedBuffer(NULL),
	m_NeedBufferUserData(NULL)
{
	// Just checking
	if(InternalBufferSize==0)
	{
		InternalBufferSize=0x20000;
	}

	// Allocate a buffer
	m_Buffer=new unsigned char[InternalBufferSize];
	m_BufferLength=InternalBufferSize;
	return;
}

CBufferDataStream::~CBufferDataStream()
{
	delete[] m_Buffer;
	return;
}

void CBufferDataStream::SetNeedBufferCallback(TNeedBufferCallback Function, void* UserData)
{
	m_NeedBuffer=Function;
	m_NeedBufferUserData=UserData;
	return;
}

void CBufferDataStream::SendBuffer(void* Buffer, unsigned long Length)
{
	// Check to make sure this is not the end
	if(m_StreamEnd)
	{
		m_StreamEnd=false;
	}

	// Get the amount of unused space
	unsigned long UnusedSpace=GetUnusedBuffer();

	// Make the buffer bigger if we have to
	if(Length>UnusedSpace)
	{
		// Allocate
		unsigned char* NewBuffer;
		unsigned long OldBufferUsed=m_BufferLength-UnusedSpace;
		NewBuffer=new unsigned char[Length+OldBufferUsed];

		// Copy old data
		memcpy(NewBuffer, m_Buffer+m_BufferOffset, OldBufferUsed);

		// Update
		delete[] m_Buffer;
		m_Buffer=NewBuffer;
		m_BufferLength=Length+OldBufferUsed;
		m_BufferOffset=0;
		m_BufferUsed=OldBufferUsed;
	}

	if(m_BufferUsed>=m_BufferOffset)
	{
		// Copy the first part
		unsigned long FirstPartCopy=min(m_BufferLength-m_BufferUsed, Length);
		if(FirstPartCopy)
		{
			memcpy(m_Buffer+m_BufferUsed, (unsigned char*)Buffer, FirstPartCopy);
			m_BufferUsed+=FirstPartCopy;
		}

		// Copy the second part
		unsigned long SecondPartCopy=min(m_BufferOffset, Length-FirstPartCopy);
		if(SecondPartCopy)
		{
			memcpy(m_Buffer, (unsigned char*)Buffer+FirstPartCopy, SecondPartCopy);
			m_BufferUsed=SecondPartCopy;
		}
	}
	else
	{
		// Only one part
		unsigned long PartCopy=min(m_BufferOffset-m_BufferUsed, Length);
		memcpy(m_Buffer+m_BufferUsed, Buffer, PartCopy);
		m_BufferUsed+=PartCopy;
	}
	return;
}

void CBufferDataStream::EndStream()
{
	m_StreamEnd=true;
	return;
}

void CBufferDataStream::ResetBuffer()
{
	m_BufferOffset=0;
	m_BufferUsed=0;
	return;
}

unsigned long CBufferDataStream::GetBufferedLength() const
{
	return GetUsedBuffer();
}

unsigned long CBufferDataStream::DoRead(void* Buffer, unsigned long Length, bool Exact)
{
	// How much data can we copy?
	unsigned long ReadLength=GetUsedBuffer();

	// Is it enough?
	if(ReadLength<Length && !m_StreamEnd)
	{
		if(!NeedMoreData(Length))
		{
			// Oops! We don't have enough data, but it's not the end of the stream yet!
			throw(XNeedBuffer(Length));
			return 0;
		}
		else
		{
			return DoRead(Buffer, Length, Exact);
		}
	}
	ReadLength=min(ReadLength, Length);

	if(m_BufferUsed>=m_BufferOffset)
	{
		// Only one part
		memcpy(Buffer, m_Buffer+m_BufferOffset, ReadLength);
		m_BufferOffset+=ReadLength;
	}
	else
	{
		// First part
		unsigned long FirstPart=min(m_BufferLength-m_BufferOffset, ReadLength);
		if(FirstPart)
		{
			memcpy(Buffer, m_Buffer+m_BufferOffset, FirstPart);
			m_BufferOffset+=FirstPart;
		}

		// Second part
		unsigned long SecondPart=min(m_BufferUsed, ReadLength-FirstPart);
		if(SecondPart)
		{
			memcpy((unsigned char*)Buffer+FirstPart, m_Buffer, SecondPart);
			m_BufferOffset=SecondPart;
		}
	}
	return ReadLength;
}

unsigned long CBufferDataStream::DoPeek(void* Buffer, unsigned long Length, bool Exact)
{
	unsigned long Read = min(GetUsedBuffer(), Length);
	
	if (Read > 0)
	{
		// Emulate this with a read, and then we reset the buffer pointer
		unsigned long OldBufferOffset = m_BufferOffset;
		unsigned long Result;
		Result = DoRead(Buffer, Length, Exact);
		m_BufferOffset = OldBufferOffset;
		return Result;
	}
	return 0;
}

bool CBufferDataStream::DoCanSeekBackward() const
{
	return false;
}

unsigned long CBufferDataStream::DoForwardSeek(unsigned long RelOffset)
{
	// How much data can we copy?
	unsigned long ReadLength=GetUsedBuffer();

	// Is it enough?
	if(ReadLength<RelOffset && !m_StreamEnd)
	{
		if(!NeedMoreData(RelOffset))
		{
			// Oops! We don't have enough data, but it's not the end of the stream yet!
			throw(XNeedBuffer(RelOffset));
			return -1;
		}
		else
		{
			return DoForwardSeek(RelOffset);
		}
	}
	ReadLength=min(ReadLength, RelOffset);
	m_BufferOffset=(m_BufferOffset+ReadLength)%m_BufferLength;
	return m_CurrentOffset+ReadLength;
}

bool CBufferDataStream::DoIsEnd() const
{
	// Only if there is no data left in the last buffer
	if(m_StreamEnd && GetUsedBuffer()==0)
	{
		return true;
	}
	return false;
}

unsigned long CBufferDataStream::GetUsedBuffer() const
{
	if(m_BufferUsed>=m_BufferOffset)
	{
		return m_BufferUsed-m_BufferOffset;
	}
	else
	{
		return m_BufferLength-(m_BufferOffset-m_BufferUsed);
	}
	return -1;
}

unsigned long CBufferDataStream::GetUnusedBuffer() const
{
	return m_BufferLength-GetUsedBuffer();
}

bool CBufferDataStream::NeedMoreData(unsigned long Bytes)
{
	// Check the callback
	if(m_NeedBuffer)
	{
		m_NeedBuffer(*this, Bytes, m_NeedBufferUserData);
		if(GetBufferedLength()<Bytes && !m_StreamEnd)
		{
			return false;
		}
		return true;
	}
	return false;
}
