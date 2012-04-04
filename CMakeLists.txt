cmake_minimum_required(VERSION 2.6)

project(sprig)

if(UNIX)
	set(sprig_INSTALL_BY_DEFAULT ON)
else(UNIX)
	set(sprig_INSTALL_BY_DEFAULT OFF)
endif(UNIX)

option(sprig_INSTALL "Install SPriG libs, includes, and CMake scripts" ${sprig_INSTALL_BY_DEFAULT})
option(sprig_BUILD_SHARED "Build SPriG shared libraries" ON)
option(sprig_BUILD_STATIC "Build SPriG static libraries" ON)
option(sprig_BUILD_EXAMPLES "Build SPriG examples" ON)


set(sprig_VERSION 1.2.0)


find_package(SDL REQUIRED)

# Workaround for the non-working REQUIRED flag
if ( NOT SDL_FOUND )
   message ( FATAL_ERROR "SDL not found!" )
endif ( NOT SDL_FOUND )


include_directories(${SDL_INCLUDE_DIR})
include_directories(sprig)

link_libraries (
   ${SDL_LIBRARY}
)

add_definitions("-Wall -std=c99 --pedantic")

# The SPriG library.
add_subdirectory(source)


if(sprig_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif(sprig_BUILD_EXAMPLES)

