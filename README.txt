 
Sprig uses CMake (www.cmake.org) for automating the build process.  CMake generates platform-specific build files for Windows, Linux, MacOS, and others.


For Linux systems, run CMake in the base directory:
cmake -G "Unix Makefiles"
make
sudo make install

For Linux systems, using the old default installation directory can be done like so:
cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=/usr

