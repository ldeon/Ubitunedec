// InterleavedStream.h : UbiSoft version 8 interleaved audio stream decoding
//

#include "Pch.h"
#include "Interleaved9Stream.h"
#include "Version5Stream.h"
#include "OggVorbisStream.h"
#include "BufferDataStream.h"
#include "DataExceptions.h"
#include "AudioExceptions.h"
#include "UbiFormats.h"

// Information associated with each layer
struct CInterleaved9Stream::SInterleavedLayer
{
	SInterleavedLayer() : Stream(NULL), Data(NULL) {}
	~SInterleavedLayer() { delete Stream; delete Data; }

	EUbiFormat Type;
	CAudioStream* Stream;
	CBufferDataStream* Data;
	bool First;
	unsigned long BlockSize;
};

CInterleaved9Stream::CInterleaved9Stream(CDataStream* Input) :
	CLayeredStreamHelper(Input),
	m_Variant(EV_A),
	m_BlockNumber(0),
	m_TotalBlocks(0),
	m_SampleRate(48000),
	m_Channels(2)
{
	DoRegisterParams();
	return;
}

CInterleaved9Stream::~CInterleaved9Stream()
{
	Clear();
	return;
}

bool CInterleaved9Stream::InitializeHeader()
{
	return InitializeHeader(0);
}

bool CInterleaved9Stream::InitializeHeader(unsigned long SampleRate)
{
	return InitializeHeader(SampleRate, 2);
}

bool CInterleaved9Stream::InitializeHeader(unsigned long SampleRate, unsigned char PcmChannels)
{
	// Check params
	if(PcmChannels<1)
	{
		PcmChannels=2;
	}

	// Clear previous data
	Clear();

	// Read the type from the file
	unsigned short Type=0;
	unsigned short SubType=0;
	if(m_InputStream->CanSeekBackward())
	{
		m_InputStream->SeekToBeginning();
	}
	m_InputStream->ExactRead(&Type, 2);
	m_InputStream->ExactRead(&SubType, 2);
	if(Type==9)
	{
		m_InputStream->SetEndian(CDataStream::LittleEndian);
	}
	else if(SubType==0x0900)
        {
                m_InputStream->SetEndian(CDataStream::BigEndian);
        }
	else
	{
		throw(XFileException("File does not have the correct signature (should be 09)"));
	}

	// Read a bit of the first header
	unsigned long NumberLayers;
	unsigned long TotalInfoSize;
	m_InputStream->ExactIgnore(4);
	NumberLayers=m_InputStream->ExactReadULong();
	m_TotalBlocks=m_InputStream->ExactReadULong();
	m_BlockNumber=0;
	TotalInfoSize=m_InputStream->ExactReadULong();

	// Check if this file is a different variant
	if(Type==9 || SubType==0x0900)
	{
		m_Variant=EV_A;

		m_InputStream->ExactIgnore(TotalInfoSize);
		m_InputStream->ExactIgnore(64 - NumberLayers*4);
	}

	// Process the second header
	std::vector<unsigned long> HeaderSizes;
	for(unsigned long i=0;i<NumberLayers;i++)
	{
		// Read the audio header size
		unsigned long HeaderSize;
		HeaderSize=m_InputStream->ExactReadULong();
		HeaderSizes.push_back(HeaderSize);
		if(HeaderSize>1000000)
		{
			throw(XFileException("Header size too big"));
		}
	}

	// Read the headers and create the layers
	for(unsigned long i=0;i<NumberLayers;i++)
	{
		// Create the layer
		SInterleavedLayer* NewLayer=new SInterleavedLayer;
		m_Layers.push_back(NewLayer);
		SInterleavedLayer& Layer=*m_Layers[i];

		// Read the header and send it
		unsigned char* Buffer;
		Layer.Data=new CBufferDataStream();
		Buffer=(unsigned char*)m_InputStream->ExactRead(HeaderSizes[i]);
		if(HeaderSizes[i]>0)
		{
			Layer.Data->SendBuffer(Buffer, HeaderSizes[i]);
		}

		// Detect the type
		if(HeaderSizes[i]==0)
		{
			// Must be PCM, there is no header
			Layer.Type=EUF_PCM;
			Layer.Stream=NULL;
			if(SampleRate==0)
			{
				m_SampleRate=44100;
			}
			else
			{
				m_SampleRate=SampleRate;
			}
			m_Channels=PcmChannels;
		}
		else if((Buffer[0]==5 || Buffer[0]==3 || Buffer[0]==6) && HeaderSizes[i] >= 28)
		{
			// Check the header size
			if(HeaderSizes[i] != 28 && HeaderSizes[i] != 36)
			{
				std::cerr << "Warning: Header size is unrecognized (ADPCM, " << HeaderSizes[i] << " bytes, should be 28 or 36)" << std::endl;
			}

			// It is likely a simple block
			CVersion5Stream* Stream=new CVersion5Stream(Layer.Data);
			switch (Buffer[0])
			{
			case 3:
				Layer.Type = EUF_UBI_V3;
				break;
			case 5:
				Layer.Type = EUF_UBI_V5;
				break;
			case 6:
				Layer.Type = EUF_UBI_V6;
				break;
			default:
				Layer.Type = EUF_UBI_V5;
				break;
			}
			Layer.Stream=Stream;

			// Initialize the header
			try
			{
				// Initialize the header
				if(!Stream->InitializeHeader(SampleRate))
				{
					Clear();
					return false;
				}

				// Get some information
				m_SampleRate=Stream->GetSampleRate();
				m_Channels=Stream->GetChannels();
			}
			catch(XNeedBuffer&)
			{
				Clear();
				throw(XFileException("The decoder needed more information than the header provided"));
			}
		}
		else if(memcmp(Buffer, "OggS", 4)==0)
		{
			// It is Ogg Vorbis
			COggVorbisStream* Stream=new COggVorbisStream(Layer.Data);
			Layer.Type=EUF_OGG;
			Layer.Stream=Stream;
		}
	}

	// Set the callback functions now that we're able to start reading blocks
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Get the layer
		SInterleavedLayer& Layer=*m_Layers[i];

		// Set it
		Layer.Data->SetNeedBufferCallback(BufferCallback, this);
	}

	// Initialize the headers
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Get the layer
		SInterleavedLayer& Layer=*m_Layers[i];

		// Initialize the headers
		if(Layer.Type==EUF_PCM)
		{
			// Raw PCM has no header
		}
		else if(Layer.Type==EUF_UBI_V3 || Layer.Type==EUF_UBI_V5 || Layer.Type==EUF_UBI_V6)
		{
			// Already initialized
		}
		else if(Layer.Type==EUF_OGG)
		{
			// Grab as much data as we need
			while(true)
			{
				try
				{
					// Initialize the header
					if(!Layer.Stream->InitializeHeader())
					{
						Clear();
						return false;
					}

					m_SampleRate=Layer.Stream->GetSampleRate();
					m_Channels=Layer.Stream->GetChannels();
				}
				catch(XNeedBuffer& e)
				{
					while(Layer.Data->GetBufferedLength()<e.GetLength())
					{
						// Get some data
						if(!DoReadBlock())
						{
							break;
						}
					}
					continue;
				}
				break;
			}
		}
	}

	// We're initialized
	m_Initialized=true;
	return true;
}

bool CInterleaved9Stream::DoDecodeLayer(unsigned long LayerIndex)
{
	// Make sure it's valid
	if(!m_Initialized)
	{
		throw(XProgramException("The stream has not been initialized"));
	}

	// Go through each of the layers
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Get the layer
		SInterleavedLayer& Layer=*m_Layers[i];

		// If this is not the layer just continue
		if(i!=LayerIndex)
		{
			if(!IsLayerDecoded(i))
			{
				Layer.Data->ResetBuffer();
			}
			continue;
		}

		// Prepare the output
		unsigned long RequestAmount=RecommendBufferLength();
		PrepareOutputBuffer(RequestAmount);

		// Grab as much data as we need
		while(true)
		{
			try
			{
				if(Layer.Stream)
				{
					if(!Layer.Stream->Decode(m_OutputBuffer, RequestAmount))
					{
						return false;
					}
					//std::cout << RequestAmount << std::endl;
					m_OutputBufferUsed=RequestAmount;
				}
				else
				{
					m_OutputBufferUsed=Layer.Data->Read(m_OutputBuffer, RequestAmount)/2;
				}
			}
			catch(XNeedBuffer& e)
			{
				while(Layer.Data->GetBufferedLength()<e.GetLength())
				{
					// Get some data
					if(!DoReadBlock())
					{
						break;
					}
				}
				continue;
			}
			break;
		}
	}
	return true;
}

bool CInterleaved9Stream::DoReadBlock()
{
	// Check for the end of the file
	if(m_InputStream->IsEnd() || m_BlockNumber==m_TotalBlocks)
	{
		// Mark the end of the stream for all of the layers
		for(unsigned long i=0;i<m_Layers.size();i++)
		{
			SInterleavedLayer& Layer=*m_Layers[i];
			Layer.Data->EndStream();
		}
		return false;
	}

	// Process the first block header
	if(m_Variant==EV_A)
	{
		unsigned long BlockID;
		BlockID=m_InputStream->ExactReadULong();
		if(BlockID!=3)
		{
			// Mark the end of the stream for all of the layers
			for(unsigned long i=0;i<m_Layers.size();i++)
			{
				SInterleavedLayer& Layer=*m_Layers[i];
				Layer.Data->EndStream();
			}
			return false;
			//throw(XFileException("Error: Invalid block ID"));
		}
		m_InputStream->ExactReadULong();
	}

	// Read in the block sizes
	std::vector<unsigned long> BlockSizes;
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		unsigned long BlockSize;
		BlockSize=m_InputStream->ExactReadULong();
		BlockSizes.push_back(BlockSize);
	}

	// Go through each of the layers
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Get a reference to the layer
		SInterleavedLayer& Layer=*m_Layers[i];
		Layer.First=false;

		// Feed it a block
		void* Buffer;
		Buffer=m_InputStream->ExactRead(BlockSizes[i]);
		Layer.Data->SendBuffer(Buffer, BlockSizes[i]);
	}
	m_BlockNumber++;
	return true;
}

bool CInterleaved9Stream::LayerExtract(CDataStream* Input, unsigned long Layer, std::ostream& Output)
{
	// Read the type from the file
	unsigned short Type;
	unsigned short SubType;
	if(Input->CanSeekBackward())
	{
		Input->SeekToBeginning();
	}
	Input->ExactRead(&Type, 2);
	Input->ExactRead(&SubType, 2);
	if(Type==9)
	{
		Input->SetEndian(CDataStream::LittleEndian);
	}
	else if(SubType==0x0900)
        {
                Input->SetEndian(CDataStream::BigEndian);
        }
	else
	{
		throw(XFileException("File does not have the correct signature (should be 08)"));
	}

	// Read a bit of the first header
	unsigned long NumberLayers;
	unsigned long TotalInfoSize;
	unsigned long TotalBlocks;
	unsigned long BlockNumber;
	CInterleaved9Stream::EVariant Variant;
	Input->ExactIgnore(4);
	NumberLayers=Input->ExactReadULong();
	TotalBlocks=Input->ExactReadULong();
	BlockNumber=0;
	TotalInfoSize=Input->ExactReadULong();

	// Check if this file is a different variant
	if(Type==9 || SubType==0x0900)
	{
		Variant=EV_A;

		Input->ExactIgnore(TotalInfoSize);
		Input->ExactIgnore(64 - NumberLayers*4);
	}

	// Process the second header
	std::vector<unsigned long> HeaderSizes;
	for(unsigned long i=0;i<NumberLayers;i++)
	{
		// Read the audio header size
		unsigned long HeaderSize;
		HeaderSize=Input->ExactReadULong();
		HeaderSizes.push_back(HeaderSize);
	}

	// Read the headers and create the layers
	for(unsigned long i=0;i<NumberLayers;i++)
	{
		// Read the header and write it
		unsigned char* Buffer;
		Buffer=(unsigned char*)Input->ExactRead(HeaderSizes[i]);
		if(HeaderSizes[i]>0)
		{
			if(i==Layer)
			{
				Output.write((char*)Buffer, HeaderSizes[i]);
			}
		}
	}

	// Loop to get all the blocks
	while(true)
	{
		// Check for the end of the file
		if(Input->IsEnd() || BlockNumber==TotalBlocks)
		{
			break;
		}

		// Process the first block header
		if(Variant==EV_A)
		{
			unsigned long BlockID;
			BlockID=Input->ExactReadULong();
			if(BlockID!=3)
			{
				throw(XFileException("Error: Invalid block ID"));
			}
			Input->ExactReadULong();
		}

		// Read in the block sizes
		std::vector<unsigned long> BlockSizes;
		for(unsigned long i=0;i<NumberLayers;i++)
		{
			unsigned long BlockSize;
			BlockSize=Input->ExactReadULong();
			BlockSizes.push_back(BlockSize);
		}

		// Go through each of the layers
		for(unsigned long i=0;i<NumberLayers;i++)
		{
			// Read
			void* Buffer;
			Buffer=Input->ExactRead(BlockSizes[i]);
			if(i==Layer)
			{
				Output.write((char*)Buffer, BlockSizes[i]);
			}
		}
		BlockNumber++;
	}
	return true;
}

unsigned long CInterleaved9Stream::GetSampleRate() const
{
	return m_SampleRate;
}

unsigned char CInterleaved9Stream::GetChannels() const
{
	return m_Channels;
}

std::string CInterleaved9Stream::GetFormatName() const
{
	return "ubi_iv9";
}

EUbiFormat CInterleaved9Stream::GetType(unsigned long Layer) const
{
	// Check the state
	if(Layer<0 || Layer>=m_Layers.size())
	{
		throw(XUserException("The layer number is not valid"));
	}

	// Get a reference to the layer
	const SInterleavedLayer& LayerRef=*m_Layers[Layer];
	return LayerRef.Type;
}

unsigned long CInterleaved9Stream::GetNumberBlocks() const
{
	return m_TotalBlocks;
}

unsigned long CInterleaved9Stream::GetLayerCount() const
{
	return (unsigned long)m_Layers.size();
}

void CInterleaved9Stream::DoRegisterParams()
{
	//RegisterParam("Layer", (TSetLongParamProc)SetLayer, NULL, (TGetLongParamProc)GetLayer, NULL);
	return;
}

void CInterleaved9Stream::Clear()
{
	for(std::vector<SInterleavedLayer*>::iterator Iter=m_Layers.begin();Iter!=m_Layers.end();++Iter)
	{
		delete *Iter;
	}
	m_Layers.clear();
	m_OutputBufferOffset=0;
	m_OutputBufferUsed=0;
	m_Initialized=false;
	return;
}

void CInterleaved9Stream::BufferCallback(CBufferDataStream& Stream, unsigned long Bytes, void* UserData)
{
	// Check the user data
	if(!UserData)
	{
		return;
	}

	// Get the audio stream class
	CInterleaved9Stream& Audio=*(CInterleaved9Stream*)UserData;

	// Read in as much data as needed
	while(Stream.GetBufferedLength()<Bytes)
	{
		// Get some data
		if(!Audio.DoReadBlock())
		{
			break;
		}
	}
	return;
}
