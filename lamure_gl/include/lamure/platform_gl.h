// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_PLATFORM_GL_H_
#define LAMURE_PLATFORM_GL_H_

#ifdef _MSC_VER
  #pragma warning (disable: 4251) // needs to have dll-interface to be used by clients of class
#endif

#if WIN32
  #if defined(LAMURE_GL_LIBRARY)
    #define GL_DLL __declspec( dllexport )
  #else
    #define GL_DLL __declspec( dllimport )
  #endif
#else
  #define GL_DLL
#endif

#endif // LAMURE_PLATFORM_GL_H_
