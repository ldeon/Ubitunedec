// AudioStream.cpp : Audio stream decoding abstract class
//

#include "Pch.h"
#include "AudioStream.h"

struct CAudioStream::SParamDef
{
	TSetLongParamProc SetLong;
	TSetStringParamProc SetString;
	TGetLongParamProc GetLong;
	TGetStringParamProc GetString;
};

CAudioStream::CAudioStream(CDataStream* Input) :
	m_InputStream(Input),
	m_Params(NULL)
{
	m_Params=new std::map<std::string, SParamDef>;
	return;
}

CAudioStream::~CAudioStream()
{
	delete m_Params;
	return;
}

void CAudioStream::SetInputStream(CDataStream* Input)
{
	m_InputStream=Input;
	return;
}

CDataStream* CAudioStream::GetInputStream() const
{
	return m_InputStream;
}

unsigned long CAudioStream::RecommendBufferLength() const
{
	return 65536;
}

bool CAudioStream::DecodeToFile(std::ostream& Output, unsigned long& NumberSamples)
{
	// Create the buffers
	unsigned long OutputBufferLength=RecommendBufferLength();
	short* OutputBuffer=new short[OutputBufferLength];
	NumberSamples=0;

	// Do the loop
	while(true)
	{
		// Decode some
		unsigned long LocalSamples=OutputBufferLength;
		if(!Decode(OutputBuffer, LocalSamples))
		{
			delete[] OutputBuffer;
			return false;
		}

		// Check if any was decoded
		if(LocalSamples==0)
		{
			break;
		}

		// Write it to the output stream
		NumberSamples+=LocalSamples;
		Output.write((char*)OutputBuffer, LocalSamples*2);
	}

	// Clean up
	delete [] OutputBuffer;
	return true;
}

bool CAudioStream::SetParam(std::string Name, long Value)
{
	// Find the item
	std::map<std::string, SParamDef>::const_iterator Iter;
	Iter=m_Params->find(Name);
	if(Iter==m_Params->end())
	{
		return false;
	}

	// Check the funciton and call it
	if(Iter->second.SetLong==NULL)
	{
		return false;
	}
	return (this->*(Iter->second.SetLong))(Value);
}

bool CAudioStream::SetParam(std::string Name, std::string Value)
{
	// Find the item
	std::map<std::string, SParamDef>::const_iterator Iter;
	Iter=m_Params->find(Name);
	if(Iter==m_Params->end())
	{
		return false;
	}

	// Check the funciton and call it
	if(Iter->second.SetString==NULL)
	{
		return false;
	}
	return (this->*(Iter->second.SetString))(Value);
}

long CAudioStream::GetLongParam(std::string Name) const
{
	// Find the item
	std::map<std::string, SParamDef>::const_iterator Iter;
	Iter=m_Params->find(Name);
	if(Iter==m_Params->end())
	{
		return false;
	}

	// Check the funciton and call it
	if(Iter->second.GetLong==NULL)
	{
		return false;
	}
	return (this->*(Iter->second.GetLong))();
}

std::string CAudioStream::GetStringParam(std::string Name) const
{
	// Find the item
	std::map<std::string, SParamDef>::const_iterator Iter;
	Iter=m_Params->find(Name);
	if(Iter==m_Params->end())
	{
		return false;
	}

	// Check the funciton and call it
	if(Iter->second.GetString==NULL)
	{
		return false;
	}
	return (this->*(Iter->second.GetString))();
}

void CAudioStream::GetParamList(std::vector<std::string> Params) const
{
	// Iterate through all the items
	for(std::map<std::string, SParamDef>::const_iterator Iter=m_Params->begin();Iter!=m_Params->end();++Iter)
	{
		Params.push_back(Iter->first);
	}
	return;
}

void CAudioStream::RegisterParam(std::string Name, TSetLongParamProc SetLong, TSetStringParamProc SetString, TGetLongParamProc GetLong, TGetStringParamProc GetString)
{
	// Check if name exists in the list
	if(m_Params->find(Name)!=m_Params->end())
	{
		m_Params->erase(m_Params->find(Name));
	}

	// Add it to the list
	SParamDef Param;
	Param.SetLong=SetLong;
	Param.SetString=SetString;
	Param.GetLong=GetLong;
	Param.SetString=SetString;
	std::map<std::string, SParamDef>::value_type Value(Name, Param);
	m_Params->insert(Value);
	return;
}
