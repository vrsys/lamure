// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "HistogramMatcher.h"

void HistogramMatcher::
InitReference(const ColorArray& colors)
{
    reference = BuildHistogram(colors);
}

HistogramMatcher::Hist HistogramMatcher::
BuildHistogram(const ColorArray& colors) const
{
    Hist h;

    #pragma omp parallel for
    for (size_t i = 0; i < colors.Size(); ++i ){
        #pragma omp atomic update
        ++h[0][colors.r[i]];
        
        #pragma omp atomic update
        ++h[1][colors.g[i]];

        #pragma omp atomic update
        ++h[2][colors.b[i]];
    }
    #pragma omp parallel for
    for (size_t i = 0; i < 256; ++i ) {
        h[0][i] /= colors.Size();
        h[1][i] /= colors.Size();
        h[2][i] /= colors.Size();
    }
    return h;
}

void HistogramMatcher::
Match(ColorArray& colors, double blend_fac) const
{
    Hist hist = BuildHistogram(colors);
    Hist T;

    // match histograms
    #pragma omp parallel for
    for (size_t comp = 0; comp < 3; ++comp) {
        double val_1 = 0.0;
        double val_2 = 0.0;
        double S[256] = {};
        double G[256] = {};
        for (int index = 0; index < 256; ++index) {
            val_1 += hist[comp][index];
            val_2 += reference[comp][index];
            S[index] = val_1;
            G[index] = val_2;
        }
        double min_val = 0.0;
        int PG = 0;
        for (int i = 0; i < 256; ++i) {
            min_val = 1.0;
            for (int j = 0; j < 256; ++j) {
                if ((G[j] - S[i]) < min_val && (G[j] - S[i]) >= 0) {
                    min_val = (G[j] - S[i]);
                    PG = j;
                }
            }
            T[comp][i] = static_cast<unsigned char>(PG);
        }
    }

    #pragma omp parallel for
    for (size_t idx = 0; idx < colors.Size(); ++idx){
        colors.r[idx] = static_cast<unsigned char>(blend_fac * T[0][colors.r[idx]] + (1.0 - blend_fac) * colors.r[idx]);
        colors.g[idx] = static_cast<unsigned char>(blend_fac * T[1][colors.g[idx]] + (1.0 - blend_fac) * colors.g[idx]);
        colors.b[idx] = static_cast<unsigned char>(blend_fac * T[2][colors.b[idx]] + (1.0 - blend_fac) * colors.b[idx]);
    }
}

