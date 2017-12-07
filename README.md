
&nbsp;<details><summary>Building under Ubuntu distribution notice</summary>

To be able to build [LAMURE](https://github.com/vrsys/lamure) framework, one has to manage following dependencies:

1. Get [CGAL 4.4](http://www.cgal.org/), [schism](https://github.com/chrislu/schism) and [Boost 1.62](http://www.boost.org/).
A very useful shortcut would be to get builds from [here](https://1drv.ms/f/s!ApCtNlJREf82dWvZ7hhdj36-HRs) and unpack them with:

    ```
    cd /
    sudo tar xvfj <name>.tar
    ```
2. Next, install
[OpenGL](www.opengl.org/),
[freeimage](freeimage.sourceforge.net/),
[GMP](gmplib.org/),
[MPFR](www.mpfr.org/),
[freeglut](freeglut.sourceforge.net/) via:

    ```
    sudo apt-get install libgl1-mesa-dev mesa-common-dev libfreeimageplus-dev libfreeimageplus-doc libfreeimageplus3 libgmp3-dev libmpfr-dev libmpfr-doc libmpfr4 libmpfr4-dbg freeglut3-dev
    ```
3. Install Gnu Compiler Collections 4.8:

    ```
    sudo apt-get install gcc-4.8 g++-4.8
    ```

    Make sure that CMake configuration is done properly:

    ```
    CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/g++-4.8
    CMAKE_C_COMPILER:FILEPATH=/usr/bin/gcc-4.8
    ```
&nbsp;</details>
