// UbiFormats.h : Determine the file format of the input
//

#include "Pch.h"
#include "UbiFormats.h"
#include "DataExceptions.h"
#include "AudioExceptions.h"
#include "FileDataStream.h"
#include "InterleavedStream.h"

EUbiFormat DetermineFormat(std::istream& input, std::streamsize size)
{
	std::streamoff beginning = input.tellg();

	// Calculate actual size
	if (size < 1)
	{
		input.seekg(0, std::ios_base::end);
		size = input.tellg() - beginning;
		input.seekg(beginning);
	}

	// Read in the signature
	char magic[4];
	input.read(magic, 4);

	EUbiFormat type = EUF_NULL;

	if(magic[0] == 3)
	{
		type = EUF_UBI_V3;
	}
	else if(magic[0] == 5)
	{
		type = EUF_UBI_V5;
	}
	else if(magic[0] == 6)
	{
		type = EUF_UBI_V6;
	}
	else if(magic[0] == 2)
	{
		type = EUF_UBI_IV2;
	}
	else if(magic[0] == 8 && magic[1] == 0 && magic[2] == 0 && magic[3] == 0)
	{
		// Try a version 8 interleaved stream first
		CFileDataStream FileStream(&input, beginning, size);
		CInterleavedStream Stream(&FileStream);
		try
		{
			std::vector<unsigned long> Layers;
			Layers.push_back(1);
			Stream.SetCurrentLayers(Layers);

			// Initialize
			if(!Stream.InitializeHeader())
			{
				// Not a version 8 so must be a 6-Or-4
				type = EUF_UBI_6OR4;
			}
			else
			{
				short Buffer[1024];
				unsigned long NumberSamples=1024;
				if(Stream.Decode(Buffer, NumberSamples))
				{
					type = EUF_UBI_IV8;
				}
				else
				{
					type = EUF_UBI_6OR4;
				}
			}
		}
		catch (...)
		{
			// Not a version 8 so must be a 6-Or-4
			type = EUF_UBI_6OR4;
		}
	}
	else if(magic[0] == 8 && magic[1] == 0)
	{
		type = EUF_UBI_IV8;
	}
	else if(magic[0] == 9 && magic[1] == 0)
	{
		type = EUF_UBI_IV9;
	}
	else if(magic[3] == 9 && magic[2] == 0)
	{
		type = EUF_UBI_IV9;
	}
	else if(magic[0] == 7 && magic[1] == 0)
	{
		type = EUF_UBI_IV8;
	}
	else if(magic[3] == 8 && magic[2] == 0)
	{
		type = EUF_UBI_IV8;
	}
	else if(magic[0] == 'O' && magic[1] == 'g' && magic[2] == 'g' && magic[3] == 'S')
	{
		type = EUF_OGG;
	}

	input.seekg(beginning);
	return type;
}