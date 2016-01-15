Pion Network Library
====================

C++ framework for building lightweight HTTP interfaces

[![Build Status](https://travis-ci.org/splunk/pion.svg?branch=develop)](https://travis-ci.org/splunk/pion)

**Project Home:** https://github.com/splunk/pion

Retrieving the code
-------------------

    git clone https://github.com/splunk/pion.git
    cd pion


Building the code
-----------------

*For XCode:* use `pion.xcodeproj`

*For Visual Studio:* use `pion.sln`

On Unix platforms (including Linux, OSX, etc.) you can run

    ./autogen.sh
    ./configure

to generate Makefiles using GNU autotools, followed by

    make

to build everything except the unit tests.

You can build and run all the unit tests with

    make check

Generate build using CMake
---------------------------
[CMake](http://www.cmake.org) is cross-platform build generator.
Pion required cmake version 2.8.10+

To generate build call 

    cmake <path to pion clone> [-G <generator name>] [-D<option>...]

for example to generate MSVS2012 Win64 solution run

    git clone git@github.com:splunk/pion.git
    cd pion/build
    cmake .. -G"Visual Studio 11 Win64"

this will create pion_solution.sln for MSVS2012/Win64

if cmake can't find dependency, use -D<option> to control Find<library> modules search behaviour
    
    -DBOOST_ROOT=<path to installed boost libraries>
    -DZLIB_ROOT=<path to installed zlib>
    -DOPENSSL_ROOT_DIR=...
    -DLOG4CPLUS_ROOT=...

Third Party Libraries
---------------------

Pion *requires* the [Boost C++ libraries](http://www.boost.org/) version 1.35
or greater. Please see the `README.boost` file within the `doc` subdirectory
for instructions on how to download, build and install Boost.

For logging, Pion may be configured to:

* use std::cout and std::cerr for logging (the default configuration)

* use one of the following open source libraries:
    * [log4cplus](http://log4cplus.sourceforge.net/) (recommended; `--with-log4cplus`),
    * [log4cxx](http://logging.apache.org/log4cxx/) (`--with-log4cxx`) or
	* [log4cpp](http://log4cpp.sourceforge.net/) (`--with-log4cpp`).

* disable logging entirely (run `configure --disable-logging`)

Detailed build instructions are available for all of the platforms
supported by Pion within the `doc` subdirectory (`README.platform`).


License
-------

Copyright &copy; 2007-2016 Splunk Inc.

The Pion Network Library is published under the
[Boost Software License](http://www.boost.org/users/license.html).
See COPYING for licensing information.
