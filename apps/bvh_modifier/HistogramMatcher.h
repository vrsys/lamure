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

class histogram_matcher
{
 public:

  struct color_array {
      std::vector<unsigned char> r;
      std::vector<unsigned char> g;
      std::vector<unsigned char> b;

      void add_color(const lamure::vec3b_t& c) {
          r.push_back(c.x_);
          g.push_back(c.y_);
          b.push_back(c.z_);
      }

      size_t size() const { return r.size(); }
      void clear() { r.clear(); g.clear(); b.clear(); }
  };

  void init_reference(const color_array& colors);
  void match(color_array& colors, double blend_fac = 1.0) const;

 private:

  struct histogram {
#if WIN32
      double c[3][256];
#else
      double c[3][256] = { {}, {}, {} };
#endif
      double* operator[](size_t n) { return c[n]; }
      const double* operator[](size_t n) const { return c[n]; }
  };

  histogram build_histogram(const color_array& colors) const; 

  histogram reference;
};

#endif // #ifndef HISTOGRAMMATCHER_H

