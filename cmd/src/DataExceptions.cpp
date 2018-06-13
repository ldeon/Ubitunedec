// DataExceptions.h : Data stream exceptions
//

#include "Pch.h"
#include "DataExceptions.h"

XDataException::XDataException()
{
	return;
}

XDataException::XDataException(std::string Message) :
	m_Message(Message)
{
	return;
}

XDataException::~XDataException()
{
	return;
}

std::string XDataException::GetMessage() const
{
	return m_Message;
}

std::string XDataException::GetFriendlyMessage() const
{
	return "Data stream error";
}

// Exception: Seeking not allowed
XSeekNotAllowed::XSeekNotAllowed() :
	XDataException("Seeking in this stream is not allowed")
{
	return;
}

XSeekNotAllowed::~XSeekNotAllowed()
{
	return;
}

std::string XSeekNotAllowed::GetFriendlyMessage() const
{
	return "Program bug";
}

// Exception: Seek error
XSeekError::XSeekError() :
	XDataException("Seek error")
{
	return;
}

XSeekError::~XSeekError()
{
	return;
}

std::string XSeekError::GetFriendlyMessage() const
{
	return "Program bug";
}

// Exception: Not enough data to read
XNotEnoughData::XNotEnoughData() :
	XDataException("Not enough data to read")
{
	return;
}

XNotEnoughData::~XNotEnoughData()
{
	return;
}

std::string XNotEnoughData::GetFriendlyMessage() const
{
	return "File is not valid";
}

// Exception: Needs another buffer
XNeedBuffer::XNeedBuffer(unsigned long Length) :
	XDataException("Another buffer is needed; ran out of data"),
	m_Length(Length)
{
	return;
}

XNeedBuffer::~XNeedBuffer()
{
	return;
}

std::string XNeedBuffer::GetFriendlyMessage() const
{
	return "Program bug";
}

unsigned long XNeedBuffer::GetLength() const
{
	return m_Length;
}
