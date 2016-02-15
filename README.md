SpaceWireRMAPLibrary
----------------------------------------

SpaceWire RMAP Library is an open-source C++ class library for
developments and tests of SpaceWire networks and data transfer over
RMAP. The library is highly modularized, and provides easy-to-use access
to SpaceWire interfaces and a software RMAP stack. The Ruby binding is
also provided using SWIG, and quick prototyping of
SpaceWire/RMAP-related programs can be done without compilation.

The library has been used in many institutes for more than 5 years,
particularly for developments and tests of the small satellite SDS-I
(2009), the space X-ray observatory ASTRO-H (2015), and other small
scientific satellites. Many high-energy astrophysics experiments on
ground or balloon-borne ones also used SpaceWire RMAP Library in their
data acquisition and control systems based on SpaceWire and RMAP.

Now, version 2 of SpaceWire RMAP Library
----------------------------------------

In 2006, SpaceWire RMAP Library v1 was released, and has been used in
many applications and in many institutes. Since RMAP was in a drafting
phase at that time, the RMAP implementation in v1 was tentative, and is
now obsolete in many ways (e.g. naming convention). SpaceWire RMAP
Library v2 which was written from scratch totally replaces v1, with many
new functions which improves flexibility and controllability of
SpaceWire and RMAP functions.

The SpaceWire RMAP Library v1 code is still contained in the v2 folder
so as to allow old users to work with their applications developed for
v1 (see SpaceWireRMAPLibrary/classic/). However, maintenance to the v1
code is suspended, and development power is devoted to v2.

Requirement
-----------

SpaceWire RMAP Library can be used on Mac, and probably on Linux (not
tested). If any Windows user ports the library to Win32 or
Windows+Cygwin, please feedback a resulting source tree to the project.

The library requires
[CxxUtilities](https://github.com/yuasatakayuki/CxxUtilities),
[XMLUtilities](https://github.com/sakuraisoki/XMLUtilities/), and
[xerces-c++](http://xerces.apache.org/xerces-c/) as external libraries.
The former two libraries are bundled with the release archive of
SpaceWire RMAP Library for users’ convenience. Developers using Mac OS X
and Objective-C++, they can use NSXMLParser instead of XMLUtilities (by
giving the “-DXMLUTILITIES\_NSXMLPARSER” flag to the compiler).

When using the Ruby binding of SpaceWire RMAP Library,
[CMake](http://www.cmake.org) and [SWIG](http://www.swig.org) required
to build a Ruby SpaceWire/RMAP module.

Download and Install
--------------------
### Install using Homebrew

```sh
brew tap yuasatakayuki/hxisgd
brew install xerces-c spacewirermaplibrary
```

SpaceWireRMAPLibrary and related libraries will be installed to ```/usr/local/include```.
When compiling an application using SpaceWireRMAPLibrary, one needs to include the path
in header search path by doing e.g. ```-I/usr/local/include```, and include xerces-c by
providing a linker flag ```-lxerces-c```.

### Install from Git repository (recommended)

It is recommended to install from [the github
repository](https://github.com/yuasatakayuki/SpaceWireRMAPLibrary) so as
to catch the latest version.

```sh
#Move to your favorite install directory!
mkdir $HOME/work/install
cd $HOME/work/install

#Cloning libraries from github repositories
git clone https://github.com/yuasatakayuki/SpaceWireRMAPLibrary
git clone https://github.com/yuasatakayuki/CxxUtilities
git clone https://github.com/sakuraisoki/XMLUtilities

#Install xerces-c++
# See http://mxcl.github.com/homebrew/ for details of HomeBrew
brew install xerces-c

#Set environment variables (save in .zshrc/.bashrc)
export SPACEWIRERMAPLIBRARY_PATH=$HOME/work/install/SpaceWireRMAPLibrary
export CXXUTILITIES_PATH=$HOME/work/install/CxxUtilities
export XMLUTILITIES_PATH=$HOME/work/install/XMLUtilities
export XERCESDIR=/usr/local
```

### Update existing repository

If you are using the Github repository version of SpaceWire RMAP
Library, you can update to the latest version by just executing “git
pull” in the folder.

```sh
#Update SpaceWire RMAP Library
cd $SPACEWIRERMAPLIBRARY_PATH
git pull

#Update CxxUtilities
cd $CXXUTILITIES_PATH
git pull

#Update XMLUtilities
cd $XMLUTILITIES_PATH
git pull
```

### Ruby binding

The SpaceWireRMAPLibrary/swig/ folder contains a SWIG wrapper of
SpaceWireRMAPLibrary. By doing like below, you can build and install a
Ruby SpaceWire/RMAP module.

```sh
cd SpaceWireRMAPLibrary/swig/
cmake .
make
make install
# By default, compiled library (Ruby module) is installed to $HOME/lib.
#If you want to change this, do:
# cmake -DCMAKE_INSTALL_PREFIX=install/directory/ .
```

Users need to add a path to a folder which contains the Ruby
SpaceWire/RMAP module to the RUBYLIB environment variable so that ruby
can find the module when required. Set it by doing like below in your
.zshrc or .bashrc:

```sh
export RUBYLIB=$HOME/lib/ruby
#when you changed an install destination using the cmake's -DCMAKE_INSTALL_PREFIX option,
#set an appropriate path instead of $HOME/lib/ruby.
```

See the gist post “[User Ruby wrapper of SpaceWire/RMAP Library](https://gist.github.com/yuasatakayuki/de61b6796847fe68f215)”
for detailed usage of the Ruby binding. (it is almost intuitive if you
have been familiar with C++ SpaceWire RMAP Library).

Documentation
-------------

User guide is available in doc/.

Download the latest
version: [SpaceWireRMAPLibraryUserGuide\_20120110.pdf](https://github.com/yuasatakayuki/SpaceWireRMAPLibrary/raw/master/doc/SpaceWireRMAPLibraryUserGuide_20120110.pdf)

Git repository
--------------

The source code of the software is also available from [the SpaceWire
RMAP Library github
repository](https://github.com/yuasatakayuki/SpaceWireRMAPLibrary).
