# Overview
Wolkenbase reads a point cloud in LAS format and extracts the ground layer. It can write the ground and nonground points to separate files, or write them all to one file with the classification byte showing which is which. You can also tell it to limit the number of points per file.

The GUI program needs Qt5; the command-line program needs Boost.

Wolkenbase compiles on Ubuntu Focal and Eoan and DragonFly 5.9.

If building on MinGW, clone the repo https://github.com/meganz/mingw-std-threads and put it beside this one.

To compile, if you're not developing the program:

1. Create a subdirectory build/ inside the directory where you untarred the source code.
2. cd build
3. cmake ..
4. make

If you are developing the program:

1. Create a directory build/wolkenbase outside the directory where you cloned the source code.
2. cd build/wolkenbase
3. cmake \<directory where the source code is\>
4. make
