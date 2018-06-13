// Segment.h : Segment definition
//

#pragma once

class CSegment
{
protected:
	bool m_Skip;
	std::streamoff m_Offset;
	std::streamsize m_Size;

public:
	CSegment();
	CSegment(std::streamoff Offset, std::streamsize Size, bool Skip=false);
	CSegment(const CSegment& Obj);
	~CSegment();
	CSegment operator=(const CSegment& Right);

	bool GetSkip() const;
	void SetSkip(bool Skip);
	std::streamoff GetOffset() const;
	void SetOffset(std::streamoff Offset);
	std::streamsize GetSize() const;
	void SetSize(std::streamsize Size);
};
