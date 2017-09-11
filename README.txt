1. unzip externals.zip to directory "externals"

2. Run CMake to generate a solution file and corresponding projects for "Microsoft Visual Studio 2013 Compiler x64"

   Note: All external libraries are only provided for MSVC2013 x64! If you decide to compile for x86 or an older 
   compiler version, you need to compile all external libraries for this platform.
   
3. Configure build option and choose install directory (CMAKE_INSTALL_PREFIX)
   
   Note: All external libraries are provided as pre-compiled binaries and should be found automatically.

4. Generate solution 

5. Open Visual Studio solution file and compile

6. Build "INSTALL" target to copy necessary shader files, fonts and libraries to install path.

Have fun!