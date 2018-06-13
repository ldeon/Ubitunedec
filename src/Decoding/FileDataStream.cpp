// FileDataStream.cpp : A data stream from a file
//

#include "Pch.h"
#include "FileDataStream.h"
#include "DataExceptions.h"

CFileDataStream::CFileDataStream(std::istream* Input) :
	m_Input(Input),
	m_BeginOffset(0),
	m_EndOffset(0),
	m_CanSeek(false)
{
	// Seek to the end to find the offset
	m_Input->seekg(0, std::ios_base::end);
	m_EndOffset=m_Input->tellg();
	if(m_EndOffset!=-1)
	{
		m_CanSeek=true;
		m_Length=m_EndOffset-m_BeginOffset;
		m_Input->seekg(m_BeginOffset);
	}
	return;
}

CFileDataStream::CFileDataStream(std::istream* Input, std::streamoff Begin, std::streamsize Size) :
	m_Input(Input),
	m_BeginOffset(Begin),
	m_EndOffset(m_BeginOffset+Size),
	m_CanSeek(false)
{
	// Seek to the end to find the offset
	std::streamoff EndOffset;
	m_Input->seekg(0, std::ios_base::end);
	EndOffset=m_Input->tellg();
	if(EndOffset!=-1)
	{
		m_CanSeek=true;
		if(m_BeginOffset>EndOffset)
		{
			m_BeginOffset=EndOffset;
		}
		if(m_EndOffset>EndOffset)
		{
			m_EndOffset=EndOffset;
		}
		m_Length=m_EndOffset-m_BeginOffset;
		m_Input->seekg(m_BeginOffset);
	}
	return;
}

CFileDataStream::~CFileDataStream()
{
	return;
}

unsigned long CFileDataStream::DoRead(void* Buffer, unsigned long Length, bool Exact)
{
	CheckAlignment();

	// Figure out how much is left
	unsigned long BytesLeft;
	BytesLeft=GetBytesLeft();
	if(BytesLeft!=-1 && BytesLeft<Length)
	{
		Length=BytesLeft;
	}

	// Simple read and see if we hit the end of the file
	m_Input->read((char*)Buffer, Length);
	if(m_Input->fail())
	{
		// TODO: I don't know how to handle this case
		return 0;
	}
	return Length;
}

bool CFileDataStream::DoCanSeekBackward() const
{
	return m_CanSeek;
}

unsigned long CFileDataStream::DoForwardSeek(unsigned long RelOffset)
{
	CheckAlignment();

	// There are two different ways of doing this
	if(m_CanSeek)
	{
		m_Input->seekg(RelOffset, std::ios_base::cur);
		if(m_Input->fail() || m_Input->tellg()>m_EndOffset)
		{
			m_Input->seekg(m_EndOffset);
		}
		if(m_Input->fail())
		{
			throw(XSeekError());
		}
		return m_Input->tellg()-m_BeginOffset;
	}
	else
	{
		m_Input->ignore(RelOffset);
		return m_CurrentOffset+RelOffset;
	}
	return -1;
}

unsigned long CFileDataStream::DoBackwardSeek(unsigned long RelOffset)
{
	CheckAlignment();

	// Make sure we can seek
	if(!m_CanSeek)
	{
		throw(XSeekNotAllowed());
	}

	// Do the seeking
	m_Input->seekg(-(std::streamoff)RelOffset, std::ios_base::cur);
	if(m_Input->fail() || m_Input->tellg()<m_BeginOffset)
	{
		m_Input->seekg(m_BeginOffset);
	}
	if(m_Input->fail())
	{
		throw(XSeekError());
	}
	return m_Input->tellg()-m_BeginOffset;
}

bool CFileDataStream::DoIsEnd() const
{
	// End of file or at our offset
	if(m_Input->eof())
	{
		return true;
	}
	if(GetBytesLeft()==0)
	{
		return true;
	}
	return false;
}

unsigned long CFileDataStream::GetBytesLeft() const
{
	// Check the end offset
	if(m_EndOffset==-1)
	{
		return -1;
	}
	return m_Length-m_CurrentOffset;
}

void CFileDataStream::CheckAlignment()
{
	if(m_CanSeek && (unsigned long)(m_Input->tellg()-m_BeginOffset)!=m_CurrentOffset)
	{
		m_Input->seekg(m_BeginOffset+m_CurrentOffset);
	}
	return;
}

