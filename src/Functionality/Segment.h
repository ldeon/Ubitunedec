/*
 Segment.h : A segment definition
*/

#pragma once
#include "Decoding\UbiFormats.h"

class CAudioStream;
class CLayeredAudioStream;

namespace NDecGui
{
	class CSegmentsListView;
};

namespace NDecFunc
{
	class CSegment
	{
	protected:
		std::string m_Filename;
		std::streamoff m_Offset;
		std::streamsize m_Size;
		EUbiFormat m_Type;
		unsigned char m_Channels;
		unsigned long m_SampleRate;
		std::vector<unsigned long> m_Layers;
		std::vector<EUbiFormat> m_LayerTypes;
		NDecGui::CSegmentsListView* m_ListView;
		unsigned long m_ListViewIndex;

	public:
		CSegment();
		CSegment(std::string Filename, std::streamoff Offset, std::streamsize Size, \
			EUbiFormat Type=EUF_NULL, unsigned char Channels=0, unsigned long SampleRate=0);
		CSegment(const CSegment& Object);
		~CSegment();
		//virtual operator=

		void SetFilename(std::string Filename);
		std::string GetFilename() const;
		void SetOffset(std::streamoff Offset);
		std::streamoff GetOffset() const;
		void SetSize(std::streamsize Size);
		std::streamsize GetSize() const;
		void SetType(EUbiFormat Type);
		EUbiFormat GetType() const;
		void SetChannels(unsigned char Channels);
		unsigned char GetChannels() const;
		void SetSampleRate(unsigned long SampleRate);
		unsigned long GetSampleRate() const;
		std::vector<unsigned long>& GetLayers();
		const std::vector<unsigned long>& GetLayers() const;
		std::vector<EUbiFormat>& GetLayerTypes();
		const std::vector<EUbiFormat>& GetLayerTypes() const;
		void SetListView(NDecGui::CSegmentsListView* ListView);
		NDecGui::CSegmentsListView* GetListView() const;
		void SetListViewIndex(unsigned long Index);
		unsigned long GetListViewIndex() const;
	};
};
