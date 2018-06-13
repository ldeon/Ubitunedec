// LayeredStreamHelper.h : Layered audio stream decoding helper class
//

#pragma once

#include "StreamHelper.h"

// Provides audio stream decoding
class CLayeredStreamHelper : public CStreamHelper
{
private:
	bool _SetCurrentLayer(long Layer);
	long _GetCurrentLayer();

protected:
	std::vector<unsigned long> m_CurrentLayers;

protected:
	virtual bool IsLayerDecoded(unsigned long LayerIndex) const;
	virtual bool DoDecodeBlock();
	void RegisterLayerParams();
	virtual bool DoDecodeLayer(unsigned long LayerIndex)=0;

public:
	CLayeredStreamHelper(CDataStream* Input);
	virtual ~CLayeredStreamHelper();

	virtual unsigned long GetLayerCount() const=0;
	virtual void SetCurrentLayer(unsigned long LayerIndex);
	virtual void AddCurrentLayer(unsigned long LayerIndex);
	virtual void ClearCurrentLayers();
	virtual unsigned long GetCurrentLayer() const;
	virtual void SetCurrentLayers(const std::vector<unsigned long>& Layers);
};
