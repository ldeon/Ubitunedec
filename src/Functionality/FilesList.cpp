/*
 FilesList.cpp : A list of files
*/

#include "Pch.h"

#include <wx/filename.h>

#include "Functionality/FilesList.h"
#include "Functionality/SegmentsList.h"

// CFilesList Implementation
NDecFunc::CFilesList::CFilesList()
{
	return;
}

NDecFunc::CFilesList::~CFilesList()
{
	Clear();
	return;
}

void NDecFunc::CFilesList::Add(CSegmentsList* File)
{
	if(File)
	{
		push_back(File);
	}
	return;
}

void NDecFunc::CFilesList::Remove(unsigned long Index)
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

void NDecFunc::CFilesList::Remove(const std::vector<unsigned long>& Indicies)
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

NDecFunc::CSegmentsList* NDecFunc::CFilesList::Get(unsigned long Index)
{
	if(!IsValid(Index))
	{
		return NULL;
	}
	return (*this)[Index];
}

NDecFunc::CSegmentsList* NDecFunc::CFilesList::Get(std::string Filename)
{
	// TODO: Function CSegmentsList* Get(std::string Filename)
	return NULL;
}

bool NDecFunc::CFilesList::IsValid(unsigned long Index) const
{
	if(Index>=GetCount())
	{
		return false;
	}
	return true;
}

unsigned long NDecFunc::CFilesList::GetCount() const
{
	return size();
}

void NDecFunc::CFilesList::Clear()
{
	for(iterator Iter=begin();Iter!=end();++Iter)
	{
		delete *Iter;
		*Iter=NULL;
	}
	clear();
	return;
}

void NDecFunc::CFilesList::Scan(std::string InputDir)
{
	// Delete all previous files
	Clear();

	// Some variables
	std::string InputSearch;
	_finddata_t FileInfo;
    intptr_t FindHandle;
	InputSearch=InputDir+"*.*";

	// Search for files
	if((FindHandle=_findfirst(InputSearch.c_str(), &FileInfo))==-1L)
	{
		return;
	}
	do
	{
		// Get the filename
		std::string Filename(InputDir);
		Filename.append(FileInfo.name);

		// Make sure it exists
		if(!wxFileName::FileExists(Filename))
		{
			continue;
		}

		// Create a segment list for this file
		CSegmentsList* SegmentList;
		SegmentList=new CSegmentsList;
		SegmentList->SetFilename(Filename);

		// Scan the file
		SegmentList->ScanFile();

		// Only add it if we found something
		if(SegmentList->GetCount())
		{
			Add(SegmentList);
		}
		else
		{
			delete SegmentList;
		}
	} while(_findnext(FindHandle, &FileInfo)==0);
	_findclose(FindHandle);
	return;
}
