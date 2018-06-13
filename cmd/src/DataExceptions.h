// DataExceptions.h : Data stream exceptions
//

#pragma once

// A general data stream exception
class XDataException
{
protected:
	std::string m_Message;

public:
	XDataException();
	XDataException(std::string Message);
	virtual ~XDataException();

	virtual std::string GetMessage() const;
	virtual std::string GetFriendlyMessage() const;
};

// Seeking is not allowed
class XSeekNotAllowed : public XDataException
{
public:
	XSeekNotAllowed();
	virtual ~XSeekNotAllowed();

	virtual std::string GetFriendlyMessage() const;
};

// Error seeking
class XSeekError : public XDataException
{
public:
	XSeekError();
	virtual ~XSeekError();

	virtual std::string GetFriendlyMessage() const;
};

// Not enough data to read
class XNotEnoughData : public XDataException
{
public:
	XNotEnoughData();
	virtual ~XNotEnoughData();

	virtual std::string GetFriendlyMessage() const;
};

// Needs another buffer
class XNeedBuffer : public XDataException
{
protected:
	unsigned long m_Length;

public:
	XNeedBuffer(unsigned long Length);
	virtual ~XNeedBuffer();

	virtual std::string GetFriendlyMessage() const;
	virtual unsigned long GetLength() const;
};
