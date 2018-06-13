// AudioExceptions.h : Audio stream exceptions
//

#pragma once

// A general audio exception
class XAudioException
{
protected:
	std::string m_Message;

public:
	XAudioException();
	XAudioException(std::string Message);
	virtual ~XAudioException();

	virtual std::string GetMessage() const;
	virtual std::string GetFriendlyMessage() const;
};

// A user generated exception
class XUserException : public XAudioException
{
public:
	XUserException();
	XUserException(std::string Message);
	virtual ~XUserException();

	virtual std::string GetMessage() const;
	virtual std::string GetFriendlyMessage() const;
};

// A file generated exception
class XFileException : public XAudioException
{
public:
	XFileException();
	XFileException(std::string Message);
	virtual ~XFileException();

	virtual std::string GetMessage() const;
	virtual std::string GetFriendlyMessage() const;
};

// A program bug generated exception
class XProgramException : public XAudioException
{
public:
	XProgramException();
	XProgramException(std::string Message);
	virtual ~XProgramException();

	virtual std::string GetMessage() const;
	virtual std::string GetFriendlyMessage() const;
};
