// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef ID_HISTOGRAM_H_
#define ID_HISTOGRAM_H_

#include <vector>
#include <map>
#include <utility>

class id_histogram
{
public:
	id_histogram();
	~id_histogram();

	void create(const void* pixelData, const unsigned int& numPixels);
	
	std::map<unsigned int, std::vector<unsigned int>> get_visible_nodes(const unsigned int& numPixels, const float& visibilityThreshold) const;
	std::map<unsigned int, std::map<unsigned int, unsigned int>> get_histogram() const;

private:
	std::map<unsigned int, std::map<unsigned int, unsigned int>> histogram_;			// first key is model ID, second key is node ID, final value is amount of visible pixels
};

#endif
