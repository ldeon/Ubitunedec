// SegmentParser.h : Segment definition file parser
//

#pragma once
#include "Segment.h"

class CSegmentParser
{
protected:
	virtual bool ReadWhitespace(std::istream& Input);
	virtual bool ReadEndOfLine(std::istream& Input);
	virtual bool ReadToken(std::istream& Input, std::string& Token, bool WhitespaceAllowed=false);
	virtual bool ReadKeywordLine(std::istream& Input, CSegment& Segment);
	virtual bool ReadOffsetSizeLine(std::istream& Input, CSegment& Segment);

protected:
	bool m_ParserStatus;
	std::string m_ParserMessage;

	std::string m_File;
	std::string m_Name;
	std::vector<CSegment> m_Segments;
	unsigned long m_SampleRate;
	unsigned char m_Channels;

public:
	CSegmentParser();
	virtual ~CSegmentParser();

	virtual bool Parse(std::istream& Input);
	virtual std::string GetParserMessage() const;
	virtual std::string GetFile() const;
	virtual std::string GetName() const;
	virtual unsigned long GetSampleRate() const;
	virtual unsigned char GetChannels() const;
	virtual unsigned long GetSegmentCount() const;
	virtual CSegment GetSegment(unsigned long Index) const;
	virtual void AppendSegment(const CSegment& Segment);
};
