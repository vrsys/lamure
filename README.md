This repository is a fork of [LAMURE](https://github.com/vrsys/lamure) by [Virtual Reality Systems Group at the Bauhaus-Universität Weimar](http://www.uni-weimar.de/medien/vr) created for the [Project Visual Provenance](https://www.uni-weimar.de/de/medien/professuren/vr/teaching/ss-2017/project-visual-provenance/).

>In the field of 3D-Digitalization, a vast number of factors influence the quality of reconstruction and visualization. Therefore, a detailed analysis over the course of data processing is essential for facilitating quality assurance and comparison not only across different sensors but also across different processing pipelines.

>Historically, the term provenance refers to the origin of an object with respect to its whole but also to its details. In this sense, we strive to identify, prioritize, record and visualize so-called provenance-data that aggregates during structure-from-motion (SfM) reconstruction as well as multi-resolution processing of very large scanned datasets. However, the fusion of comprehensive amounts of provenance-data with very large 3D-scans and its real-time access is a challenging problem.

>In this project, we will extract provenance-related meta-/para-data from a series of toolchains, including our scanned data simplification pipeline. Students will design and implement a spatial datastructure to organize and store this information efficiently and with real-time-access demands in mind. Finally, students will create exciting and novel visualizations for exploring, understanding and assessing the quality of digitized material in real-time. We will work with our existing frameworks Lamure and Avango/Guacamole.

&nbsp;<details open><summary>Contributions here are going to be focused at following applications</summary>

### Lamure Visual Provenance Viewer

Lamure Visual Provenance Viewer is a [LAMURE](https://github.com/vrsys/lamure) app,
capable of displaying arbitrary sparse and dense point clouds representing
3D reconstruction output from structure from motion (SfM) tools.

### Sparse data file format

Bytewise specification (following the guidelines of [Fadden](http://www.fadden.com/tech/file-formats.html)). Numerical values in double.

    ```
    +00     2B Magic number (0xAFFE)
    +02     4B CRC-32
    +06     4B Length of data
    +0A     2B Number of cameras

        [start camera]

        +00     2B  Camera index
        +02     8B  Focal length
        +0A     32B Quaternion, WXYZ
        +2A     24B Camera center, XYZ
        +42     2B  Length of file path (YY)
        +44     YY  File path
        +44+YY  4B  Length of meta data (ZZ)
        +48+YY  ZZ  Meta data

        [end camera]

    +??     4B Number of sparse points

        [start sparse point]

        +00     4B  Point index
        +04     24B Position, XYZ
        +1C     24B Color, RGB
        +20     2B  Number of measurements

            [start measurement]

            +00     2B Camera index
            +02     4B Occurence, X
            +06     4B Occurence, Y

            [end measurement]

        +??      4B  Length of meta data (ZZ)
        +??+4B   ZZ  Meta data

        [end sparse point]

    ```

### Dense (LoD) data file format

    ```
    +00     2B Magic number (0xAFFE)
    +02     4B CRC-32
    +06     4B Length of data
    +0A     4B Number of points

    [start dense point]

    +00     4B  Point index
    +04     24B Position, XYZ
    +1C     24B Color, RGB
    +40     24B Normal, XYZ
    +58     4B  Length of meta data (ZZ)
    +5C     ZZ  Meta data

    [end dense point]
    ```

</details>

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