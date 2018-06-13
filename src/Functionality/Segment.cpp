/*
 Segment.cpp : A segment definition
*/

#include "Pch.h"

#include "Functionality/Segment.h"
#include "Decoding/UbiFormats.h"

// CSegment Implementation
NDecFunc::CSegment::CSegment() :
	m_Filename(""),
	m_Offset(0),
	m_Size(0),
	m_Type(EUF_NULL),
	m_Channels(0),
	m_SampleRate(0),
	m_ListView(NULL),
	m_ListViewIndex(0)
{
	return;
}

NDecFunc::CSegment::CSegment(std::string Filename, std::streamoff Offset, std::streamsize Size, \
							 EUbiFormat Type, unsigned char Channels, unsigned long SampleRate) :
	m_Filename(Filename),
	m_Offset(Offset),
	m_Size(Size),
	m_Type(Type),
	m_Channels(Channels),
	m_SampleRate(SampleRate),
	m_ListView(NULL),
	m_ListViewIndex(0)
{
	return;
}

NDecFunc::CSegment::CSegment(const CSegment& Object) :
	m_Filename(Object.m_Filename),
	m_Offset(Object.m_Offset),
	m_Size(Object.m_Size),
	m_Type(Object.m_Type),
	m_Channels(Object.m_Channels),
	m_SampleRate(Object.m_SampleRate),
	m_Layers(Object.m_Layers),
	m_LayerTypes(Object.m_LayerTypes),
	m_ListView(Object.m_ListView),
	m_ListViewIndex(Object.m_ListViewIndex)
{
	return;
}

NDecFunc::CSegment::~CSegment()
{
	return;
}

void NDecFunc::CSegment::SetFilename(std::string Filename)
{
	m_Filename=Filename;
	return;
}

std::string NDecFunc::CSegment::GetFilename() const
{
	return m_Filename;
}

void NDecFunc::CSegment::SetOffset(std::streamoff Offset)
{
	m_Offset=Offset;
	return;
}

std::streamoff NDecFunc::CSegment::GetOffset() const
{
	return m_Offset;
}

void NDecFunc::CSegment::SetSize(std::streamsize Size)
{
	m_Size=Size;
	return;
}

std::streamsize NDecFunc::CSegment::GetSize() const
{
	return m_Size;
}

void NDecFunc::CSegment::SetType(EUbiFormat Type)
{
	m_Type=Type;
	return;
}

EUbiFormat NDecFunc::CSegment::GetType() const
{
	return m_Type;
}

void NDecFunc::CSegment::SetChannels(unsigned char Channels)
{
	m_Channels=Channels;
	return;
}

unsigned char NDecFunc::CSegment::GetChannels() const
{
	return m_Channels;
}

void NDecFunc::CSegment::SetSampleRate(unsigned long SampleRate)
{
	m_SampleRate=SampleRate;
	return;
}

unsigned long NDecFunc::CSegment::GetSampleRate() const
{
	return m_SampleRate;
}

std::vector<unsigned long>& NDecFunc::CSegment::GetLayers()
{
	return m_Layers;
}

const std::vector<unsigned long>& NDecFunc::CSegment::GetLayers() const
{
	return m_Layers;
}

std::vector<EUbiFormat>& NDecFunc::CSegment::GetLayerTypes()
{
	return m_LayerTypes;
}

const std::vector<EUbiFormat>& NDecFunc::CSegment::GetLayerTypes() const
{
	return m_LayerTypes;
}

void NDecFunc::CSegment::SetListView(NDecGui::CSegmentsListView* ListView)
{
	m_ListView=ListView;
	return;
}

NDecGui::CSegmentsListView* NDecFunc::CSegment::GetListView() const
{
	return m_ListView;
}

void NDecFunc::CSegment::SetListViewIndex(unsigned long Index)
{
	m_ListViewIndex=Index;
	return;
}

unsigned long NDecFunc::CSegment::GetListViewIndex() const
{
	return m_ListViewIndex;
}
