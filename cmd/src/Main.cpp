// Main.cpp : Defines the entry point for the console application
//

#include "Pch.h"
#include "UbiFormats.h"
#include "FileDataStream.h"
#include "Scan.h"
#include "SegmentParser.h"
#include "WaveWriter.h"
#include "Version.h"

// The action to be taken
enum EAction
{
	EACT_DECODE,
	EACT_SCAN,
	EACT_LAYEREXTRACT
};

// The arguments sent to the application
struct SArguments
{
	SArguments() :
		Action(EACT_DECODE),
		ShowBanner(true),
		ShowUsage(false),
		ShowInfo(false),
		InputFilename(""),
		InputOffset(0),
		InputSize(0),
		InputChannels(0),
		InputTypeForce(EUF_NULL),
		OutputFilename(""),
		OuputWaveFile(true),
		OutputSampleRate(0),
		SegmentFilename(""),
		CompLeftSample(0),
		CompLeftIndex(0),
		CompRightSample(0),
		CompRightIndex(0)
	{
	};

	EAction Action;
	bool ShowBanner;
	bool ShowUsage;
	bool ShowInfo;
	std::string InputFilename;
	std::streamoff InputOffset;
	std::streamsize InputSize;
	unsigned char InputChannels;
	EUbiFormat InputTypeForce;
	std::vector<unsigned long> InputLayers;
	std::string OutputFilename;
	bool OuputWaveFile;
	unsigned long OutputSampleRate;
	std::string SegmentFilename;
	short CompLeftSample;
	unsigned char CompLeftIndex;
	short CompRightSample;
	unsigned char CompRightIndex;
};

// Parse the arguments sent to the program
bool ParseArguments(SArguments& Args, unsigned long Argc, _TCHAR* Argv[])
{
	// Go through each of the arguments sent
	for(unsigned long i=1;i<Argc;)
	{
		std::string Arg(Argv[i++]);
		
		// Check it
		if(Arg=="-b" || Arg=="--no-banner")
		{
			Args.ShowBanner=false;
		}
		else if(Arg=="-D" || Arg=="--decode")
		{
			Args.Action=EACT_DECODE;
		}
		else if(Arg=="-S" || Arg=="--scan")
		{
			Args.Action=EACT_SCAN;
		}
		else if(Arg=="-L" || Arg=="--layer-extract")
		{
			Args.Action=EACT_LAYEREXTRACT;
		}
		else if(Arg=="-o" || Arg=="--output")
		{
			if(i>=Argc)
			{
				return false;
			}
			Args.OutputFilename=std::string(Argv[i++]);
		}
		else if(Arg=="-w" || Arg=="--wave")
		{
			Args.OuputWaveFile=true;
		}
		else if(Arg=="-r" || Arg=="--raw")
		{
			Args.OuputWaveFile=false;
		}
		else if(Arg=="-i" || Arg=="--offset")
		{
			if(i>=Argc)
			{
				return false;
			}
			Args.InputOffset=atol(Argv[i++]);
		}
		else if(Arg=="-s" || Arg=="--size")
		{
			if(i>=Argc)
			{
				return false;
			}
			Args.InputSize=atol(Argv[i++]);
		}
		else if(Arg=="-t" || Arg=="--stereo")
		{
			Args.InputChannels=2;
		}
		else if(Arg=="-m" || Arg=="--mono")
		{
			Args.InputChannels=1;
		}
		else if(Arg=="-a" || Arg=="--auto")
		{
			Args.InputChannels=0;
		}
		else if(Arg=="-l" || Arg=="--layer")
		{
			if(i>=Argc)
			{
				return false;
			}
			std::string LayerString(Argv[i++]);
			
			// Parse the string
			if(LayerString=="all")
			{
				for(unsigned long i=0;i<16;i++)
				{
					Args.InputLayers.push_back(i);
				}
			}
			else
			{
				std::string CurrentLayer;
				for(unsigned long i=0;i<LayerString.size();i++)
				{
					if(isdigit(LayerString[i]))
					{
						CurrentLayer.append(1, LayerString[i]);
					}
					else if(LayerString[i]==',')
					{
						unsigned long LayerInt;
						LayerInt=atoi(CurrentLayer.c_str())-1;
						Args.InputLayers.push_back(LayerInt);
						CurrentLayer.clear();
					}
					else
					{
						// Ignore the rest
					}
				}
				if(CurrentLayer.size())
				{
					unsigned long LayerInt;
					LayerInt=atoi(CurrentLayer.c_str())-1;
					Args.InputLayers.push_back(LayerInt);
					CurrentLayer.clear();
				}
			}
		}
		else if(Arg=="-g" || Arg=="--segments")
		{
			if(i>=Argc)
			{
				return false;
			}
			Args.SegmentFilename=std::string(Argv[i++]);
		}
		else if(Arg=="-n" || Arg=="--info")
		{
			Args.ShowInfo=true;
		}
		else if(Arg=="--input-type")
		{
			if(i>=Argc)
			{
				return false;
			}
			Args.InputTypeForce=StringToUbiFormat(Argv[i++]);
		}
		else if(Arg=="--sample-rate")
		{
			if(i>=Argc)
			{
				return false;
			}
			Args.OutputSampleRate=atoi(Argv[i++]);
		}
		else if(Arg=="--comp-left")
		{
			if(i+1>=Argc)
			{
				return false;
			}
			Args.CompLeftSample=atoi(Argv[i++]);
			Args.CompLeftIndex=atoi(Argv[i++]);
		}
		else if(Arg=="--comp-right")
		{
			if(i+1>=Argc)
			{
				return false;
			}
			Args.CompRightSample=atoi(Argv[i++]);
			Args.CompRightIndex=atoi(Argv[i++]);
		}
		else
		{
			Args.InputFilename=Arg;
		}
	}
	return true;
}

unsigned long GetNumberSampleClips(CAudioStream& Stream)
{
	// Some variables
	unsigned long Count=0;
	unsigned long NumberSamples=882000;
	short* Sample;
	Sample=new short[882000];

	// Decode as much as possible
	Stream.Decode(Sample, NumberSamples);

	// Check it
	for(unsigned long i=0;i<NumberSamples;i++)
	{
		if(Sample[i]>=32767 || Sample[i]<=-32768)
		{
			Count++;
		}
	}

	// Clean up
	delete[] Sample;
	return Count;
}

int Decode(SArguments& Args)
{
	// Check the arguments
	if(Args.InputFilename=="")
	{
		std::cerr << "Input file not specified." << std::endl;
		return 1;
	}
	if(Args.OutputFilename=="")
	{
		std::cerr << "Output file not specified." << std::endl;
		return 1;
	}

	// Open the input stream
	std::ifstream Input;
	Input.open(Args.InputFilename.c_str(), std::ios_base::in | std::ios_base::binary);
	if(!Input.is_open())
	{
		std::cerr << "Unable to open input file '" << Args.InputFilename << "'." << std::endl;
		return 2;
	}

	// If the size is not specified, compute it
	if(Args.InputSize==0)
	{
		Input.seekg(0, std::ios_base::end);
		if(Args.InputOffset<=Input.tellg())
		{
			Args.InputSize=(std::streamoff)Input.tellg()-Args.InputOffset;
		}
		Input.seekg(0);
	}

	// Open the output stream
	std::ofstream Output;
	Output.open(Args.OutputFilename.c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	if(!Output.is_open())
	{
		std::cerr << "Unable to open output file '" << Args.OutputFilename << "'." << std::endl;
		return 3;
	}

	// Open the segment file stream and parse the segments
	CSegmentParser Segments;
	if(!Args.SegmentFilename.empty())
	{
		std::ifstream SegmentFile;
		SegmentFile.open(Args.SegmentFilename.c_str(), std::ios_base::in);
		if(!SegmentFile.is_open())
		{
			std::cerr << "Unable to open segment definition file '" << Args.SegmentFilename << "'." << std::endl;
			return 104;
		}
		if(!Segments.Parse(SegmentFile))
		{
			SegmentFile.close();
			std::cerr << "Segment file parsing failed." << std::endl;
			std::cerr << Segments.GetParserMessage() << std::endl;
			return 105;
		}
		SegmentFile.close();

		// TODO: Give a little hint about the filename not being right?
	}

	// Check the segments
	if(Args.SegmentFilename.empty())
	{
		CSegment Seg(Args.InputOffset, Args.InputSize);
		Segments.AppendSegment(Seg);
	}

	// Prepare for the wave header, if required
	unsigned long SampleRate=0;
	unsigned char BitsPerSample=0;
	unsigned char Channels=0;
	unsigned long NumberSamples=0;
	if(Args.OuputWaveFile)
	{
		PrepareWaveHeader(Output);
	}

	// See if some of the parameters are forced
	if(Segments.GetChannels()!=0)
	{
		Channels=Segments.GetChannels();
	}
	if(Args.InputChannels!=0)
	{
		Channels=Args.InputChannels;
	}
	if(Segments.GetSampleRate()!=0)
	{
		SampleRate=Segments.GetSampleRate();
	}
	if(Args.OutputSampleRate!=0)
	{
		SampleRate=Args.OutputSampleRate;
	}

	for(unsigned long i=0;i<Segments.GetSegmentCount();i++)
	{
		// Get the current segment
		CSegment Segment(Segments.GetSegment(i));
		unsigned long LocalNumberSamples=0;
		if(Segment.GetSkip())
		{
			continue;
		}

		// Determine the type
		EUbiFormat Type;
		Input.seekg(Segment.GetOffset());

		Type = Args.InputTypeForce;
		if(Type == EUF_NULL)
			Type = DetermineFormat(Input, Segment.GetSize());

		// Set up an input stream
		CFileDataStream FileStream(&Input, Segment.GetOffset(), Segment.GetSize());

		// Decompress the audio
		if(Type==EUF_UBI_V3 || Type==EUF_UBI_V5 || Type==EUF_UBI_V6)
		{
			// Decode the stream
			CVersion5Stream Stream(&FileStream);
			try
			{
				// Initialize
				if(!Stream.InitializeHeader(SampleRate))
				{
					std::cerr << "Problems initializing the header." << std::endl;
					continue;
				}

				// Show information
				if(Args.ShowInfo)
				{
					if(Type==EUF_UBI_V3)
					{
						std::cout << "Type: Old simple chunk (version 3)" << std::endl;
					}
					else if(Type==EUF_UBI_V5)
					{
						std::cout << "Type: Simple chunk (version 5)" << std::endl;
					};
					std::cout << "Offset, Size: " << Segment.GetOffset() << ", " << Segment.GetSize() << std::endl;
					std::cout << "Channels: " << (int)Stream.GetChannels() << std::endl;
					std::cout << std::endl;
				}

				// Decode
				if(!Stream.DecodeToFile(Output, LocalNumberSamples))
				{
					std::cerr << "Problems decompressing the input file." << std::endl;
				}
			}
			catch(XDataException& e)
			{
				std::cerr << "Error: " << e.GetFriendlyMessage() << std::endl;
				std::cerr << e.GetMessage() << std::endl;
			}
			catch(XAudioException& e)
			{
				std::cerr << e.GetFriendlyMessage() << std::endl;
				std::cerr << "Details: " << e.GetMessage() << std::endl;
			}
			catch(...)
			{
				std::cerr << "Unspecified error" << std::endl;
			}

			// Set the information
			SampleRate=Stream.GetSampleRate();
			BitsPerSample=16;
			Channels=Stream.GetChannels();
			NumberSamples+=LocalNumberSamples;
		}
		else if(Type==EUF_UBI_IV8)
		{
			// Decode the stream
			CInterleavedStream Stream(&FileStream);
			try
			{
				// Set some arguments
				if(Args.InputLayers.size()<1)
				{
					std::cerr << "You need to specify which layer or layers (comma separated) to decode." << std::endl;
					continue;
				}
				Stream.SetCurrentLayers(Args.InputLayers);

				/*// Automatic channels detection
				if(Channels==0)
				{
					unsigned long MonoCount=-1;
					unsigned long StereoCount=-1;

					// Try mono
					if(Stream.InitializeHeader(1, Args.InputTypeForce))
					{
						if(Stream.GetType(Stream.GetCurrentLayer())==CInterleavedStream::AT_OGGVORBIS)
						{
							Channels=Stream.GetChannels();
						}
						else
						{
							MonoCount=GetNumberSampleClips(Stream);
						}
					}

					// Try stereo
					if(Stream.InitializeHeader(2, Args.InputTypeForce))
					{
						if(Stream.GetType(Stream.GetCurrentLayer())==CInterleavedStream::AT_OGGVORBIS)
						{
							Channels=Stream.GetChannels();
						}
						else
						{
							StereoCount=GetNumberSampleClips(Stream);
						}
					}

					// Figure out which one
					if(Channels)
					{
						// We're good
					}
					else if(MonoCount<StereoCount)
					{
						Channels=1;
					}
					else if(StereoCount<MonoCount)
					{
						Channels=2;
					}
					else
					{
						std::cerr << "Could not determine the number of channels; assuming stereo" << std::endl;
						Channels=2;
					}
				}*/

				// Initialize
				if(!Stream.InitializeHeader(SampleRate, Args.InputChannels))
				{
					std::cerr << "Problems initializing the header." << std::endl;
					continue;
				}

				// Show info
				if(Args.ShowInfo)
				{
					std::cout << "Type: Interleaved chunk (version 8)" << std::endl;
					std::cout << "Offset: " << Segment.GetOffset() << std::endl;
					std::cout << "Number of Blocks: " << Stream.GetNumberBlocks() << std::endl;
					std::cout << "Channels: " << (int)Stream.GetChannels() << std::endl;
					std::cout << "Number of Layers: " << Stream.GetLayerCount() << std::endl;
					for(unsigned long i=0;i<Stream.GetLayerCount();i++)
					{
						std::cout << "Layer " << i+1 << ": ";
						switch(Stream.GetType(i))
						{
							case EUF_PCM:
								std::cout << "Raw 16-bit uncompressed";
							break;
							case EUF_UBI_V3:
								std::cout << "Simple chunk (version 3)";
							break;
							case EUF_UBI_V5:
								std::cout << "Simple chunk (version 5)";
							break;
							case EUF_UBI_V6:
								std::cout << "Simple chunk (version 6)";
							break;
							case EUF_OGG:
								std::cout << "Ogg Vorbis";
							break;
						}
						std::cout << std::endl;
					}
					std::cout << std::endl;
				}

				// Decode
				if(!Stream.DecodeToFile(Output, LocalNumberSamples))
				{
					std::cerr << "Problems decompressing the input file." << std::endl;
				}
			}
			catch(XDataException& e)
			{
				std::cerr << e.GetFriendlyMessage() << std::endl;
				std::cerr << "Details: " << e.GetMessage() << std::endl;
			}
			catch(XAudioException& e)
			{
				std::cerr << e.GetFriendlyMessage() << std::endl;
				std::cerr << "Details: " << e.GetMessage() << std::endl;
			}
			catch(...)
			{
				std::cerr << "Unspecified error" << std::endl;
			}

			// Set the information
			SampleRate=Stream.GetSampleRate();
			BitsPerSample=16;
			Channels=Stream.GetChannels();
			NumberSamples+=LocalNumberSamples;
		}
		else if(Type==EUF_UBI_IV9)
		{
			// Decode the stream
			CInterleaved9Stream Stream(&FileStream);
			try
			{
				// Set some arguments
				if(Args.InputLayers.size()<1)
				{
					std::cerr << "You need to specify which layer or layers (comma separated) to decode." << std::endl;
					continue;
				}
				Stream.SetCurrentLayers(Args.InputLayers);

				// Initialize
				if(!Stream.InitializeHeader(SampleRate, Args.InputChannels))
				{
					std::cerr << "Problems initializing the header." << std::endl;
					continue;
				}

				// Show info
				if(Args.ShowInfo)
				{
					std::cout << "Type: Interleaved chunk (version 9)" << std::endl;
					std::cout << "Offset: " << Segment.GetOffset() << std::endl;
					std::cout << "Number of Blocks: " << Stream.GetNumberBlocks() << std::endl;
					std::cout << "Channels: " << (int)Stream.GetChannels() << std::endl;
					std::cout << "Number of Layers: " << Stream.GetLayerCount() << std::endl;
					for(unsigned long i=0;i<Stream.GetLayerCount();i++)
					{
						std::cout << "Layer " << i+1 << ": ";
						switch(Stream.GetType(i))
						{
							case EUF_PCM:
								std::cout << "Raw 16-bit uncompressed";
							break;
							case EUF_UBI_V3:
								std::cout << "Simple chunk (version 3)";
							break;
							case EUF_UBI_V5:
								std::cout << "Simple chunk (version 5)";
							break;
							case EUF_UBI_V6:
								std::cout << "Simple chunk (version 6)";
							break;
							case EUF_OGG:
								std::cout << "Ogg Vorbis";
							break;
						}
						std::cout << std::endl;
					}
					std::cout << std::endl;
				}

				// Decode
				if(!Stream.DecodeToFile(Output, LocalNumberSamples))
				{
					std::cerr << "Problems decompressing the input file." << std::endl;
				}
			}
			catch(XDataException& e)
			{
				std::cerr << e.GetFriendlyMessage() << std::endl;
				std::cerr << "Details: " << e.GetMessage() << std::endl;
			}
			catch(XAudioException& e)
			{
				std::cerr << e.GetFriendlyMessage() << std::endl;
				std::cerr << "Details: " << e.GetMessage() << std::endl;
			}
			catch(...)
			{
				std::cerr << "Unspecified error" << std::endl;
			}

			// Set the information
			SampleRate=Stream.GetSampleRate();
			BitsPerSample=16;
			Channels=Stream.GetChannels();
			NumberSamples+=LocalNumberSamples;
		}
		else if(Type==EUF_UBI_IV2)
		{
			// Decode the stream
			COldInterleavedStream Stream(&FileStream);
			try
			{
				// Set some arguments
				if(Args.InputLayers.size()<1)
				{
					std::cerr << "You need to specify which layer or layers (comma separated) to decode." << std::endl;
					continue;
				}
				Stream.SetCurrentLayers(Args.InputLayers);

				// Initialize
				if(!Stream.InitializeHeader(SampleRate))
				{
					std::cerr << "Problems initializing the header." << std::endl;
					continue;
				}

				// Show info
				if(Args.ShowInfo)
				{
					std::cout << "Type: Old interleaved chunk (version 2)" << std::endl;
					std::cout << "Offset: " << Segment.GetOffset() << std::endl;
					std::cout << "Total Size: " << Stream.GetTotalBytes() << std::endl;
					std::cout << "Channels: " << (int)Stream.GetChannels() << std::endl;
					std::cout << "Number of Layers: " << Stream.GetLayerCount() << std::endl;
					for(unsigned long i=0;i<Stream.GetLayerCount();i++)
					{
						std::cout << "Layer " << i+1 << ": ";
						switch(Stream.GetType(i))
						{
							case EUF_UBI_V3:
								std::cout << "Simple chunk (version 3)";
							break;
							case EUF_UBI_V5:
								std::cout << "Simple chunk (version 5)";
							break;
							case EUF_UBI_V6:
								std::cout << "Simple chunk (version 6)";
							break;
							case EUF_UBI_6OR4:
								std::cout << "Old 6-or-4 bit chunk";
							break;
						}
						std::cout << std::endl;
					}
					std::cout << std::endl;
				}

				// Decode
				if(!Stream.DecodeToFile(Output, LocalNumberSamples))
				{
					std::cerr << "Problems decompressing the input file." << std::endl;
				}
			}
			catch(XDataException& e)
			{
				std::cerr << "Error: " << e.GetFriendlyMessage() << std::endl;
				std::cerr << e.GetMessage() << std::endl;
			}
			catch(XAudioException& e)
			{
				std::cerr << e.GetFriendlyMessage() << std::endl;
				std::cerr << "Details: " << e.GetMessage() << std::endl;
			}
			catch(...)
			{
				std::cerr << "Unspecified error" << std::endl;
			}

			// Set the information
			SampleRate=Stream.GetSampleRate();
			BitsPerSample=16;
			Channels=Stream.GetChannels();
			NumberSamples+=LocalNumberSamples;
		}
		else if(Type==EUF_UBI_6OR4)
		{
			// Decode the stream
			COld6Or4BitStream Stream(&FileStream);
			try
			{
				// Initialize
				if(!Stream.InitializeHeader())
				{
					std::cerr << "Problems initializing the header." << std::endl;
					continue;
				}

				// Show info
				if(Args.ShowInfo)
				{
					std::cout << "Type: Old 6-Or-4 Bit" << std::endl;
					std::cout << "Offset: " << Segment.GetOffset() << std::endl;
					std::cout << "Channels: " << (int)Stream.GetChannels() << std::endl;
					std::cout << "Sample Rate: " << Stream.GetSampleRate() << std::endl;
					std::cout << "Bits Per Sample: " << (int)Stream.GetBitsPerSample() << std::endl;
					std::cout << std::endl;
				}

				// Decode
				if(!Stream.DecodeToFile(Output, LocalNumberSamples))
				{
					std::cerr << "Problems decompressing the input file." << std::endl;
				}

				// Set the information
				SampleRate=Stream.GetSampleRate();
				Channels=Stream.GetChannels();
			}
			catch(XDataException& e)
			{
				std::cerr << "Error: " << e.GetFriendlyMessage() << std::endl;
				std::cerr << e.GetMessage() << std::endl;
			}
			catch(XAudioException& e)
			{
				std::cerr << e.GetFriendlyMessage() << std::endl;
				std::cerr << "Details: " << e.GetMessage() << std::endl;
			}

			// Set the information
			BitsPerSample=16;
			NumberSamples+=LocalNumberSamples;
		}
		else if(Type==EUF_OGG)
		{
			// Decode the stream
			COggVorbisStream Stream(&FileStream);
			try
			{
				// Initialize
				if(!Stream.InitializeHeader())
				{
					std::cerr << "Problems initializing the header." << std::endl;
					continue;
				}

				// Show info
				if(Args.ShowInfo)
				{
					std::cout << "Type: Ogg Vorbis" << std::endl;
					std::cout << "Offset: " << Segment.GetOffset() << std::endl;
					std::cout << "Channels: " << (int)Stream.GetChannels() << std::endl;
					std::cout << "Sample Rate: " << Stream.GetSampleRate() << std::endl;
					std::cout << std::endl;
				}

				// Decode
				if(!Stream.DecodeToFile(Output, LocalNumberSamples))
				{
					std::cerr << "Problems decompressing the input file." << std::endl;
				}

				// Set the information
				SampleRate=Stream.GetSampleRate();
				Channels=Stream.GetChannels();
			}
			catch(XDataException& e)
			{
				std::cerr << "Error: " << e.GetFriendlyMessage() << std::endl;
				std::cerr << e.GetMessage() << std::endl;
			}
			catch(XAudioException& e)
			{
				std::cerr << e.GetFriendlyMessage() << std::endl;
				std::cerr << "Details: " << e.GetMessage() << std::endl;
			}

			// Set the information
			BitsPerSample=16;
			NumberSamples+=LocalNumberSamples;
		}
		else if(Type==EUF_UBI_RAW)
		{
			// Decode the stream
			CRawCompressedStream Stream(&FileStream);
			try
			{
				// Automatic channels detection
				if(Channels==0)
				{
					unsigned long MonoCount=-1;
					unsigned long StereoCount=-1;

					// Try mono
					if(Stream.InitializeHeader(1, Args.CompLeftSample, Args.CompLeftIndex, Args.CompRightSample, Args.CompRightIndex))
					{
						MonoCount=GetNumberSampleClips(Stream);
					}

					// Try stereo
					if(Stream.InitializeHeader(2, Args.CompLeftSample, Args.CompLeftIndex, Args.CompRightSample, Args.CompRightIndex))
					{
						StereoCount=GetNumberSampleClips(Stream);
					}

					// Figure out which one
					if(MonoCount<StereoCount)
					{
						Channels=1;
					}
					else if(StereoCount<MonoCount)
					{
						Channels=2;
					}
					else
					{
						std::cerr << "Could not determine the number of channels; assuming stereo" << std::endl;
						Channels=2;
					}
				}

				// Initialize
				if(!Stream.InitializeHeader(Channels, Args.CompLeftSample, Args.CompLeftIndex, Args.CompRightSample, Args.CompRightIndex))
				{
					std::cerr << "Problems initializing the header." << std::endl;
					continue;
				}

				// Show information
				if(Args.ShowInfo)
				{
					std::cout << "Type: Raw UbiSoft ADPCM" << std::endl;
					std::cout << "Offset, Size: " << Segment.GetOffset() << ", " << Segment.GetSize() << std::endl;
					std::cout << "Channels: " << (int)Stream.GetChannels() << std::endl;
					std::cout << std::endl;
				}

				// Decode
				if(!Stream.DecodeToFile(Output, LocalNumberSamples))
				{
					std::cerr << "Problems decompressing the input file." << std::endl;
				}
			}
			catch(XDataException& e)
			{
				std::cerr << "Error: " << e.GetFriendlyMessage() << std::endl;
				std::cerr << e.GetMessage() << std::endl;
			}
			catch(XAudioException& e)
			{
				std::cerr << e.GetFriendlyMessage() << std::endl;
				std::cerr << "Details: " << e.GetMessage() << std::endl;
			}
			catch(...)
			{
				std::cerr << "Unspecified error" << std::endl;
			}

			// Set the information
			SampleRate=Stream.GetSampleRate();
			BitsPerSample=16;
			Channels=Stream.GetChannels();
			NumberSamples+=LocalNumberSamples;
		}
		else if(Type==EUF_RAW)
		{
			// Decode the stream
			CRawPcmStream Stream(&FileStream);
			try
			{
				// No automatic detection is possible at this time
				if(Channels==0)
				{
					std::cerr << "Could not determine the number of channels; assuming stereo" << std::endl;
					Channels=2;
				}

				// Initialize
				if(!Stream.InitializeHeader(Channels))
				{
					std::cerr << "Problems initializing the header." << std::endl;
					continue;
				}

				// Show information
				if(Args.ShowInfo)
				{
					std::cout << "Type: Raw 16-bit PCM" << std::endl;
					std::cout << "Offset, Size: " << Segment.GetOffset() << ", " << Segment.GetSize() << std::endl;
					std::cout << std::endl;
				}

				// Decode
				if(!Stream.DecodeToFile(Output, LocalNumberSamples))
				{
					std::cerr << "Problems decompressing the input file." << std::endl;
				}
			}
			catch(XDataException& e)
			{
				std::cerr << "Error: " << e.GetFriendlyMessage() << std::endl;
				std::cerr << e.GetMessage() << std::endl;
			}
			catch(XAudioException& e)
			{
				std::cerr << e.GetFriendlyMessage() << std::endl;
				std::cerr << "Details: " << e.GetMessage() << std::endl;
			}
			catch(...)
			{
				std::cerr << "Unspecified error" << std::endl;
			}

			// Set the information
			SampleRate=Stream.GetSampleRate();
			BitsPerSample=16;
			Channels=Stream.GetChannels();
			NumberSamples+=LocalNumberSamples;
		}
		else
		{
			std::cerr << "The input file is not a supported format. ";
			std::cerr << "Use --input-type to force a specific format." << std::endl;
			continue;
		}
	}

	// Fix the wave header
	// TODO: Move this up, perhaps?
	if(Segments.GetSampleRate()!=0)
	{
		SampleRate=Segments.GetSampleRate();
	}
	if(Args.OutputSampleRate!=0)
	{
		SampleRate=Args.OutputSampleRate;
	}
	if(Args.OuputWaveFile)
	{
		Output.seekp(0);
		WriteWaveHeader(Output, SampleRate, BitsPerSample, Channels, NumberSamples);
	}

	// Finish up
	Input.close();
	Output.close();
	return 0;
}

class PrintingScanCallback : public ScanCallback
{
public:

	virtual bool foundSegment(std::streamoff offset, std::streamsize size)
	{
		std::cout << offset << "\t" << size << std::endl;
		count++;
		return true;
	}

	// Keep track of how many were found
	unsigned long count;
};

int Scan(SArguments& Args)
{
	// Check the arguments
	if(Args.InputFilename=="")
	{
		std::cerr << "Input file not specified." << std::endl;
		return 1;
	}

	// Open the input stream
	std::ifstream Input;
	Input.open(Args.InputFilename.c_str(), std::ios_base::in | std::ios_base::binary);
	if(!Input.is_open())
	{
		std::cerr << "Unable to open input file '" << Args.InputFilename << "'." << std::endl;
		return 2;
	}

	// If the size is not specified, compute it
	if(Args.InputSize==0)
	{
		Input.seekg(0, std::ios_base::end);
		if(Args.InputOffset<=Input.tellg())
		{
			Args.InputSize=Input.tellg()-Args.InputOffset;
		}
		Input.seekg(0);
	}

	PrintingScanCallback callback;
	callback.count = 0;

	// Scan the file
	Input.seekg((std::streamoff)Args.InputOffset);
	ScanAndList(Input, Args.InputOffset+Args.InputSize, callback);

	std::cerr << "Found: " << callback.count << std::endl;

	// Finish up
	Input.close();
	return 0;
}

int LayerExtract(SArguments& Args)
{
	// Check the arguments
	if(Args.InputFilename=="")
	{
		std::cerr << "Input file not specified." << std::endl;
		return 1;
	}
	if(Args.OutputFilename=="")
	{
		std::cerr << "Output file not specified." << std::endl;
		return 1;
	}

	// Open the input stream
	std::ifstream Input;
	Input.open(Args.InputFilename.c_str(), std::ios_base::in | std::ios_base::binary);
	if(!Input.is_open())
	{
		std::cerr << "Unable to open input file '" << Args.InputFilename << "'." << std::endl;
		return 2;
	}

	// If the size is not specified, compute it
	if(Args.InputSize==0)
	{
		Input.seekg(0, std::ios_base::end);
		if(Args.InputOffset<=Input.tellg())
		{
			Args.InputSize=(std::streamoff)Input.tellg()-Args.InputOffset;
		}
		Input.seekg(0);
	}

	// Open the output stream
	std::ofstream Output;
	Output.open(Args.OutputFilename.c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	if(!Output.is_open())
	{
		std::cerr << "Unable to open output file '" << Args.OutputFilename << "'." << std::endl;
		return 3;
	}

	// Determine the type
	EUbiFormat Type;
	Input.seekg(Args.InputOffset);
	Type = Args.InputTypeForce;

	if(Type == EUF_NULL)
		Type = DetermineFormat(Input, Args.InputSize);
		
	if(Type == EUF_NULL)
	{
		std::cerr << "The input file is not a supported format. ";
		std::cerr << "Use --input-type to force a specific format." << std::endl;
		Input.close();
		Output.close();
		return 100;
	}

	// Set up an input stream
	CFileDataStream FileStream(&Input, Args.InputOffset, Args.InputSize);

	// Get the layer
	unsigned long Layer;
	if(Args.InputLayers.size()!=1)
	{
		// Do a generic copy
		while(!FileStream.IsEnd())
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
		Layer=Args.InputLayers[0];

		// Do the layer extract
		try
		{
			if(Type==EUF_UBI_IV8)
			{
				if(!CInterleavedStream::LayerExtract(&FileStream, Layer, Output))
				{
					std::cerr << "Error extracting layers" << std::endl;
				}
			}
			else if(Type==EUF_UBI_IV9)
			{
				if(!CInterleaved9Stream::LayerExtract(&FileStream, Layer, Output))
				{
					std::cerr << "Error extracting layers" << std::endl;
				}
			}
			else if(Type==EUF_UBI_IV2)
			{
				if(!COldInterleavedStream::LayerExtract(&FileStream, Layer, Output))
				{
					std::cerr << "Error extracting layers" << std::endl;
				}
			}
			else
			{
				std::cerr << "This format cannot be layer extracted" << std::endl;
			}
		}
		catch(XDataException& e)
		{
			std::cerr << "Error: " << e.GetFriendlyMessage() << std::endl;
			std::cerr << e.GetMessage() << std::endl;
		}
		catch(XAudioException& e)
		{
			std::cerr << e.GetFriendlyMessage() << std::endl;
			std::cerr << "Details: " << e.GetMessage() << std::endl;
		}
		catch(...)
		{
			std::cerr << "Unspecified error" << std::endl;
		}
	}

	// Finish up
	Input.close();
	Output.close();
	return 0;
}

int _tmain(int Argc, _TCHAR* Argv[])
{
	// Parse the arguments
	SArguments Args;
	if(Argc==1)
	{
		Args.ShowUsage=true;
	}
	bool ArgParse=ParseArguments(Args, Argc, Argv);

	// Display banner
	if(Args.ShowBanner)
	{
		std::cerr << "Sound/Music Decoder for UBISOFT Formats version " << DECUBISND_VERSION << std::endl << std::endl;
	}

	// Display usage
	if(Args.ShowUsage)
	{
		std::cout << "Usage: DecUbiSnd InputFilename [Options]" << std::endl;
		std::cout << std::endl;
		std::cout << "Decoding: (-D, --decode) (default)" << std::endl;
		std::cout << "  -o, --output File     Specify the output filename" << std::endl;
		std::cout << "  -w, --wave            Output a standard wave file (.wav)" << std::endl;
		std::cout << "  -r, --raw             Output raw data (no header)" << std::endl;
		std::cout << "  -i, --offset Number   Specify the offset to begin decoding" << std::endl;
		std::cout << "  -s, --size Number     Specify the number of bytes to decode" << std::endl;
		std::cout << "  -t, --stereo          The sound is stereo" << std::endl;
		std::cout << "  -m, --mono            The sound is mono" << std::endl;
		std::cout << "  -a, --auto            Automatically detect the number of channels" << std::endl;
		std::cout << "  -l, --layer Number    Specify the layer number(s) (comma separated)" << std::endl;
		std::cout << "  -g, --segments File   Use a segment definition file" << std::endl;
		std::cout << "  -n, --info            Output information about the file" << std::endl;
		std::cout << "  --input-type Type     Force it to use the decoder for Type (see below)" << std::endl;
		std::cout << "  --sample-rate Rate    Force a specific sampling rate" << std::endl;
		std::cout << std::endl;
		std::cout << "Scanning: (-S, --scan)" << std::endl;
		std::cout << "  -i, --offset Number   Specify the offset to begin scanning" << std::endl;
		std::cout << "  -s, --size Number     Specify the number of bytes to scan" << std::endl;
		std::cout << std::endl;
		std::cout << "Layer Extract: (-L, --layer-extract)" << std::endl;
		std::cout << "  -o, --output File     Specify the output filename" << std::endl;
		std::cout << "  -i, --offset Number   Specify the offset of the stream" << std::endl;
		std::cout << "  -s, --size Number     Specify the number of bytes in the stream" << std::endl;
		std::cout << "  -l, --layer Number    Specify the layer number" << std::endl;
		std::cout << "  --input-type Type     Force it to use the decoder for Type (see below)" << std::endl;
		std::cout << std::endl;
		std::cout << "Raw UbiSoft ADPCM Decode: (--input-type ubi_raw)" << std::endl;
		std::cout << "  --comp-left Smp Idx   Specify left decompression parameters" << std::endl;
		std::cout << "  --comp-right Smp Idx  Specify right decompression parameters" << std::endl;
		std::cout << std::endl;
		std::cout << "Possible Input Types: " << std::endl;
		std::cout << "  ubi_v3     Old simple chunk (version 3)" << std::endl;
		std::cout << "  ubi_v5     Simple chunk (version 5)" << std::endl;
		std::cout << "  ubi_v6     Simple chunk (version 6)" << std::endl;
		std::cout << "  ubi_iv2    Old interleaved chunk (version 2)" << std::endl;
		std::cout << "  ubi_iv8    Interleaved chunk (version 8)" << std::endl;
		std::cout << "  ubi_iv9    Interleaved chunk (version 9)" << std::endl;
		std::cout << "  ubi_6or4   Old UbiSoft 6-Or-4 bit chunk (XIII, SC1 PC)" << std::endl;
		std::cout << "  ubi_raw    Raw compressed UbiSoft ADPCM" << std::endl;
		std::cout << "  raw        Raw 16-bit uncompressed PCM" << std::endl;
		std::cout << "  ogg        Ogg Vorbis" << std::endl;
		std::cout << std::endl;
	}

	// Check for errors
	if(!ArgParse)
	{
		std::cerr << "The arguments are not valid." << std::endl;
		return 1;
	}

	// Process it based on the action required
	int ReturnValue=1;
	switch(Args.Action)
	{
		case EACT_DECODE:
			ReturnValue=Decode(Args);
		break;
		case EACT_SCAN:
			ReturnValue=Scan(Args);
		break;
		case EACT_LAYEREXTRACT:
			ReturnValue=LayerExtract(Args);
		break;
	}
	return ReturnValue;
}
