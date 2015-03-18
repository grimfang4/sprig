The SDL Primitive Generator, SPriG, is a free and open source C/C++ library that does three main things:

* Draws graphics primitives (pixels, lines, circles, polygons, etc.) with optional anti-aliasing, alpha-blending, arbitrary thickness, and dirty rect support.
* Rotates, scales, and mirrors SDL surfaces with optional anti-aliasing and alpha-blending.
* Makes working with SDL surfaces quicker and easier.


Sprig uses CMake (www.cmake.org) for automating the build process.  CMake generates platform-specific build files for Windows, Linux, MacOS, and others.


For Linux systems, run CMake in the base directory:
cmake -G "Unix Makefiles"
make
sudo make install

For Linux systems, using the old default installation directory can be done like so:
cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=/usr

