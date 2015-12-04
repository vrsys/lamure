// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "statistic.h"

#include <algorithm>
#include <numeric>



Statistic::Statistic()
  : m_values(),
    m_mean(0.0f),
    m_sd(0.0f)
{}


Statistic::~Statistic()
{}



void
Statistic::add(const float& v){
  m_values.push_back(v);
}

void
Statistic::calc(){
  const double sum = std::accumulate(m_values.begin(), m_values.end(), 0.0);
  m_mean = sum / m_values.size();
  const double sq_sum = std::inner_product(m_values.begin(), m_values.end(), m_values.begin(), 0.0);
  m_sd = std::sqrt(sq_sum / m_values.size() - m_mean * m_mean);

}


float
Statistic::getMean() const{
  return m_mean;
}


float
Statistic::getSD() const{
  return m_sd;
}



