// DataStream.h : A data stream
//

#pragma once

// A data stream
class CDataStream
{
public:
	enum EEndian
	{
		LittleEndian,
		BigEndian
	};

protected:
	unsigned char* m_ReadBuffer;
	unsigned long m_ReadBufferLength;

	// TODO: Peeking emulation support

	unsigned long m_CurrentOffset;
	unsigned long m_Length;

	EEndian m_Endian;
	bool m_NeedSwap;

protected:
	virtual unsigned long DoRead(void* Buffer, unsigned long Length, bool Exact)=0;
	virtual unsigned long DoPeek(void* Buffer, unsigned long Length, bool Exact);
	virtual bool DoCanSeekBackward() const=0;
	virtual unsigned long DoForwardSeek(unsigned long RelOffset)=0;
	virtual unsigned long DoBackwardSeek(unsigned long RelOffset);
	virtual bool DoIsEnd() const=0;

	virtual void PrepareReadBuffer(unsigned long Length);

public:
	CDataStream();
	virtual ~CDataStream();

	virtual void ExactRead(void* Buffer, unsigned long Length);
	virtual unsigned long Read(void* Buffer, unsigned long Length);
	virtual void ExactPeek(void* Buffer, unsigned long Length);
	virtual unsigned long Peek(void* Buffer, unsigned long Length);
	virtual void* ExactRead(unsigned long Length);
	virtual void* Read(unsigned long& Length);
	virtual void* ExactPeek(unsigned long Length);
	virtual void* Peek(unsigned long& Length);
	virtual unsigned long Ignore(unsigned long Length);
	virtual void ExactIgnore(unsigned long Length);
	virtual bool CanSeekBackward() const;
	virtual void SeekToBeginning();
	virtual void SeekAbsolute(unsigned long Offset);
	virtual void SeekRelative(long Offset);
	virtual void SeekBackward(unsigned long Offset);
	virtual unsigned long Tell() const;
	virtual unsigned long GetLength() const;
	virtual bool IsEnd() const;

	virtual void SetEndian(EEndian Endian);
	virtual EEndian GetEndian() const;
	virtual unsigned long ExactReadULong();
	virtual long ExactReadLong();
	virtual unsigned short ExactReadUShort();
	virtual short ExactReadShort();
	virtual unsigned char ExactReadUChar();
	virtual char ExactReadChar();
};
