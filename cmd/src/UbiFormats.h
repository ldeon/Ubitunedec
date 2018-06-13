// UbiFormats.h : UbiSoft formats
//

#pragma once

enum EUbiFormat
{
	EUF_NULL,
	EUF_UBI_V3,
	EUF_UBI_V5,
	EUF_UBI_V6,
	EUF_UBI_IV2,
	EUF_UBI_IV8,
	EUF_UBI_IV9,
	EUF_UBI_6OR4,
	EUF_UBI_RAW,
	EUF_PCM,
	EUF_RAW = EUF_PCM,
	EUF_OGG
};

inline EUbiFormat StringToUbiFormat(const std::string& String)
{
	if(String=="ubi_v3")
	{
		return EUF_UBI_V3;
	}
	else if(String=="ubi_v5")
	{
		return EUF_UBI_V5;
	}
	else if(String=="ubi_v6")
	{
		return EUF_UBI_V6;
	}
	else if(String=="ubi_iv2")
	{
		return EUF_UBI_IV2;
	}
	else if(String=="ubi_iv8")
	{
		return EUF_UBI_IV8;
	}
	else if(String=="ubi_iv9")
	{
		return EUF_UBI_IV9;
	}
	else if(String=="ubi_6or4")
	{
		return EUF_UBI_6OR4;
	}
	else if(String=="ubi_raw")
	{
		return EUF_UBI_RAW;
	}
	else if(String=="raw")
	{
		return EUF_RAW;
	}
	else if(String=="ogg")
	{
		return EUF_OGG;
	}
	return EUF_UBI_RAW;
}

EUbiFormat DetermineFormat(std::istream& input, std::streamsize size = -1);

// This is somewhat unwise to put this here
#include "DataExceptions.h"
#include "AudioExceptions.h"
#include "Version5Stream.h"
#include "InterleavedStream.h"
#include "Interleaved9Stream.h"
#include "OldInterleavedStream.h"
#include "Old6Or4BitStream.h"
#include "OggVorbisStream.h"
#include "RawCompressedStream.h"
#include "RawPcmStream.h"
