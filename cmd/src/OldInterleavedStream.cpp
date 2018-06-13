// OldInterleavedStream.cpp : UbiSoft version 2 interleaved audio stream decoding
//

#include "Pch.h"
#include "OldInterleavedStream.h"
#include "Version5Stream.h"
#include "Old6Or4BitStream.h"
#include "BufferDataStream.h"
#include "DataExceptions.h"
#include "AudioExceptions.h"
#include "UbiFormats.h"

// Information associated with each layer
struct COldInterleavedStream::SOldInterleavedLayer
{
	SOldInterleavedLayer() : Stream(NULL), Data(NULL) {};
	~SOldInterleavedLayer() { delete Stream; delete Data; }

	EUbiFormat Type;
	CAudioStream* Stream;
	CBufferDataStream* Data;
	bool First;
};

COldInterleavedStream::COldInterleavedStream(CDataStream* Input) :
	CLayeredStreamHelper(Input),
	m_BlockNumber(0),
	m_TotalBytes(0),
	m_SampleRate(36000),
	m_Stereo(true)
{
	DoRegisterParams();
	return;
}

COldInterleavedStream::~COldInterleavedStream()
{
	Clear();
	return;
}

bool COldInterleavedStream::InitializeHeader()
{
	return InitializeHeader(0);
}

bool COldInterleavedStream::InitializeHeader(unsigned long SampleRate)
{
	// Clear previous data
	Clear();

	// Read the type from the file
	unsigned short Type;
	if(m_InputStream->CanSeekBackward())
	{
		m_InputStream->SeekToBeginning();
	}
	m_InputStream->ExactRead(&Type, 2);
	if(Type!=2)
	{
		throw(XFileException("File does not have the correct signature (should be 02)"));
	}

	// Read the header
	unsigned long NumberLayers;
	unsigned long TotalSize;
	m_InputStream->ExactIgnore(2);
	m_InputStream->ExactRead(&NumberLayers, 4);
	m_InputStream->ExactRead(&TotalSize, 4);
	m_InputStream->ExactIgnore(12);

	// Set the total number of bytes
	m_TotalBytes=TotalSize;
	m_BlockNumber=1;

	// A check
	if(NumberLayers != 3)
	{
		//std::cerr << "Information: " << NumberLayers << " layers" << std::endl;
	}

	// Create the layers
	for(unsigned long i=0;i<NumberLayers;i++)
	{
		// Create the layer
		SOldInterleavedLayer* Layer=new SOldInterleavedLayer;

		// Create a new stream for the layer
		Layer->Data = new CBufferDataStream();
		Layer->Stream = NULL;
		Layer->First = true;

		// Push it on
		m_Layers.push_back(Layer);
	}

	// Set the callback functions now that we're able to start reading blocks
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Get the layer
		SOldInterleavedLayer& Layer=*m_Layers[i];

		// Set it
		Layer.Data->SetNeedBufferCallback(BufferCallback, this);
	}

	// Read the first block
	if(!DoReadBlock())
	{
		Clear();
		return false;
	}

	// Initialize the layers
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Get the layer
		SOldInterleavedLayer& Layer = *m_Layers[i];

		try
		{
			// Look at the header type
			uint8_t Type;
			Layer.Data->ExactPeek(&Type, 1);
		
			if (Type == 3 || Type == 5 || Type == 6)
			{
				CVersion5Stream* Stream;
				Stream = new CVersion5Stream(Layer.Data);
				switch (Type)
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
				Layer.Stream = Stream;

				// Initialize the header
				if(!Stream->InitializeHeader(SampleRate))
				{
					Clear();
					return false;
				}
			}
			else if (Type == 8)
			{
				COld6Or4BitStream* Stream;
				Stream = new COld6Or4BitStream(Layer.Data);
				Layer.Type = EUF_UBI_6OR4;
				Layer.Stream = Stream;

				// Initialize the header
				if(!Stream->InitializeHeader())
				{
					Clear();
					return false;
				}
			}
			else
			{
				throw XFileException("Old interleaved format: unrecognized inner stream format");
			}

			// Get channels and sample rate
			m_SampleRate = Layer.Stream->GetSampleRate();
			if(Layer.Stream->GetChannels() == 1)
			{
				m_Stereo = false;
			}
			else
			{
				m_Stereo = true;
			}
		}
		catch(XNeedBuffer&)
		{
			Clear();
			throw(XFileException("The decoder needed more information than the header provided"));
		}
	}
	return true;
}

bool COldInterleavedStream::DoDecodeLayer(unsigned long LayerIndex)
{
	// Go through each of the layers
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Get the layer
		SOldInterleavedLayer& Layer=*m_Layers[i];

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
				if(!Layer.Stream->Decode(m_OutputBuffer, RequestAmount))
				{
					return false;
				}
				m_OutputBufferUsed=RequestAmount;
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

bool COldInterleavedStream::DoReadBlock()
{
	// Check for the end of the file
	if(m_InputStream->IsEnd() || m_InputStream->Tell()>=m_TotalBytes)
	{
		// Mark the end of the stream for all of the layers
		for(unsigned long i=0;i<m_Layers.size();i++)
		{
			SOldInterleavedLayer& Layer=*m_Layers[i];
			Layer.Data->EndStream();
		}
		return false;
	}

	// Process the first block header
	unsigned long BlockID;
	m_InputStream->ExactRead(&BlockID, 4);
	if(BlockID!=m_BlockNumber)
	{
		throw(XFileException("Error: Invalid block ID"));
	}
	m_BlockNumber++;
	m_InputStream->ExactIgnore(4);

	// Read in the block sizes
	std::vector<unsigned long> BlockSizes;
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		unsigned long BlockSize;
		m_InputStream->ExactRead(&BlockSize, 4);
		BlockSizes.push_back(BlockSize);
	}

	// Go through each of the layers
	for(unsigned long i=0;i<m_Layers.size();i++)
	{
		// Get a reference to the layer
		SOldInterleavedLayer& Layer=*m_Layers[i];
		Layer.First=false;

		// Feed it a block
		void* Buffer;
		Buffer=m_InputStream->ExactRead(BlockSizes[i]);
		Layer.Data->SendBuffer(Buffer, BlockSizes[i]);
	}
	return true;
}

bool COldInterleavedStream::LayerExtract(CDataStream* Input, unsigned long Layer, std::ostream& Output)
{
	// Read the type from the file
	unsigned short Type;
	if(Input->CanSeekBackward())
	{
		Input->SeekToBeginning();
	}
	Input->ExactRead(&Type, 2);
	if(Type!=2)
	{
		throw(XFileException("File does not have the correct signature (should be 02)"));
	}

	// Read the header
	unsigned long NumberLayers;
	unsigned long TotalSize;
	Input->ExactIgnore(2);
	Input->ExactRead(&NumberLayers, 4);
	Input->ExactRead(&TotalSize, 4);
	Input->ExactIgnore(12);

	// Set the total number of bytes
	unsigned long BlockNumber=1;

	// A check
	if(NumberLayers!=3)
	{
		std::cerr << "Information: " << NumberLayers << " layers" << std::endl;
	}

	// Loop through all the blocks
	while(true)
	{
		// Check for the end of the file
		if(Input->IsEnd() || Input->Tell()>=TotalSize)
		{
			break;
		}

		// Process the first block header
		unsigned long BlockID;
		Input->ExactRead(&BlockID, 4);
		if(BlockID!=BlockNumber)
		{
			throw(XFileException("Error: Invalid block ID"));
		}
		BlockNumber++;
		Input->ExactIgnore(4);

		// Read in the block sizes
		std::vector<unsigned long> BlockSizes;
		for(unsigned long i=0;i<NumberLayers;i++)
		{
			unsigned long BlockSize;
			Input->ExactRead(&BlockSize, 4);
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
	}
	return true;
}

unsigned long COldInterleavedStream::GetSampleRate() const
{
	return m_SampleRate;
}

unsigned char COldInterleavedStream::GetChannels() const
{
	return m_Stereo ? 2 : 1;
}

std::string COldInterleavedStream::GetFormatName() const
{
	return "ubi_iv2";
}

EUbiFormat COldInterleavedStream::GetType(unsigned long Layer) const
{
	// Check the state
	if(Layer<0 || Layer>=m_Layers.size())
	{
		throw(XUserException("The layer number is not valid"));
	}

	// Get a reference to the layer
	const SOldInterleavedLayer& LayerRef=*m_Layers[Layer];
	return LayerRef.Type;
}

unsigned long COldInterleavedStream::GetTotalBytes() const
{
	return m_TotalBytes;
}

unsigned long COldInterleavedStream::GetLayerCount() const
{
	return (unsigned long)m_Layers.size();
}


void COldInterleavedStream::DoRegisterParams()
{
	//RegisterParam("Layer", (TSetLongParamProc)SetLayer, NULL, (TGetLongParamProc)GetLayer, NULL);
	//RegisterParam("SampleRate", (TSetLongParamProc)SetSampleRate, NULL, (TGetLongParamProc)GetSampleRate, NULL);
	return;
}

void COldInterleavedStream::Clear()
{
	for(std::vector<SOldInterleavedLayer*>::iterator Iter=m_Layers.begin();Iter!=m_Layers.end();++Iter)
	{
		delete *Iter;
	}
	m_Layers.clear();
	m_OutputBufferOffset=0;
	m_OutputBufferUsed=0;
	m_Initialized=false;
	return;
}

void COldInterleavedStream::BufferCallback(CBufferDataStream& Stream, unsigned long Bytes, void* UserData)
{
	// Check the user data
	if(!UserData)
	{
		return;
	}

	// Get the audio stream class
	COldInterleavedStream& Audio=*(COldInterleavedStream*)UserData;

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
