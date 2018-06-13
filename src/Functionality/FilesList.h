/*
 FilesList.h : A list of files
*/

#pragma once

namespace NDecFunc
{
	class CSegmentsList;

	class CFilesList : protected std::vector<CSegmentsList*>
	{
	public:
		CFilesList();
		virtual ~CFilesList();

		virtual void Add(CSegmentsList* File);
		virtual void Remove(unsigned long Index);
		virtual void Remove(const std::vector<unsigned long>& Indicies);
		virtual CSegmentsList* Get(unsigned long Index);
		virtual CSegmentsList* Get(std::string Filename);
		virtual bool IsValid(unsigned long Index) const;
		virtual unsigned long GetCount() const;
		virtual void Clear();
		virtual void Scan(std::string InputDir);
	};
};
