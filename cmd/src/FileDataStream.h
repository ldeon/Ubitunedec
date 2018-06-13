// FileDataStream.cpp : A data stream from a file
//

#pragma once

#include "DataStream.h"

// A data stream from a file
class CFileDataStream : public CDataStream
{
protected:
	std::istream* m_Input;
	std::streamoff m_BeginOffset;
	std::streamoff m_EndOffset;
	bool m_CanSeek;

protected:
	virtual unsigned long DoRead(void* Buffer, unsigned long Length, bool Exact);
	virtual bool DoCanSeekBackward() const;
	virtual unsigned long DoForwardSeek(unsigned long RelOffset);
	virtual unsigned long DoBackwardSeek(unsigned long RelOffset);
	virtual bool DoIsEnd() const;
	virtual unsigned long GetBytesLeft() const;
	virtual void CheckAlignment();

public:
	CFileDataStream(std::istream* Input);
	CFileDataStream(std::istream* Input, std::streamoff Begin, std::streamsize Size);
	virtual ~CFileDataStream();
};
