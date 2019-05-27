#ifndef LAMURE_VT_PLATFORM_H
#define LAMURE_VT_PLATFORM_H

#ifdef _MSC_VER
#pragma warning (disable: 4251) // needs to have dll-interface to be used by clients of class
#endif

#if WIN32
#if defined(LAMURE_VT_LIBRARY)
     #define VT_DLL __declspec( dllexport )
  #else
    #define VT_DLL __declspec( dllimport )
  #endif
#else
#define VT_DLL
#endif

#endif //LAMURE_VT_PLATFORM_H
