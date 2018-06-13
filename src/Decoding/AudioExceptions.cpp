// AudioExceptions.cpp : Audio stream exceptions
//

#include "Pch.h"
#include "AudioExceptions.h"

// A general audio exception
XAudioException::XAudioException()
{
	return;
}

XAudioException::XAudioException(std::string Message) :
	m_Message(Message)
{
	return;
}

XAudioException::~XAudioException()
{
	return;
}

std::string XAudioException::GetMessage() const
{
	return m_Message;
}

std::string XAudioException::GetFriendlyMessage() const
{
	return "General error";
}

// A user generated exception
XUserException::XUserException()
{
	return;
}

XUserException::XUserException(std::string Message) :
	XAudioException(Message)
{
	return;
}

XUserException::~XUserException()
{
	return;
}

std::string XUserException::GetMessage() const
{
	return m_Message;
}

std::string XUserException::GetFriendlyMessage() const
{
	return "Invalid user input";
}

// A file generated exception
XFileException::XFileException()
{
	return;
}

XFileException::XFileException(std::string Message) :
	XAudioException(Message)
{
	return;
}

XFileException::~XFileException()
{
	return;
}

std::string XFileException::GetMessage() const
{
	return m_Message;
}

std::string XFileException::GetFriendlyMessage() const
{
	return "Invalid file";
}

// A program bug generated exception
XProgramException::XProgramException()
{
	return;
}

XProgramException::XProgramException(std::string Message) :
	XAudioException(Message)
{
	return;
}

XProgramException::~XProgramException()
{
	return;
}

std::string XProgramException::GetMessage() const
{
	return m_Message;
}

std::string XProgramException::GetFriendlyMessage() const
{
	return "Program bug";
}
