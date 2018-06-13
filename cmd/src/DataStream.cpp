// DataStream.cpp : A data stream
//

#include "Pch.h"
#include "DataStream.h"
#include "DataExceptions.h"

CDataStream::CDataStream() :
	m_ReadBuffer(NULL),
	m_ReadBufferLength(0),
	m_CurrentOffset(0),
	m_Length(-1),
	m_Endian(LittleEndian),
	m_NeedSwap(false)
{
	return;
}

CDataStream::~CDataStream()
{
	delete[] m_ReadBuffer;
	m_ReadBuffer=NULL;
	m_ReadBufferLength=0;
	return;
}

void CDataStream::ExactRead(void* Buffer, unsigned long Length)
{
	// Check arguments
	if(Length==0)
	{
		return;
	}

	// Try to read some data
	unsigned long WasRead;
	WasRead=DoRead(Buffer, Length, true);
	m_CurrentOffset+=WasRead;
	assert(m_Length==-1 || m_CurrentOffset<=m_Length);

	// If not enough was read, fail
	if(WasRead!=Length)
	{
		throw(XNotEnoughData());
	}
	return;
}

unsigned long CDataStream::Read(void* Buffer, unsigned long Length)
{
	// Check arguments
	if(Length==0)
	{
		return 0;
	}

	// Read
	unsigned long WasRead;
	WasRead=DoRead(Buffer, Length, false);
	m_CurrentOffset+=WasRead;
	assert(m_Length==-1 || m_CurrentOffset<=m_Length);
	return WasRead;
}

void CDataStream::ExactPeek(void* Buffer, unsigned long Length)
{
	// Check arguments
	if(Length==0)
	{
		return;
	}

	// Try to read some data
	unsigned long WasRead;
	WasRead=DoPeek(Buffer, Length, true);

	// If not enough was read, fail
	if(WasRead!=Length)
	{
		throw(XNotEnoughData());
	}
	return;
}

unsigned long CDataStream::Peek(void* Buffer, unsigned long Length)
{
	// Check arguments
	if(Length==0)
	{
		return 0;
	}

	// Peek
	return DoPeek(Buffer, Length, false);
}

void* CDataStream::ExactRead(unsigned long Length)
{
	// Check arguments
	if(Length==0)
	{
		return NULL;
	}

	// Prepare read buffer
	PrepareReadBuffer(Length);

	// Try to read some data
	ExactRead(m_ReadBuffer, Length);
	return m_ReadBuffer;
}

void* CDataStream::Read(unsigned long& Length)
{
	// Check arguments
	if(Length==0)
	{
		return NULL;
	}

	// Prepare read buffer
	PrepareReadBuffer(Length);

	// Read some data
	Length=Read(m_ReadBuffer, Length);
	return m_ReadBuffer;
}

void* CDataStream::ExactPeek(unsigned long Length)
{
	// Check arguments
	if(Length==0)
	{
		return NULL;
	}

	// Prepare read buffer
	PrepareReadBuffer(Length);

	// Try to peek at some data
	ExactPeek(m_ReadBuffer, Length);
	return m_ReadBuffer;
}

void* CDataStream::Peek(unsigned long& Length)
{
	// Check arguments
	if(Length==0)
	{
		return NULL;
	}

	// Prepare read buffer
	PrepareReadBuffer(Length);

	// Read some data
	Length=Peek(m_ReadBuffer, Length);
	return m_ReadBuffer;
}

unsigned long CDataStream::Ignore(unsigned long Length)
{
	// Seek forward
	unsigned long NewOffset;
	unsigned long SeekDistance;
	NewOffset=DoForwardSeek(Length);
	SeekDistance=NewOffset-m_CurrentOffset;
	m_CurrentOffset=NewOffset;
	assert(m_Length==-1 || m_CurrentOffset<=m_Length);
	return SeekDistance;
}

void CDataStream::ExactIgnore(unsigned long Length)
{
	// Seek forward
	unsigned long NewOffset;
	unsigned long SeekDistance;
	NewOffset=DoForwardSeek(Length);
	SeekDistance=NewOffset-m_CurrentOffset;
	m_CurrentOffset=NewOffset;
	assert(m_Length==-1 || m_CurrentOffset<=m_Length);

	// If not enough was read, fail
	if(SeekDistance!=Length)
	{
		throw(XNotEnoughData());
	}
	return;
}

bool CDataStream::CanSeekBackward() const
{
	return DoCanSeekBackward();
}

void CDataStream::SeekToBeginning()
{
	SeekAbsolute(0);
	return;
}

void CDataStream::SeekAbsolute(unsigned long Offset)
{
	// Calculate whether this is forward or backward
	long RelOffset=Offset-m_CurrentOffset;

	// Seek accordingly
	if(RelOffset>0)
	{
		m_CurrentOffset=DoForwardSeek(RelOffset);
	}
	else if(RelOffset<0)
	{
		m_CurrentOffset=DoBackwardSeek(-RelOffset);
	}
	assert(m_Length==-1 || m_CurrentOffset<=m_Length);
	return;
}

void CDataStream::SeekRelative(long Offset)
{
	// Seek accordingly
	if(Offset>0)
	{
		m_CurrentOffset=DoForwardSeek(Offset);
	}
	else if(Offset<0)
	{
		m_CurrentOffset=DoBackwardSeek(-Offset);
	}
	assert(m_Length==-1 || m_CurrentOffset<=m_Length);
	return;
}

void CDataStream::SeekBackward(unsigned long Offset)
{
	// If we don't know the stream size, don't permit it
	if(m_Length==-1)
	{
		throw(XSeekNotAllowed());
	}

	// Calculate whether this is forward or backward
	long RelOffset=(m_Length-Offset)-m_CurrentOffset;

	// Seek accordingly
	if(RelOffset>0)
	{
		m_CurrentOffset=DoForwardSeek(RelOffset);
	}
	else if(RelOffset<0)
	{
		m_CurrentOffset=DoBackwardSeek(-RelOffset);
	}
	assert(m_Length==-1 || m_CurrentOffset<=m_Length);
	return;
}

unsigned long CDataStream::Tell() const
{
	assert(m_Length==-1 || m_CurrentOffset<=m_Length);
	return m_CurrentOffset;
}

unsigned long CDataStream::GetLength() const
{
	return m_Length;
}

bool CDataStream::IsEnd() const
{
	return DoIsEnd();
}

void CDataStream::SetEndian(EEndian Endian)
{
	switch(Endian)
	{
		case LittleEndian:
			m_NeedSwap=false;
		break;
		case BigEndian:
			m_NeedSwap=true;
		break;
		default:
		return;
	}
	m_Endian=Endian;
	return;
}

CDataStream::EEndian CDataStream::GetEndian() const
{
	return m_Endian;
}

unsigned long CDataStream::ExactReadULong()
{
	// Read the value
	unsigned long Value=0;
	ExactRead(&Value, 4);

	// Swap if needed
	if(m_NeedSwap)
	{
		Value=(Value&0xFF000000)>>24 | (Value&0x00FF0000)>>8 | \
			(Value&0x000000FF)<<24 | (Value&0x0000FF00)<<8;
	}
	return Value;
}

long CDataStream::ExactReadLong()
{
	return static_cast<long>(ExactReadULong());
}

unsigned short CDataStream::ExactReadUShort()
{
	// Read the value
	unsigned short Value=0;
	ExactRead(&Value, 2);

	// Swap if needed
	if(m_NeedSwap)
	{
		Value=(Value&0xFF00)>>8 | (Value&0x00FF)<<8;
	}
	return Value;
}

short CDataStream::ExactReadShort()
{
	return static_cast<short>(ExactReadUShort());
}

unsigned char CDataStream::ExactReadUChar()
{
	// Read the value
	unsigned char Value=0;
	ExactRead(&Value, 1);
	return Value;
}

char CDataStream::ExactReadChar()
{
	return static_cast<char>(ExactReadUChar());
}

unsigned long CDataStream::DoPeek(void* Buffer, unsigned long Length, bool Exact)
{
	// If we can seek backward then we can do it easily
	if(CanSeekBackward())
	{
		unsigned long PreviousOffset;
		unsigned long WasRead;
		PreviousOffset=m_CurrentOffset;
		WasRead=Read(Buffer, Length);
		SeekAbsolute(PreviousOffset);
		return WasRead;
	}
	else
	{
		assert("Peeking emulation has not been implemented yet");
	}
	return -1;
}

unsigned long CDataStream::DoBackwardSeek(unsigned long RelOffset)
{
	assert(!CanSeekBackward());
	throw(XSeekNotAllowed());
	return -1;
}

void CDataStream::PrepareReadBuffer(unsigned long Length)
{
	if(!m_ReadBuffer || m_ReadBufferLength<Length)
	{
		// Delete the old buffer
		delete[] m_ReadBuffer;
		m_ReadBuffer=NULL;
		m_ReadBufferLength=0;

		// Allocate a new buffer
		m_ReadBuffer=new unsigned char[Length];
		m_ReadBufferLength=Length;
	}
	return;
}
