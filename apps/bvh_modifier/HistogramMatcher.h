// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef HISTOGRAMMATCHER_H
#define HISTOGRAMMATCHER_H

#include <lamure/types.h>
#include <vector>

class HistogramMatcher
{
 public:

  struct ColorArray {
      std::vector<unsigned char> r;
      std::vector<unsigned char> g;
      std::vector<unsigned char> b;

      void AddColor(const lamure::vec3b& c) {
          r.push_back(c.r);
          g.push_back(c.g);
          b.push_back(c.b);
      }

      size_t Size() const { return r.size(); }
      void Clear() { r.clear(); g.clear(); b.clear(); }
  };

  void InitReference(const ColorArray& colors);
  void Match(ColorArray& colors, double blend_fac = 1.0) const;

 private:

  struct Hist {
#if WIN32
      double c[3][256];
#else
      double c[3][256] = { {}, {}, {} };
#endif
      double* operator[](size_t n) { return c[n]; }
      const double* operator[](size_t n) const { return c[n]; }
  };

  Hist BuildHistogram(const ColorArray& colors) const; 

  Hist reference;
};

#endif // #ifndef HISTOGRAMMATCHER_H

