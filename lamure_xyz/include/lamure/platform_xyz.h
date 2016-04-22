// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef XYZ_PLATFORM_H_
#define XYZ_PLATFORM_H_

#ifdef _MSC_VER
  #pragma warning (disable: 4251) // needs to have dll-interface to be used by clients of class
#endif

#if WIN32
  #if defined(LAMURE_XYZ_LIBRARY)
    #define XYZ_DLL __declspec( dllexport )
  #else
    #define XYZ_DLL __declspec( dllimport )
  #endif
#else
  #define XYZ_DLL
#endif

#endif // XYZ_PLATFORM_H_
