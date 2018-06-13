// SegmentParser.cpp : Segment definition file parser
//

#include "Pch.h"
#include "SegmentParser.h"

CSegmentParser::CSegmentParser() :
	m_ParserStatus(true),
	m_SampleRate(0),
	m_Channels(0)
{
	return;
}

CSegmentParser::~CSegmentParser()
{
	return;
}

bool CSegmentParser::Parse(std::istream& Input)
{
	// Reset the parser state
	m_ParserStatus=true;
	m_ParserMessage="";

	// Go through each line
	while(!Input.eof())
	{
		// Read the whitespace
		ReadWhitespace(Input);
		if(Input.eof())
		{
			break;
		}

		// Get one character to figure out what kind of line this is
		CSegment Segment;
		char Char;
		Char=Input.get();
		Input.putback(Char);

		// Base our decision on whether it starts with a letter or a number
		if(isdigit(Char))
		{
			if(!ReadOffsetSizeLine(Input, Segment))
			{
				return false;
			}
		}
		else if(isalpha(Char))
		{
			if(!ReadKeywordLine(Input, Segment))
			{
				return false;
			}
		}
		else if(Char==13 || Char==10)
		{
			// Empty line are acceptable
		}
		else
		{
			m_ParserMessage=std::string("Unexpected first character '") + Char + "'. Needs to be a number or a digit.";
			m_ParserStatus=false;
			return false;
		}

		// Add the segment
		if(Segment.GetSize()>0)
		{
			AppendSegment(Segment);
		}

		// Read the end of the line
		if(!ReadEndOfLine(Input))
		{
			return false;
		}
	}
	return true;
}

std::string CSegmentParser::GetParserMessage() const
{
	return m_ParserMessage;
}

std::string CSegmentParser::GetFile() const
{
	return m_File;
}

std::string CSegmentParser::GetName() const
{
	return m_Name;
}

unsigned long CSegmentParser::GetSampleRate() const
{
	return m_SampleRate;
}

unsigned char CSegmentParser::GetChannels() const
{
	return m_Channels;
}

unsigned long CSegmentParser::GetSegmentCount() const
{
	return (unsigned long)m_Segments.size();
}

CSegment CSegmentParser::GetSegment(unsigned long Index) const
{
	assert(Index<GetSegmentCount());
	return m_Segments[Index];
}

void CSegmentParser::AppendSegment(const CSegment& Segment)
{
	m_Segments.push_back(Segment);
	return;
}

bool CSegmentParser::ReadWhitespace(std::istream& Input)
{
	while(!Input.eof())
	{
		// Read a character
		char Char;
		Char=Input.get();

		// Do something with it
		if(Char==9 || Char==32)
		{
			// Toss it
		}
		else
		{
			Input.putback(Char);
			return true;
		}
	}
	return true;
}

bool CSegmentParser::ReadEndOfLine(std::istream& Input)
{
	// Check for end of file
	if(Input.eof())
	{
		return true;
	}

	// Read a character
	char Char;
	Char=Input.get();

	// Do something with it
	if(Char==10 || Char==13)
	{
		// Check if there is another byte
		Char=Input.get();
		if(Char!=10 && Char!=13)
		{
			Input.putback(Char);
		}
		return true;
	}
	m_ParserMessage=std::string("Error: Unexpected character. Was '") + Char + "' and should have been a new line.";
	m_ParserStatus=false;
	return false;
}

bool CSegmentParser::ReadToken(std::istream& Input, std::string& Token, bool WhitespaceAllowed)
{
	// Append each char to the string until a whitespace or end of line
	Token.clear();
	while(!Input.eof())
	{
		// Read a character
		char Char;
		Char=Input.get();

		// Do something with it
		if(Char==9 || Char==32)
		{
			if(WhitespaceAllowed)
			{
				Token.append(&Char);
			}
			else
			{
				Input.putback(Char);
				return true;
			}
		}
		else if(Char==10 || Char==13)
		{
			Input.putback(Char);
			return true;
		}
		else
		{
			Token.append(1, Char);
		}
	}
	return true;
}

bool CSegmentParser::ReadKeywordLine(std::istream& Input, CSegment& Segment)
{
	// Check for end of file
	if(Input.eof())
	{
		return true;
	}

	// Read the keyword in
	std::string Keyword;
	ReadWhitespace(Input);
	if(!ReadToken(Input, Keyword))
	{
		return false;
	}

	// Do something with it
	if(Keyword=="file")
	{
		// Read in the filename
		ReadWhitespace(Input);
		if(!ReadToken(Input, m_File, true))
		{
			return false;
		}
	}
	else if(Keyword=="name")
	{
		// Read in the description
		ReadWhitespace(Input);
		if(!ReadToken(Input, m_Name, true))
		{
			return false;
		}
	}
	else if(Keyword=="skip")
	{
		// Read in the segment with a skip flag
		ReadWhitespace(Input);
		Segment.SetSkip(true);
		if(!ReadOffsetSizeLine(Input, Segment))
		{
			return false;
		}
	}
	else if(Keyword=="samplerate" || Keyword=="sample_rate")
	{
		// Read in the sample rate
		std::string SampleRate;
		ReadWhitespace(Input);
		if(!ReadToken(Input, SampleRate))
		{
			return false;
		}
		m_SampleRate=atoi(SampleRate.c_str());
	}
	else if(Keyword=="channels")
	{
		// Read in the number of channels
		std::string Channels;
		ReadWhitespace(Input);
		if(!ReadToken(Input, Channels))
		{
			return false;
		}
		m_Channels=atoi(Channels.c_str());
	}
	else
	{
		m_ParserMessage=std::string("Error: Unknown keyword '") + Keyword + "'.";
		m_ParserStatus=false;
		return false;
	}
	return true;
}

bool CSegmentParser::ReadOffsetSizeLine(std::istream& Input, CSegment& Segment)
{
	// Check for end of file
	if(Input.eof())
	{
		return true;
	}

	// Read the offset in
	std::string Offset;
	ReadWhitespace(Input);
	if(!ReadToken(Input, Offset))
	{
		return false;
	}
	Segment.SetOffset(atol(Offset.c_str()));

	// Read the size in
	std::string Size;
	ReadWhitespace(Input);
	if(!ReadToken(Input, Size))
	{
		return false;
	}
	Segment.SetSize(atol(Size.c_str()));
	return true;
}
