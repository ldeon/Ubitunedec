// LayeredStreamHelper.cpp : Layered audio stream decoding helper class
//

#include "Pch.h"
#include "LayeredStreamHelper.h"
#include "DataExceptions.h"
#include "AudioExceptions.h"

CLayeredStreamHelper::CLayeredStreamHelper(CDataStream* Input) :
	CStreamHelper(Input)
{
	RegisterLayerParams();
	return;
}

CLayeredStreamHelper::~CLayeredStreamHelper()
{
	return;
}

void CLayeredStreamHelper::SetCurrentLayer(unsigned long LayerIndex)
{
	ClearCurrentLayers();
	AddCurrentLayer(LayerIndex);
	return;
}

void CLayeredStreamHelper::AddCurrentLayer(unsigned long LayerIndex)
{
	// We don't need to see if the layer is in range -- yet
	m_CurrentLayers.push_back(LayerIndex);
	return;
}

void CLayeredStreamHelper::ClearCurrentLayers()
{
	m_CurrentLayers.clear();
	return;
}

unsigned long CLayeredStreamHelper::GetCurrentLayer() const
{
	if(!m_CurrentLayers.size())
	{
		return 0xFFFFFFFF;
	}
	for(unsigned long i=0;i<GetLayerCount();i++)
	{
		if(IsLayerDecoded(i))
		{
			return i;
		}
	}
	return 0xFFFFFFFF;
}

void CLayeredStreamHelper::SetCurrentLayers(const std::vector<unsigned long>& Layers)
{
	// Go through each of the layers passed
	for(std::vector<unsigned long>::const_iterator Iter=Layers.begin();Iter!=Layers.end();++Iter)
	{
		m_CurrentLayers.push_back(*Iter);
	}
	return;
}

bool CLayeredStreamHelper::IsLayerDecoded(unsigned long LayerIndex) const
{
	// Go through each of the layers
	for(std::vector<unsigned long>::const_iterator Iter=m_CurrentLayers.begin();Iter!=m_CurrentLayers.end();++Iter)
	{
		if(*Iter==LayerIndex)
		{
			return true;
		}
	}
	return false;
}

bool CLayeredStreamHelper::DoDecodeBlock()
{
	// Check the number of layers to decode
	unsigned long LayerCount=0;
	for(unsigned long i=0;i<GetLayerCount();i++)
	{
		if(IsLayerDecoded(i))
		{
			LayerCount++;
		}
	}

	// Special cases
	if(LayerCount==0)
	{
		return true;
	}
	else if(LayerCount==1)
	{
		return DoDecodeLayer(GetCurrentLayer());
	}

	// Our output buffer
	short* OutputBuffer=NULL;
	unsigned long OutputBufferLength=0;

	// Go through each of the layers
	for(unsigned long i=0;i<GetLayerCount();i++)
	{
		// Decode it only if we have to
		if(!IsLayerDecoded(i))
		{
			continue;
		}

		// Decode the layer
		if(!DoDecodeLayer(i))
		{
			delete[] OutputBuffer;
			return false;
		}

		// Check our output buffer
		if(!OutputBuffer)
		{
			// Apparently there is no more data
			if(!(m_OutputBufferUsed-m_OutputBufferOffset))
			{
				break;
			}

			// Allocate a buffer
			OutputBufferLength=m_OutputBufferUsed-m_OutputBufferOffset;
			OutputBuffer=new short[OutputBufferLength];

			// Mix the data
			for(unsigned long i=0;i<OutputBufferLength;i++)
			{
				OutputBuffer[i]=m_OutputBuffer[i+m_OutputBufferOffset];
			}
		}
		else
		{
			// Make sure they are the same size
			if(m_OutputBufferUsed-m_OutputBufferOffset!=OutputBufferLength)
			{
				// NO! The buffer MUST be the same size
				delete[] OutputBuffer;
				throw(XFileException("The layers in this file cannot be automatically mixed becuase they are not the same length"));
			}

			// Mix the data
			for(unsigned long i=0;i<OutputBufferLength;i++)
			{
				long Sample=OutputBuffer[i]+m_OutputBuffer[i+m_OutputBufferOffset];
				if(Sample>32767)
				{
					Sample=32767;
				}
				else if(Sample<-32768)
				{
					Sample=-32768;
				}
				OutputBuffer[i]=(short)Sample;
			}
		}
	}

	// Copy to the real output buffer
	m_OutputBufferOffset=0;
	m_OutputBufferUsed=OutputBufferLength;
	memcpy(m_OutputBuffer, OutputBuffer, OutputBufferLength*sizeof(short));
	delete[] OutputBuffer;
	return true;
}

void CLayeredStreamHelper::RegisterLayerParams()
{
	RegisterParam("Layer",
		(TSetLongParamProc)&CLayeredStreamHelper::_SetCurrentLayer, NULL,
		(TGetLongParamProc)&CLayeredStreamHelper::_GetCurrentLayer, NULL);
	return;
}

bool CLayeredStreamHelper::_SetCurrentLayer(long Layer)
{
	if(Layer<1)
	{
		return false;
	}
	SetCurrentLayer(Layer-1);
	return true;
}

long CLayeredStreamHelper::_GetCurrentLayer()
{
	return GetCurrentLayer()+1;
}
