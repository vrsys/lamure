#ifndef LAMURE_PROVENANCE_PLATFORM_H
#define LAMURE_PROVENANCE_PLATFORM_H

#ifdef _MSC_VER
#pragma warning (disable: 4251) // needs to have dll-interface to be used by clients of class
#endif

#if WIN32
#if defined(LAMURE_PROVENANCE_LIBRARY)
     #define PROVENANCE_DLL __declspec( dllexport )
  #else
    #define PROVENANCE_DLL __declspec( dllimport )
  #endif
#else
#define PROVENANCE_DLL
#endif

#endif //LAMURE_PROVENANCE_PLATFORM_H
