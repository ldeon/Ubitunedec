/*
 Output.cpp : Output a segment stream
*/

#include "Pch.h"

#include <wx/progdlg.h>
#include <wx/filename.h>

#include "Functionality/Output.h"
#include "Functionality/Segment.h"
#include "Functionality/SegmentStream.h"
#include "Decoding/WaveWriter.h"
#include "Decoding/AudioStream.h"
#include "Decoding/FileDataStream.h"
#include "Decoding/Version5Stream.h"
#include "Decoding/InterleavedStream.h"
#include "Decoding/Interleaved9Stream.h"
#include "Decoding/OldInterleavedStream.h"
#include "Decoding/OggVorbisStream.h"
#include "Decoding/RawCompressedStream.h"
#include "Decoding/RawPcmStream.h"

static wxString createLayerString(const NDecFunc::CSegment& segment)
{
	wxString layers;
	if (segment.GetLayers().size() > 0)
	{
		layers += wxT("_L");
		for (std::vector<unsigned long>::const_iterator iter = segment.GetLayers().begin();
			iter != segment.GetLayers().end();
			++iter)
		{
			layers += wxString::Format(wxT("%lu"), *iter + 1);
		}
	}
	return layers;
}

/**
 * Constructs a decent output file name from the given base file name.
 */
static wxString createFileName(const wxString& baseName, const wxString& forceExt,
	const NDecFunc::CSegment& segment, unsigned int index, unsigned int count)
{
	wxFileName fileName(baseName);
	wxString layers(createLayerString(segment));

	if (count != 1)
	{
		fileName.SetName(fileName.GetName() + wxString::Format(wxT("_%lu"), index) + layers);
	}

	if (!forceExt.IsEmpty())
	{
		fileName.SetExt(forceExt);
	}

	return fileName.GetFullPath();
}

bool NDecFunc::OutputConcatenated(const std::string Filename, const std::vector<CSegment*>& Segments)
{
	// Use a progress dialog box
	wxProgressDialog Progress(_("Splice to WAV..."), _("Initializing..."), \
		100, NULL, wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME);

	// Open the file
	Progress.Pulse(_("Opening the file..."));
	std::ofstream Output;
	Output.open(Filename.c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	if(!Output.is_open())
	{
		return false;
	}

	// Prepare the wave header
	Progress.Pulse(_("Writing the wave header..."));
	PrepareWaveHeader(Output);

	// Create the segment stream
	Progress.Pulse(_("Creating the segment stream..."));
	CSegmentStream Stream;
	for(std::vector<CSegment*>::const_iterator Iter=Segments.begin();Iter!=Segments.end();++Iter)
	{
		Stream.Add(*Iter);
	}

	// Create the buffers
	Progress.Pulse(_("Decoding audio..."));
	unsigned long NumberDecodedSamples=0;
	unsigned long OutputBufferLength=65536;
	short* OutputBuffer=new short[OutputBufferLength];

	// Do the loop
	while(true)
	{
		// Decode some
		unsigned long LocalSamples=OutputBufferLength;
		if(!Stream.Decode(OutputBuffer, LocalSamples))
		{
			if(LocalSamples)
			{
				NumberDecodedSamples+=LocalSamples;
				Output.write((char*)OutputBuffer, LocalSamples*2);
			}
			wxMessageBox(_("Had some problems decoding to an output file"));
			break;
		}

		// Check if any was decoded
		if(LocalSamples==0)
		{
			break;
		}

		// Write it to the output stream
		NumberDecodedSamples+=LocalSamples;
		Output.write((char*)OutputBuffer, LocalSamples*2);

		Progress.Update(Stream.GetProgess(100), _("Decoding audio..."));
	}

	// Clean up
	delete [] OutputBuffer;

	// Update the wave header
	Progress.Update(100, _("Finishing up..."));
	if(Stream.GetCurrentSegment())
	{
		CSegment* Segment=Stream.GetCurrentSegment();
		Output.seekp(0);
		WriteWaveHeader(Output, Segment->GetSampleRate(), 16, Segment->GetChannels(),\
			NumberDecodedSamples);
	}

	// Close the file
	Output.close();
	return true;
}

/**
 * Decode the segment into the given output file, updating the progress bar.
 * Returns false if the user hit cancel or the operation failed.
 */
static bool decodeFile(wxProgressDialog& progress, const std::string& outFilename,
	const NDecFunc::CSegment& segment, unsigned long index, unsigned long segmentCount)
{
	using namespace NDecFunc;

	// Open the file
	std::ofstream Output;
	Output.open(outFilename.c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	if(!Output.is_open())
	{
		wxMessageBox(_("Could not open the output file: ") + outFilename);
		return false;
	}

	// Prepare the wave header
	PrepareWaveHeader(Output);

	// Create the segment stream
	CSegmentStream Stream;
	Stream.Add(&segment);

	// Create the buffers
	unsigned long NumberDecodedSamples=0;
	unsigned long OutputBufferLength=65536;
	short* OutputBuffer=new short[OutputBufferLength];

	// Do the loop
	while(true)
	{
		// Decode some
		unsigned long LocalSamples=OutputBufferLength;
		if(!Stream.Decode(OutputBuffer, LocalSamples))
		{
			if(LocalSamples)
			{
				NumberDecodedSamples+=LocalSamples;
				Output.write((char*)OutputBuffer, LocalSamples*2);
			}
			wxMessageBox(_("Had some problems decoding to an output file"));
			break;
		}

		// Check if any was decoded
		if(LocalSamples==0)
		{
			break;
		}

		// Write it to the output stream
		NumberDecodedSamples+=LocalSamples;
		Output.write((char*)OutputBuffer, LocalSamples*2);

		if (!progress.Update(index * 100 / segmentCount + Stream.GetProgess(100 / segmentCount),
			wxString::Format(_("Decoding audio %lu/%lu..."), index + 1, segmentCount)))
		{
			delete [] OutputBuffer;
			Output.close();
			return false;
		}
	}

	// Write the wave header
	Output.seekp(0);
	WriteWaveHeader(Output, segment.GetSampleRate(), 16, segment.GetChannels(),
		NumberDecodedSamples);

	// Clean up
	delete [] OutputBuffer;
	Output.close();
	return true;
}

/**
 * Layer extract the segment into the given output file, updating the progress bar.
 * Returns false if the user hit cancel or the operation failed.
 */
static bool extractFile(wxProgressDialog& progress, const std::string& outFilename,
	const NDecFunc::CSegment& segment, unsigned long index, unsigned long segmentCount)
{
	using namespace NDecFunc;

	// Open the input stream
	std::ifstream Input;
	Input.open(segment.GetFilename().c_str(), std::ios_base::in | std::ios_base::binary);
	if(!Input.is_open())
	{
		wxMessageBox(_("Could not open the input file: ") + segment.GetFilename());
		return false;
	}

	// Open the output file
	std::ofstream Output;
	Output.open(outFilename.c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	if(!Output.is_open())
	{
		wxMessageBox(_("Could not open the output file: ") + outFilename);
		return false;
	}

	// Determine the type
	EUbiFormat Type;
	Input.seekg(segment.GetOffset());
	Type=segment.GetType();

	// Set up an input stream
	CFileDataStream FileStream(&Input, segment.GetOffset(), segment.GetSize());

	// Get the layer
	unsigned long Layer;
	if (Type!=EUF_UBI_IV2 && Type!=EUF_UBI_IV8 && Type!=EUF_UBI_IV9)
	{
		// Do a generic copy
		while (!FileStream.IsEnd())
		{
			unsigned long Read=65536;
			char* Buffer;
			Buffer=(char*)FileStream.Read(Read);
			Output.write(Buffer, Read);
		}
	}
	else
	{
		// Set the layer
		Layer = segment.GetLayers()[0];

		// Do the layer extract
		try
		{
			if (Type==EUF_UBI_IV8)
			{
				if (!CInterleavedStream::LayerExtract(&FileStream, Layer, Output))
				{
					return false;
				}
			}
			else if (Type==EUF_UBI_IV9)
			{
				if (!CInterleaved9Stream::LayerExtract(&FileStream, Layer, Output))
				{
					return false;
				}
			}
			else if (Type==EUF_UBI_IV2)
			{
				if (!COldInterleavedStream::LayerExtract(&FileStream, Layer, Output))
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
	}

	// Clean up
	Input.close();
	Output.close();
	return true;
}

bool NDecFunc::OutputSeparate(const std::string BaseName, const std::vector<CSegment*>& Segments)
{
	// Use a progress dialog box
	wxProgressDialog Progress(_("Save to WAV(s)..."), _("Initializing..."), \
		100, NULL, wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME | \
		wxPD_CAN_ABORT);

	// We want to number these well
	std::streamoff lastOffset = -1;
	unsigned int lastIndex = 0;

	// Process each segment
	unsigned long i=0;
	for(std::vector<CSegment*>::const_iterator Iter=Segments.begin();Iter!=Segments.end();++Iter)
	{
		// Get the segment
		const CSegment& Segment=*(*Iter);

		// Update the progress dialog
		if (!Progress.Update(i*100/Segments.size(), wxString::Format(_("Decoding audio %lu/%lu..."), i+1, Segments.size())))
		{
			return false;
		}

		if (Segment.GetOffset() != lastOffset)
		{
			lastOffset = Segment.GetOffset();
			lastIndex++;
		}

		std::string Filename = createFileName(BaseName, "", Segment, lastIndex, Segments.size());

		if (!decodeFile(Progress, Filename, Segment, i, Segments.size()))
		{
			return false;
		}

		// Update
		i++;
	}

	// Finish up
	Progress.Update(100, _("Finishing up..."));
	return true;
}

bool NDecFunc::OutputLayerExtract(const std::string BaseName, const std::vector<CSegment*>& Segments, const wxString& OutputFilename)
{
	// Use a progress dialog box
	wxProgressDialog Progress(_("Extract As..."), _("Initializing..."), \
		100, NULL, wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME | \
		wxPD_CAN_ABORT);

	// We want to number these well
	std::streamoff lastOffset = -1;
	unsigned int lastIndex = 0;

	// Process each segment
	unsigned long i=0;
	for(std::vector<CSegment*>::const_iterator Iter=Segments.begin();Iter!=Segments.end();++Iter)
	{
		// Get the segment
		const CSegment& Segment=*(*Iter);

		// Update the progress dialog
		if (!Progress.Update(i*100/Segments.size(), wxString::Format(_("Extracting audio %lu/%lu..."), i+1, Segments.size())))
		{
			return false;
		}
		
		if (Segment.GetOffset() != lastOffset)
		{
			lastOffset = Segment.GetOffset();
			lastIndex++;
		}

		// Get the appropriate filename
		std::string Filename = createFileName(BaseName, "", Segment, lastIndex, Segments.size());

		if (!extractFile(Progress, Filename, Segment, i, Segments.size()))
		{
			return false;
		}

		// Update
		i++;
	}

	// Finish up
	Progress.Update(100, _("Finishing up..."));
	return true;
}

enum EBestAction
{
	EBA_DECODE,
	EBA_EXTRACT
};

bool NDecFunc::OutputBest(const std::string BaseName, const std::vector<CSegment*>& Segments)
{
	// Use a progress dialog box
	wxProgressDialog Progress(_("Save As..."), _("Initializing..."), \
		100, NULL, wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME | \
		wxPD_CAN_ABORT);

	// We want to number these well
	std::streamoff lastOffset = -1;
	unsigned int lastIndex = 0;

	// Process each segment
	unsigned long i=0;
	for(std::vector<CSegment*>::const_iterator Iter=Segments.begin();Iter!=Segments.end();++Iter)
	{
		// Get the segment
		const CSegment& Segment=*(*Iter);
		
		if (Segment.GetOffset() != lastOffset)
		{
			lastOffset = Segment.GetOffset();
			lastIndex++;
		}

		// Determine whether the best course of action would be to extract the
		// segment or decode it.

		EBestAction act;
		wxString extension;

		switch (Segment.GetType())
		{
		case EUF_OGG:
			act = EBA_EXTRACT;
			extension = "ogg";
			break;

		case EUF_UBI_IV2:
		case EUF_UBI_IV8:
		case EUF_UBI_IV9:
			if (Segment.GetLayerTypes().size() == 1)
			{
				switch (Segment.GetLayerTypes().at(0))
				{
				case EUF_OGG:
					act = EBA_EXTRACT;
					extension = "ogg";
					break;

				default:
					act = EBA_DECODE;
					extension = "wav";
					break;
				}
			}
			else
			{
				act = EBA_DECODE;
				extension = "wav";
			}
			break;

		default:
			act = EBA_DECODE;
			extension = "wav";
			break;
		}

		if (act == EBA_EXTRACT)
		{
			// Update the progress dialog
			if (!Progress.Update(i*100/Segments.size(), wxString::Format(_("Extracting audio %lu/%lu..."), i+1, Segments.size())))
			{
				return false;
			}

			// Get the appropriate filename
			std::string Filename = createFileName(BaseName, extension, Segment, lastIndex, Segments.size());

			if (!extractFile(Progress, Filename, Segment, i, Segments.size()))
			{
				return false;
			}
		}
		else if (act == EBA_DECODE)
		{
			// Update the progress dialog
			if (!Progress.Update(i*100/Segments.size(), wxString::Format(_("Decoding audio %lu/%lu..."), i+1, Segments.size())))
			{
				return false;
			}

			std::string Filename = createFileName(BaseName, extension, Segment, lastIndex, Segments.size());

			if (!decodeFile(Progress, Filename, Segment, i, Segments.size()))
			{
				return false;
			}
		}

		// Update
		i++;
	}

	// Finish up
	Progress.Update(100, _("Finishing up..."));
	return true;
}