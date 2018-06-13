// Segment.cpp : Segment definition
//

#include "Pch.h"
#include "Segment.h"

CSegment::CSegment() :
	m_Skip(false),
	m_Offset(0),
	m_Size(0)
{
	return;
}

CSegment::CSegment(std::streamoff Offset, std::streamsize Size, bool Skip) :
	m_Skip(Skip),
	m_Offset(Offset),
	m_Size(Size)
{
	return;
}

CSegment::CSegment(const CSegment& Obj) :
	m_Skip(Obj.GetSkip()),
	m_Offset(Obj.GetOffset()),
	m_Size(Obj.GetSize())
{
	return;
}

CSegment::~CSegment()
{
	return;
}

CSegment CSegment::operator=(const CSegment& Right)
{
	SetSkip(Right.GetSkip());
	SetOffset(Right.GetOffset());
	SetSize(Right.GetSize());
	return *this;
}

bool CSegment::GetSkip() const
{
	return m_Skip;
}

void CSegment::SetSkip(bool Skip)
{
	m_Skip=Skip;
	return;
}

std::streamoff CSegment::GetOffset() const
{
	return m_Offset;
}

void CSegment::SetOffset(std::streamoff Offset)
{
	m_Offset=Offset;
	return;
}

std::streamsize CSegment::GetSize() const
{
	return m_Size;
}

void CSegment::SetSize(std::streamsize Size)
{
	m_Size=Size;
	return;
}
