/*
 SegmentsList.cpp : A list of segments
*/

#include "Pch.h"

#include <wx/progdlg.h>
#include <wx/filename.h>

#include "Functionality/SegmentsList.h"
#include "Functionality/Segment.h"
#include "Decoding/AudioStream.h"
#include "Decoding/FileDataStream.h"
#include "Decoding/Version5Stream.h"
#include "Decoding/InterleavedStream.h"
#include "Decoding/Interleaved9Stream.h"
#include "Decoding/OldInterleavedStream.h"
#include "Decoding/Old6Or4BitStream.h"
#include "Decoding/OggVorbisStream.h"
#include "Decoding/RawCompressedStream.h"
#include "Decoding/RawPcmStream.h"
#include "Decoding/Scan.h"
#include "Decoding/UbiFormats.h"

// CSegmentsList Implementation
NDecFunc::CSegmentsList::CSegmentsList() :
	m_Filename("")
{
	return;
}

NDecFunc::CSegmentsList::~CSegmentsList()
{
	Clear();
	return;
}

void NDecFunc::CSegmentsList::SetFilename(std::string Filename)
{
	m_Filename=Filename;
	return;
}

std::string NDecFunc::CSegmentsList::GetFilename() const
{
	return m_Filename;
}

class GuiScanCallback : public ScanCallback
{
public:

	GuiScanCallback(wxProgressDialog& progDlg):
	  progDlg(progDlg)
	{
	}

	virtual bool progress(int percent)
	{
		return progDlg.Update(percent, _("Scanning..."));
	}

	virtual bool foundSegment(std::streamoff offset, std::streamsize size)
	{
		segments.push_back(Segment(offset, size));
		return true;
	}

	// Keep track of the found
	typedef std::pair<std::streamoff, std::streamsize> Segment;
	typedef std::vector<Segment> SegmentVector;
	SegmentVector segments;

private:
	wxProgressDialog& progDlg;
};

void NDecFunc::CSegmentsList::ScanFile()
{
	// Get a short filename
	wxFileName Filename(m_Filename);

	// Use a progress dialog box
	wxProgressDialog Progress(wxString::Format(_("Scanning %s ..."), Filename.GetFullName()), _("Initializing..."), \
		100, NULL, wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME | \
		wxPD_CAN_ABORT);

	// Open the input file
	std::ifstream Input;
	std::streamsize StreamSize;
	Input.open(m_Filename.c_str(), std::ios_base::in | std::ios_base::binary);
	if(!Input.is_open())
	{
		return;
	}
	Input.seekg(0, std::ios_base::end);
	StreamSize=Input.tellg();
	Input.seekg(0);

	// Do the scan
	GuiScanCallback callback(Progress);
	ScanAndList(Input, StreamSize, callback);

	// Close the input file
	Input.close();

	// Add the segments
	for(GuiScanCallback::SegmentVector::const_iterator Iter = callback.segments.begin(); Iter != callback.segments.end(); ++Iter)
	{
		CSegment* NewSegment;
		NewSegment=CreateSegment(Iter->first, Iter->second);
		if(NewSegment)
		{
			unsigned long InsertPos;
			std::vector<unsigned long> Sel;
			InsertPos=Insert(NewSegment);
			Sel.push_back(InsertPos);
			if(CanUnmix(Sel))
			{
				Unmix(Sel);
			}
		}
	}
	return;
}

NDecFunc::CSegment* NDecFunc::CSegmentsList::CreateSegment(std::streamoff Offset, std::streamsize Size) const
{
	// Allocate data
	CSegment* Segment=new NDecFunc::CSegment(m_Filename, Offset, Size);

	// Create a stream
	std::ifstream Input;
	Input.open(m_Filename.c_str(), std::ios_base::in | std::ios_base::binary);
	if(!Input.is_open())
	{
		return NULL;
	}

	// Autodetect type
	try
	{
		Input.seekg(Offset);
		Segment->SetType(DetermineFormat(Input, Size));
	}
	catch (...)
	{
		return NULL;
	}

	// Create an audio input data stream
	CFileDataStream InputStream(&Input, Segment->GetOffset(), Segment->GetSize());

	// Create an audio output stream for each type
	if(Segment->GetType()==EUF_UBI_V3 || Segment->GetType()==EUF_UBI_V5 || Segment->GetType()==EUF_UBI_V6)
	{
		CVersion5Stream Stream(&InputStream);

		// Try to initialize the header
		try
		{
			if(!Stream.InitializeHeader(Segment->GetSampleRate()))
			{
				// TODO: Better error handling
				return NULL;
			}
			Segment->SetSampleRate(Stream.GetSampleRate());
			Segment->SetChannels(Stream.GetChannels());
		}
		// TODO: Better error handling
		catch(...)
		{
			// TODO: Better error handling
			return NULL;
		}
	}
	else if(Segment->GetType()==EUF_UBI_IV8)
	{
		// Decode the stream
		CInterleavedStream Stream(&InputStream);

		// Try to initialize the header
		try
		{
			if(Segment->GetChannels()==0)
			{
				Segment->SetChannels(2);
			}
			if(!Stream.InitializeHeader(Segment->GetSampleRate(), Segment->GetChannels()))
			{
				// TODO: Better error handling
				return NULL;
			}
			Segment->SetSampleRate(Stream.GetSampleRate());
			Segment->SetChannels(Stream.GetChannels());
			for(unsigned long i=0;i<Stream.GetLayerCount();i++)
			{
				Segment->GetLayerTypes().push_back(Stream.GetType(i));
				Segment->GetLayers().push_back(i);
			}
		}
		// TODO: Better error handling
		catch(...)
		{
			// TODO: Better error handling
			return NULL;
		}
	}
	else if(Segment->GetType()==EUF_UBI_IV9)
	{
		// Decode the stream
		CInterleaved9Stream Stream(&InputStream);

		// Try to initialize the header
		try
		{
			if(Segment->GetChannels()==0)
			{
				Segment->SetChannels(2);
			}
			if(!Stream.InitializeHeader(Segment->GetSampleRate(), Segment->GetChannels()))
			{
				// TODO: Better error handling
				return NULL;
			}
			Segment->SetSampleRate(Stream.GetSampleRate());
			Segment->SetChannels(Stream.GetChannels());
			for(unsigned long i=0;i<Stream.GetLayerCount();i++)
			{
				Segment->GetLayerTypes().push_back(Stream.GetType(i));
				Segment->GetLayers().push_back(i);
			}
		}
		// TODO: Better error handling
		catch(...)
		{
			// TODO: Better error handling
			return NULL;
		}
	}
	else if(Segment->GetType()==EUF_UBI_IV2)
	{
		// Decode the stream
		COldInterleavedStream Stream(&InputStream);

		// Try to initialize the header
		try
		{
			if(!Stream.InitializeHeader(Segment->GetSampleRate()))
			{
				// TODO: Better error handling
				return NULL;
			}
			Segment->SetSampleRate(Stream.GetSampleRate());
			Segment->SetChannels(Stream.GetChannels());
			for(unsigned long i=0;i<Stream.GetLayerCount();i++)
			{
				Segment->GetLayerTypes().push_back(Stream.GetType(i));
				Segment->GetLayers().push_back(i);
			}
		}
		// TODO: Better error handling
		catch(...)
		{
			// TODO: Better error handling
			return NULL;
		}
	}
	else if(Segment->GetType()==EUF_UBI_6OR4)
	{
		COld6Or4BitStream Stream(&InputStream);

		// Try to initialize the header
		try
		{
			if(!Stream.InitializeHeader())
			{
				// TODO: Better error handling
				return NULL;
			}
			Segment->SetSampleRate(Stream.GetSampleRate());
			Segment->SetChannels(Stream.GetChannels());
		}
		// TODO: Better error handling
		catch(...)
		{
			// It's okay to let these ones slip through
			Segment->SetSampleRate(Stream.GetSampleRate());
			Segment->SetChannels(Stream.GetChannels());
		}
	}
	else if(Segment->GetType()==EUF_OGG)
	{
		COggVorbisStream Stream(&InputStream);

		// Try to initialize the header
		try
		{
			if(!Stream.InitializeHeader())
			{
				// TODO: Better error handling
				return NULL;
			}
			Segment->SetSampleRate(Stream.GetSampleRate());
			Segment->SetChannels(Stream.GetChannels());
		}
		// TODO: Better error handling
		catch(...)
		{
			// TODO: Better error handling
			return NULL;
		}
	}
	else if(Segment->GetType()==EUF_RAW)
	{
		// Decode the stream
		CRawPcmStream Stream(&InputStream);

		// Try to initialize the header
		try
		{
			if(!Segment->GetSampleRate())
			{
				Segment->SetSampleRate(44100);
			}
			if(!Segment->GetChannels())
			{
				Segment->SetChannels(2);
			}
			if(!Stream.InitializeHeader(Segment->GetChannels()))
			{
				// TODO: Better error handling
				return NULL;
			}
			Segment->SetChannels(Stream.GetChannels());
		}
		// TODO: Better error handling
		catch(...)
		{
			// TODO: Better error handling
			return NULL;
		}
	}
	return Segment;
}

void NDecFunc::CSegmentsList::Add(CSegment* Segment)
{
	if(Segment)
	{
		push_back(Segment);
	}
	return;
}

unsigned long NDecFunc::CSegmentsList::Insert(CSegment* Segment)
{
	// Check the segment to make sure it's valid
	if(!Segment)
	{
		return -1;
	}

	// Find a good place for this based on the offset
	for(unsigned long i=0;i<GetCount();i++)
	{
		if(Segment->GetOffset()<at(i)->GetOffset())
		{
			iterator Iter=begin()+i;
			insert(Iter, Segment);
			return i;
		}
	}
	push_back(Segment);
	return size()-1;
}

void NDecFunc::CSegmentsList::Remove(unsigned long Index)
{
	if(!IsValid(Index))
	{
		return;
	}

	iterator Iter=begin()+Index;
	delete *Iter;
	*Iter=NULL;
	erase(Iter);
	return;
}

void NDecFunc::CSegmentsList::Remove(const std::vector<unsigned long>& Indicies)
{
	for(std::vector<unsigned long>::const_iterator Iter=Indicies.begin();Iter!=Indicies.end();++Iter)
	{
		if(IsValid(*Iter))
		{
			iterator Iter2=begin()+(*Iter);
			delete *Iter2;
			*Iter2=NULL;
		}
	}
	for(iterator Iter=begin();Iter!=end();++Iter)
	{
		if(*Iter==NULL)
		{
			erase(Iter);
			--Iter;
		}
	}
	return;
}

void NDecFunc::CSegmentsList::Duplicate(unsigned long Index)
{
	std::vector<unsigned long> Indicies;
	Indicies.push_back(Index);
	Duplicate(Indicies);
	return;
}

void NDecFunc::CSegmentsList::Duplicate(const std::vector<unsigned long>& Indicies)
{
	std::vector<unsigned long> New(Indicies);

	// We have to do this the hard way
	for(std::vector<unsigned long>::iterator Iter=New.begin();Iter!=New.end();++Iter)
	{
		unsigned long Index=*Iter;

		if(!IsValid(Index))
		{
			continue;
		}

		// Duplicate the item
		CSegment* NewSegment;
		NewSegment=new CSegment(*at(Index));

		// Insert it
		Index++;
		iterator NewIter=begin()+Index;
		insert(NewIter, NewSegment);

		// Update indicies
		for(std::vector<unsigned long>::iterator Iter2=New.begin();Iter2!=New.end();++Iter2)
		{
			if(*Iter2>=Index)
			{
				(*Iter2)++;
			}
		}
	}
	return;
}

typedef std::map<std::streamoff, std::vector<unsigned long> > TMixMap;

bool NDecFunc::CSegmentsList::CanMix(const std::vector<unsigned long>& Indicies) const
{
	// Some variables
	TMixMap ToMix;

	// Figure out what to mix
	for(std::vector<unsigned long>::const_iterator Iter=Indicies.begin();Iter!=Indicies.end();++Iter)
	{
		unsigned long Index=*Iter;

		// Make sure it's valid
		if(!IsValid(Index))
		{
			continue;
		}

		// Make sure it's an interleaved stream
		CSegment& Segment=*at(Index);
		if(Segment.GetType()!=EUF_UBI_IV2 && Segment.GetType()!=EUF_UBI_IV8 && \
			Segment.GetType()!=EUF_UBI_IV9)
		{
			continue;
		}

		// Now add it
		if(ToMix.find(Segment.GetOffset())==ToMix.end())
		{
			TMixMap::value_type Value(Segment.GetOffset(), std::vector<unsigned long>());
			ToMix.insert(Value);
		}
		ToMix[Segment.GetOffset()].push_back(Index);
	}

	// Figure out if there is any
	for(TMixMap::const_iterator Iter=ToMix.begin();Iter!=ToMix.end();++Iter)
	{
		const std::vector<unsigned long>& Segments=Iter->second;
		if(Segments.size()>1)
		{
			return true;
		}
	}
	return false;
}

bool NDecFunc::CSegmentsList::CanUnmix(const std::vector<unsigned long>& Indicies) const
{
	// Figure out what to unmix
	for(std::vector<unsigned long>::const_iterator Iter=Indicies.begin();Iter!=Indicies.end();++Iter)
	{
		unsigned long Index=*Iter;

		// Make sure it's valid
		if(!IsValid(Index))
		{
			continue;
		}

		// Make sure it's an interleaved stream
		CSegment& Segment=*at(Index);
		if(Segment.GetType()!=EUF_UBI_IV2 && Segment.GetType()!=EUF_UBI_IV8 && \
			Segment.GetType()!=EUF_UBI_IV9)
		{
			continue;
		}

		// Make sure it has more that one layer
		if(Segment.GetLayers().size()<2)
		{
			continue;
		}

		// We have it
		return true;
	}
	return false;
}

void NDecFunc::CSegmentsList::Mix(const std::vector<unsigned long>& Indicies)
{
	// Some variables
	TMixMap ToMix;

	// Figure out what to mix
	for(std::vector<unsigned long>::const_iterator Iter=Indicies.begin();Iter!=Indicies.end();++Iter)
	{
		unsigned long Index=*Iter;

		// Make sure it's valid
		if(!IsValid(Index))
		{
			continue;
		}

		// Make sure it's an interleaved stream
		CSegment& Segment=*at(Index);
		if(Segment.GetType()!=EUF_UBI_IV2 && Segment.GetType()!=EUF_UBI_IV8 && \
			Segment.GetType()!=EUF_UBI_IV9)
		{
			continue;
		}

		// Now add it
		if(ToMix.find(Segment.GetOffset())==ToMix.end())
		{
			TMixMap::value_type Value(Segment.GetOffset(), std::vector<unsigned long>());
			ToMix.insert(Value);
		}
		ToMix[Segment.GetOffset()].push_back(Index);
	}

	// Mix the layers
	for(TMixMap::const_iterator Iter=ToMix.begin();Iter!=ToMix.end();++Iter)
	{
		const std::vector<unsigned long>& Segments=Iter->second;
		
		// Make sure there are at least two
		if(Segments.size()<2)
		{
			continue;
		}

		// Check
		if(!at(Segments[0]))
		{
			continue;
		}

		// Merge them
		CSegment& FirstSegment=*at(Segments[0]);
		for(std::vector<unsigned long>::const_iterator Iter2=Segments.begin()+1;Iter2!=Segments.end();++Iter2)
		{
			// Check
			if(!at(*Iter2))
			{
				continue;
			}

			const CSegment& seg = *at(*Iter2);

			// Add layers
			const std::vector<unsigned long>& CurrentLayers=seg.GetLayers();
			for(std::vector<unsigned long>::const_iterator Iter3=CurrentLayers.begin();Iter3!=CurrentLayers.end();++Iter3)
			{
				FirstSegment.GetLayers().push_back(*Iter3);
			}
			std::vector<EUbiFormat>::const_iterator TypesIter = seg.GetLayerTypes().begin();
			for(; TypesIter != seg.GetLayerTypes().end(); ++TypesIter)
			{
				FirstSegment.GetLayerTypes().push_back(*TypesIter);
			}

			// Delete
			delete at(*Iter2);
			(*this)[*Iter2]=NULL;
		}
	}

	// Remove ones marked for deletion
	// TODO This is somewhat hacky, oh well
	std::vector<CSegment*> remaining;
	for(iterator Iter = begin(); Iter != end(); ++Iter)
	{
		if(*Iter)
			remaining.push_back(*Iter);
	}
	clear();
	for (std::vector<CSegment*>::const_iterator Iter = remaining.begin(); Iter != remaining.end(); ++Iter)
	{
		push_back(*Iter);
	}
	return;
}

void NDecFunc::CSegmentsList::Unmix(const std::vector<unsigned long>& Indicies)
{
	// This is just like duplicate, we have to create our own copy
	std::vector<unsigned long> New(Indicies);

	// Figure out what to unmix
	for(std::vector<unsigned long>::const_iterator Iter=New.begin();Iter!=New.end();++Iter)
	{
		unsigned long Index=*Iter;

		// Make sure it's valid
		if(!IsValid(Index))
		{
			continue;
		}

		// Make sure it's an interleaved stream
		CSegment& Segment=*at(Index);
		if(Segment.GetType()!=EUF_UBI_IV2 && Segment.GetType()!=EUF_UBI_IV8 && \
			Segment.GetType()!=EUF_UBI_IV9)
		{
			continue;
		}

		// Make sure it has more that one layer
		if(Segment.GetLayers().size()<2)
		{
			continue;
		}

		// Create duplicates
		unsigned long NumberExtraSegments=Segment.GetLayers().size()-1;
		unsigned long InsertPos=Index+1;
		std::vector<EUbiFormat>::const_iterator TypesIter = Segment.GetLayerTypes().begin();
		for(std::vector<unsigned long>::const_iterator Iter2=Segment.GetLayers().begin();Iter2!=Segment.GetLayers().end();++Iter2)
		{
			// Duplicate the item
			CSegment* NewSegment;
			NewSegment=new CSegment(Segment);
			NewSegment->GetLayers().clear();
			NewSegment->GetLayers().push_back(*Iter2);
			NewSegment->GetLayerTypes().clear();
			NewSegment->GetLayerTypes().push_back(*TypesIter);
			++TypesIter;

			// Insert it
			iterator InsertIter=begin()+InsertPos;
			insert(InsertIter, NewSegment);
			InsertPos++;
		}

		// Delete the original segment
		delete at(Index);
		(*this)[Index]=NULL;
		erase(begin()+Index);

		// Update indicies
		for(std::vector<unsigned long>::iterator Iter2=New.begin();Iter2!=New.end();++Iter2)
		{
			if(*Iter2>=Index)
			{
				(*Iter2)+=NumberExtraSegments;
			}
		}
	}
	return;
}

NDecFunc::CSegment* NDecFunc::CSegmentsList::Get(unsigned long Index)
{
	if(!IsValid(Index))
	{
		return NULL;
	}
	return (*this)[Index];
}

bool NDecFunc::CSegmentsList::IsValid(unsigned long Index) const
{
	if(Index>=GetCount())
	{
		return false;
	}
	return true;
}

unsigned long NDecFunc::CSegmentsList::GetCount() const
{
	return size();
}

void NDecFunc::CSegmentsList::Clear()
{
	for(iterator Iter=begin();Iter!=end();++Iter)
	{
		delete *Iter;
		*Iter=NULL;
	}
	clear();
	return;
}
