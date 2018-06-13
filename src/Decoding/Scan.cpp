// Scan.cpp : Scan for UbiSoft format audio in files
//

#include "Pch.h"
#include "Scan.h"

#define SwapLong(_Value) (((unsigned long)(_Value)&0xFF000000)>>24 | ((unsigned long)(_Value)&0x00FF0000)>>8 | \
                        ((unsigned long)(_Value)&0x000000FF)<<24 | ((unsigned long)(_Value)&0x0000FF00)<<8)
#define SwapShort(_Value) (((unsigned short)(_Value)&0xFF00)>>8 | (((unsigned short)(_Value)&0x00FF)<<8)

// Check an Ogg chunk
static bool CheckOggChunk(std::istream& Input, std::streamsize& FullSize)
{
    // Some assumptions
    unsigned char Char[4];

    // Read in the characters
    Input.read((char*)Char, 4);
    Input.seekg(-4, std::ios_base::cur);

    // Check them
    if (memcmp(Char, "OggS", 4)!=0)
    {
        return false;
    }

    // Walk the chain
    bool First=true;
    unsigned char Header[27];
    FullSize=0;
    while (true)
    {
        unsigned char* Segments=0;
        unsigned long HeaderSize=0;
        unsigned long PageSize=0;

        memset(Header, 0, 27);
        Input.read((char*)Header, 27);
        if (memcmp(Header, "OggS", 4)!=0)
        {
            break;
        }
        HeaderSize=27+Header[26];
        Segments=new unsigned char[Header[26]];
        Input.read((char*)Segments, Header[26]);
        for (unsigned long i=0;i<Header[26];i++)
        {
            PageSize+=Segments[i];
        }
        PageSize+=HeaderSize;
        FullSize+=PageSize;
        if (First)
        {
            if (Header[5]&0x02)
            {
                First=false;
            }
            else
            {
                // We found the middle of the stream
                delete[] Segments;
                return false;
            }
        }
        if (Header[5]&0x04)
        {
            delete[] Segments;
            break;
        }
        Input.seekg(PageSize-HeaderSize, std::ios_base::cur);
        delete[] Segments;
    }
    return true;
}

// Do the actual scanning, to figure out roughly when the next audio is
static bool DoScan(std::istream& Input, std::streamoff EndOffset, std::streamsize& BytesRead, std::streamsize& FullSize, ScanCallback& callback)
{
    // Just check first
    if (Input.tellg()>=EndOffset)
    {
        return false;
    }

    // Some variables
    const unsigned long InputBufferLength=65535;
    unsigned char* Buffer=new unsigned char[InputBufferLength];
    std::streamsize BytesLeft=EndOffset-Input.tellg();
    std::streamoff StartOffset=Input.tellg();
    FullSize=0;

	int lastProgress = -1;

    // The scanning loop
    while (Input.tellg()<EndOffset)
    {
		Input.clear();

        // Calculate the amount of data that needs to be read
        std::streamsize NextRead;
        std::streamoff CurrentOffset=Input.tellg();
        if (BytesLeft>InputBufferLength)
        {
            NextRead=InputBufferLength;
        }
        else
        {
            NextRead=BytesLeft;
        }

		// Check for progress updates
		const int progress = (int) ((unsigned long long) CurrentOffset * 100 / EndOffset);
		if (progress != lastProgress)
		{
			if (!callback.progress(progress))
			{
				delete [] Buffer;
                return false;
			}
			lastProgress = progress;
		}

        // This should not happen, but we'll check for it anyways
        if (!NextRead)
        {
            break;
        }

        // Read the data
        Input.read((char*)Buffer, (std::streamsize)NextRead);
        BytesLeft-=NextRead;

        // Process the data in a for loop
        std::streamoff OffsetReset=Input.tellg();
        for (unsigned long i=0;i<(unsigned long)NextRead;i++)
        {
            // Store some variables
            std::streamoff ChunkStart=(std::streamoff)CurrentOffset+i;

            if (Buffer[i] == 3 || Buffer[i] == 5)
            {
                // Some assumptions
                unsigned char Char[28];
                bool ChunkValid=false;

                // Read in the characters
                Input.seekg(ChunkStart);
                Input.read((char*)Char, 28);

                // Check the characters
                if (Char[9]==0 && Char[10]==0 && Char[11]==0 && Char[18]<89 && \
                        (Char[12]==0 || Char[12]==1) && Char[22]<89 /*&& Char[23]<5*/)
                {
                    ChunkValid=true;
                }

                // Check some other conditions
                if (ChunkValid && (Char[0]==3))
                {
                    if (Char[14]!=0 || Char[15]!=10)
                    {
                        ChunkValid=false;
                    }
                }
                else if (ChunkValid && (Char[0]==5))
                {
                    if (Char[14]!=10 || Char[15]!=0)
                    {
                        ChunkValid=false;
                    }
                }

                // If the chunk is valid
                if (ChunkValid)
                {
                    // The file is valid so far, so return success
                    Input.seekg(ChunkStart);
                    BytesRead=ChunkStart-StartOffset;
                    delete [] Buffer;
                    return true;
                }

                // If this was just a false alarm
                Input.seekg((std::streamoff)(OffsetReset));
            }
			else if (Buffer[i] == 6)
            {
                // Some assumptions
                unsigned char Char[36];
                bool ChunkValid=false;

                // Read in the characters
                Input.seekg(ChunkStart);
                Input.read((char*)Char, 36);

                // Check the characters
                if (Char[9]==0 && Char[10]==0 && Char[11]==0 && Char[18]<89 && \
                        (Char[12]==0 || Char[12]==1) && Char[22]<89 /*&& Char[23]<5*/)
                {
                    ChunkValid=true;
                }

                // Check some other conditions
                if (ChunkValid && (Char[0] == 6))
                {
                    if (Char[14] != 10 || Char[15] != 0)
                    {
                        ChunkValid=false;
                    }
                }

				for (unsigned int j = 28; j < 36 && ChunkValid; j++)
				{
					if (Char[j] != 0)
						ChunkValid = false;
				}

                // If the chunk is valid
                if (ChunkValid)
                {
                    // The file is valid so far, so return success
                    Input.seekg(ChunkStart);
                    BytesRead=ChunkStart-StartOffset;
                    delete [] Buffer;
                    return true;
                }

                // If this was just a false alarm
                Input.seekg((std::streamoff)(OffsetReset));
            }
            else if (Buffer[i]==2)
            {
                // Some assumptions
                unsigned char Char[24];
                bool ChunkValid=false;

                // Read in the characters
                Input.seekg(ChunkStart);
                Input.read((char*)Char, 24);

                // Get the full size
                unsigned long NumberLayers;
                FullSize=*((uint32_t*)(Char+8)) /* + 2 */;
                NumberLayers=*((uint32_t*)(Char+4));

                // Check the characters
                if (Char[0]==2 && Char[1]==0 && Char[2]==0 && Char[3]==0 && \
                        Char[5]==0 && Char[6]==0 && Char[7]==0)
                {
                    ChunkValid=true;
                }

                // Check the full size
                if (ChunkValid && (FullSize<64 || FullSize>EndOffset-ChunkStart))
                {
                    ChunkValid=false;
                }

                // Walk the blocks
                if (ChunkValid)
                {
                    for (unsigned long i=1;Input.tellg()<ChunkStart+FullSize-2;i++)
                    {
                        unsigned long Signature;
                        std::streamsize TotalBytes=0;
                        Signature=0;

                        Input.read((char*)&Signature, 4);
                        Input.seekg(4, std::ios_base::cur);
                        for (unsigned long j=0;j<NumberLayers;j++)
                        {
                            unsigned long Size=0;
                            Input.read((char*)&Size, 4);
                            TotalBytes+=Size;
                        }

                        if (Signature!=i)
                        {
                            ChunkValid=false;
                            break;
                        }
                        if (TotalBytes>=EndOffset-ChunkStart || TotalBytes<(std::streamsize)NumberLayers*4+8)
                        {
                            ChunkValid=false;
                            break;
                        }
                        Input.seekg(TotalBytes, std::ios_base::cur);
                    }
                }

                // If the chunk is valid
                if (ChunkValid)
                {
                    // The file is valid so far, so return success
                    Input.seekg(ChunkStart);
                    BytesRead=ChunkStart-StartOffset;
                    delete [] Buffer;
                    return true;
                }

                // If this was just a false alarm
                Input.seekg((std::streamoff)(OffsetReset));
                FullSize=0;
            }
            else if (Buffer[i]==8)
            {
                // Some assumptions
                unsigned char Char[48];
                bool ChunkValid=false;

                // Read in the characters
                Input.seekg(ChunkStart);
                Input.read((char*)Char, 48);

                // Check the characters
                if (Char[0]==8 && Char[1]==0 && Char[2]==0 && \
                        Char[3]==0 && Char[37]==0 && Char[38]==0 && \
                        Char[39]==0 && (Char[36]==4 || Char[36]==6) && \
                        Char[45]==0 && Char[46]==0 && Char[47]==0 && \
                        (Char[44]==1 || Char[44]==2))
                {
                    ChunkValid=true;
                }

                // Walk the blocks
                if (ChunkValid)
                {
                    char BlockHeader[52];
                    std::streamoff LastValidOffset;
                    bool Done=false;
                    bool FoundABlock=false;
                    while (!Input.eof())
                    {
                        for (unsigned long i=0;i<Char[44];i++)
                        {
                            Input.read(BlockHeader, 52);
                            if (BlockHeader[0]!=2 || BlockHeader[1]!=0 || \
                                    BlockHeader[2]!=0 || BlockHeader[3]!=0)
                            {
                                Done=true;
                                break;
                            }
                        }
                        Input.seekg(Char[36]*384+2, std::ios_base::cur);
                        if (Done)
                        {
                            break;
                        }
                        LastValidOffset=Input.tellg();
                        FoundABlock=true;
                    }

                    if (!FoundABlock)
                    {
                        ChunkValid=false;
                    }
                    else
                    {
                        //FullSize=LastValidOffset-ChunkStart;
                    }
                }

                // If the chunk is valid
                if (ChunkValid)
                {
                    // The file is valid so far, so return success
                    Input.clear();
                    Input.seekg(ChunkStart);
                    BytesRead=ChunkStart-StartOffset;
                    delete [] Buffer;
                    return true;
                }

                // If this was just a false alarm
                Input.seekg((std::streamoff)(OffsetReset));
                FullSize=0;
            }
            if (Buffer[i]==8 || Buffer[i]==7)
            {
                // Some assumptions
                unsigned char Char[28];
                bool ChunkValid=false;
                bool BigEndian=false;
                unsigned char Variant=0;

                // Read in the characters
                Input.seekg(ChunkStart);
                Input.read((char*)Char, 28);

                // Check the characters
                if ((Char[0]==8 || Char[0]==7) && Char[1]==0 && Char[3]==0 && \
                        Char[9]==0 && Char[10]==0 && Char[11]==0)
                {
                    ChunkValid=true;
                    BigEndian=false;

                    if (Char[0]==7)
                    {
                        Variant=2;
                    }
                }
                else if (ChunkStart>=3)
                {
                    // Adjust the chuck size and reread
                    ChunkStart-=3;
                    Input.seekg(ChunkStart);
                    Input.read((char*)Char, 28);

                    // Now check the characters
                    if ((Char[3]==8 || Char[3]==7) && Char[2]==0 && Char[0]==0 && \
                            Char[8]==0 && Char[9]==0 && Char[10]==0)
                    {
                        ChunkValid=true;
                        BigEndian=true;

                        if (Char[3]==7)
                        {
                            Variant=2;
                        }
                    }
                }

                // Get some information
                uint32_t NumberLayers=*((uint32_t*)(Char+8));
                uint32_t NumberBuffers=*((uint32_t*)(Char+12));
                uint32_t OffsetToHeaders=*((uint32_t*)(Char+16));
                uint32_t HeaderSkip=*((uint32_t*)(Char+20));

                if (BigEndian)
                {
                    NumberLayers=SwapLong(NumberLayers);
                    NumberBuffers=SwapLong(NumberBuffers);
                    OffsetToHeaders=SwapLong(OffsetToHeaders);
                    HeaderSkip=SwapLong(HeaderSkip);
                }

                if (Variant==2)
                {
                    NumberBuffers=OffsetToHeaders;
                    Input.seekg(32, std::ios_base::cur);
                    HeaderSkip=0;
                    Input.read((char*)&HeaderSkip, 4);
                    if (BigEndian)
                    {
                        HeaderSkip=SwapLong(HeaderSkip);
                    }
                }
                else if (OffsetToHeaders!=NumberLayers*4+8)
                {
                    Variant=1;
                    NumberBuffers=OffsetToHeaders;
                    Input.seekg(44, std::ios_base::cur);
                    HeaderSkip=0;
                    Input.read((char*)&HeaderSkip, 4);
                    if (BigEndian)
                    {
                        HeaderSkip=SwapLong(HeaderSkip);
                    }
                }

                // Verify the information
                if (HeaderSkip>=EndOffset-ChunkStart || HeaderSkip<(std::streamsize)NumberLayers*4)
                {
                    ChunkValid=false;
                }
                if (NumberLayers==0)
                {
                    ChunkValid=false;
                }
                if (NumberBuffers<1)
                {
                    ChunkValid=false;
                }

                // Walk the blocks
                if (ChunkValid)
                {
                    Input.seekg(HeaderSkip, std::ios_base::cur);
                    for (unsigned long i=0;i<NumberBuffers;i++)
                    {
                        uint32_t Signature;
                        std::streamsize TotalBytes=0;

                        if (Variant==0)
                        {
                            Signature=0;
                            Input.read((char*)&Signature, 4);
                            if (BigEndian)
                            {
                                Signature=SwapLong(Signature);
                            }
                            Input.seekg(4, std::ios_base::cur);
                        }
                        else if (Variant==1 || Variant==2)
                        {
                            Signature=0;
                            Input.read((char*)&Signature, 4);
                            if (BigEndian)
                            {
                                Signature=SwapLong(Signature);
                            }
                            Input.seekg(4, std::ios_base::cur);
                            if (Signature!=i+1)
                            {
                                ChunkValid=false;
                                break;
                            }
                            Signature=0;
                            Input.read((char*)&Signature, 4);
                            if (BigEndian)
                            {
                                Signature=SwapLong(Signature);
                            }
                        }

                        for (unsigned long j=0;j<NumberLayers;j++)
                        {
                            unsigned long Size;
                            Size=0;
                            Input.read((char*)&Size, 4);
                            if (BigEndian)
                            {
                                Size=SwapLong(Size);
                            }
                            TotalBytes+=Size;
                        }

                        if (Signature!=3)
                        {
                            ChunkValid=false;
                            break;
                        }
                        if (TotalBytes>=EndOffset-ChunkStart || TotalBytes<(std::streamsize)NumberLayers*4+8)
                        {
                            ChunkValid=false;
                            break;
                        }
                        Input.seekg(TotalBytes, std::ios_base::cur);
                    }
                    FullSize=Input.tellg()-ChunkStart;
                }

                // If the chunk is valid
                if (ChunkValid)
                {
                    // The file is valid so far, so return success
                    Input.seekg(ChunkStart);
                    BytesRead=ChunkStart-StartOffset;
                    delete [] Buffer;
                    return true;
                }

                // If this was just a false alarm
                Input.seekg((std::streamoff)(OffsetReset));
                FullSize=0;
            }
            else if (Buffer[i]==9)
            {
                // Some assumptions
                unsigned char Char[20];
                bool ChunkValid=false;
                bool BigEndian=false;
                unsigned char Variant=0;

                // Read in the characters
                Input.seekg(ChunkStart);
                Input.read((char*)Char, 20);

                // Check the characters
                if (Char[0]==9 && Char[1]==0 && Char[2]==16 && Char[3]==0 && \
                        Char[4]==0 && Char[5]==0 && Char[6]==0 && Char[7]==0)
                {
                    ChunkValid=true;
                    BigEndian=false;
                }
                else if (ChunkStart>=3)
                {
                    // Adjust the chuck size and reread
                    ChunkStart-=3;
                    Input.seekg(ChunkStart);
                    Input.read((char*)Char, 20);
                    
                    if (Char[0]==0 && Char[1]==16 && Char[2]==0 && Char[3]==9 && \
                            Char[4]==0 && Char[5]==0 && Char[6]==0 && Char[7]==0)
                    {
                        ChunkValid=true;
                        BigEndian=true;
                    }
                }

                // Get some information
                uint32_t NumberLayers=*((uint32_t*)(Char+8));
                uint32_t NumberBuffers=*((uint32_t*)(Char+12));
                uint32_t TotalInfoSize=*((uint32_t*)(Char+16));

                if (BigEndian)
                {
                    NumberLayers=SwapLong(NumberLayers);
                    NumberBuffers=SwapLong(NumberBuffers);
                    TotalInfoSize=SwapLong(TotalInfoSize);
                }

                // Verify the information
                if (NumberLayers>64)
                {
                    ChunkValid=false;
                }
                if (TotalInfoSize>=EndOffset-ChunkStart)
                {
                    ChunkValid=false;
                }
                if (NumberLayers==0)
                {
                    ChunkValid=false;
                }
                if (NumberBuffers<1)
                {
                    ChunkValid=false;
                }

                // Walk the blocks
                if (ChunkValid)
                {
                    Input.seekg(TotalInfoSize + (64 - NumberLayers*4), std::ios_base::cur);

                    unsigned int HeaderSizes = 0;

                    for (unsigned int i=0;i<NumberLayers;i++)
                    {
                        unsigned long Size = 0;
                        Input.read((char*)&Size, 4);
                        if (BigEndian)
                        {
                            Size = SwapLong(Size);
                        }
                        HeaderSizes += Size;
                    }

                    if (HeaderSizes > EndOffset - ChunkStart)
                    {
                        ChunkValid = false;
                    }
                    else
                    {
                        Input.seekg(HeaderSizes, std::ios_base::cur);

                        for (unsigned long i=0;i<NumberBuffers;i++)
                        {
                            unsigned long Signature;
                            std::streamsize TotalBytes=0;

                            if (Variant==0)
                            {
                                Signature=0;
                                Input.read((char*)&Signature, 4);
                                if (BigEndian)
                                {
                                    Signature=SwapLong(Signature);
                                }
                                Input.seekg(4, std::ios_base::cur);
                            }

                            for (unsigned long j=0;j<NumberLayers;j++)
                            {
                                unsigned long Size;
                                Size=0;
                                Input.read((char*)&Size, 4);
                                if (BigEndian)
                                {
                                    Size=SwapLong(Size);
                                }
                                TotalBytes+=Size;
                            }

                            if (Signature!=3)
                            {
                                ChunkValid=false;
                                break;
                            }
                            if (TotalBytes>=EndOffset-ChunkStart)
                            {
                                ChunkValid=false;
                                break;
                            }
                            Input.seekg(TotalBytes, std::ios_base::cur);
                        }
                        FullSize=Input.tellg()-ChunkStart;
                    }
                }

                // If the chunk is valid
                if (ChunkValid)
                {
                    // The file is valid so far, so return success
                    Input.seekg(ChunkStart);
                    BytesRead=ChunkStart-StartOffset;
                    delete [] Buffer;
                    return true;
                }

                // If this was just a false alarm
                Input.seekg((std::streamoff)(OffsetReset));
                FullSize=0;
            }
            else if (Buffer[i]==79)
            {
                // Set the file offset
                Input.seekg(ChunkStart);

                // Check the chunk
                if (CheckOggChunk(Input, FullSize))
                {
                    // The file is valid so far, so return success
                    Input.seekg(ChunkStart);
                    BytesRead=ChunkStart-StartOffset;
                    delete [] Buffer;
                    return true;
                }

                // If this was just a false alarm
                Input.seekg((std::streamoff)(OffsetReset));
                FullSize=0;
            }
        }
    }

    // Clean up
    delete [] Buffer;
    return false;
}

// List the UbiSoft format audio chunks in the file
bool ScanAndList(std::istream& input, std::streamoff endOffset, ScanCallback& callback)
{
    unsigned long NumberFound=0;
    std::streamsize BytesRead;
    std::streamsize SizeReadFromFile=0;

    // Scan for the first chunk
    bool Found=DoScan(input, endOffset, BytesRead, SizeReadFromFile, callback);

    // Loop, until we could find no more chunks
    while (Found)
    {
        // Save the offset of the current chunk (already found)
        std::streamoff ChunkOffset=input.tellg();

        // Seek past the current chunk, saving the current chunk size
        std::streamsize ChunkSize;
        if (SizeReadFromFile)
        {
            ChunkSize=SizeReadFromFile;
            SizeReadFromFile=0;
            input.seekg(ChunkSize, std::ios_base::cur);
        }
        else
        {
            ChunkSize=0;
            input.seekg(28, std::ios_base::cur);
        }

        // Scan for the next chunk
        Found=DoScan(input, endOffset, BytesRead, SizeReadFromFile, callback);
        if (Found)
        {
            // We already passed the header so we don't find it again
            BytesRead+=28;
        }
        else
        {
            // Assume it goes to the end of the file
            BytesRead=endOffset-ChunkOffset;
        }

        // Make sure the chunk has some reasonable size
        if (!ChunkSize && BytesRead<48)
        {
            // Assume it is a simple chunk
            // Skip to the next file; this one is too small
            // The next file cannot start at the next byte
            input.seekg(29, std::ios_base::cur);
            Found=DoScan(input, endOffset, BytesRead, SizeReadFromFile, callback);
            continue;
        }

        // Set some variables
        if (!ChunkSize)
        {
            ChunkSize=BytesRead;
        }

		// Call our callback with the segment
		NumberFound++;
		if (!callback.foundSegment(ChunkOffset, ChunkSize))
			break;

        //std::cout << ChunkOffset << "\t" << ChunkSize;
        

        /*unsigned char Char[36];
        std::streamoff Prev=Input.tellg();
        Input.seekg(ChunkOffset);
        Input.read((char*)Char, 36);
        std::cout << "\t";
        for(unsigned long j=0;j<36;j++)
        {
                std::cout << (int)Char[j] << "\t";
        }
        Input.seekg(Prev);*/

        //std::cout << std::endl;
    }
    //std::cerr << "Found: " << NumberFound << std::endl;

    // If nothing was found, return false
    if (!Found)
    {
        return false;
    }
    return true;
}
