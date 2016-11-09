#include "lamure/pvs/id_histogram.h"
#include <iostream>
#include <fstream>
#include <bitset>

id_histogram::id_histogram()
{
}

id_histogram::~id_histogram()
{
}

void id_histogram::
create(const void* pixelData, const unsigned int& numPixels)
{
	histogram_.clear();
	const unsigned int* pixelDataInt = (unsigned int*)pixelData;

	for(unsigned int index = 0; index < numPixels; ++index)
	{
		unsigned int pixelValue = pixelDataInt[index];
		unsigned int modelID = (pixelValue >> 24) & 0xFF;				// RGBA-value is written in order AGBR, so skip 24 bits to get to model ID within alpha channel.

		if(modelID != 255)
		{
			unsigned int nodeID = pixelValue & 0xFFFFFF;				// RGBA-value is written in order AGBR, so first 24 bits are node ID.
			histogram_[modelID][nodeID]++;
		}
	}
}

std::map<unsigned int, std::vector<unsigned int>> id_histogram::
get_visible_nodes(const unsigned int& numPixels, const float& visibilityThreshold) const
{
	std::map<unsigned int, std::vector<unsigned int>> visibleNodes;

	for(std::map<unsigned int, std::map<unsigned int, unsigned int>>::const_iterator modelIter = histogram_.begin(); modelIter != histogram_.end(); ++modelIter)
	{
		for(std::map<unsigned int, unsigned int>::const_iterator nodeIter = modelIter->second.begin(); nodeIter != modelIter->second.end(); ++nodeIter)
		{
			if(((float)nodeIter->second / (float)numPixels) * 100.0f >= visibilityThreshold)
			{
				visibleNodes[modelIter->first].push_back(nodeIter->first);
			}
		}
	}

	return visibleNodes;
}

std::map<unsigned int, std::map<unsigned int, unsigned int>> id_histogram::
get_histogram() const
{
	return histogram_;
}