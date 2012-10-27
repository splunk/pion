Pion Network Library
====================

C++ framework for building lightweight HTTP interfaces

**Project Home:** https://github.com/cloudmeter/pion


Retrieving the code
-------------------

    git clone git@github.com:cloudmeter/pion.git
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

Copyright &copy; 2007-2012 Cloudmeter, Inc.

The Pion Network Library is published under the
[Boost Software License](http://www.boost.org/users/license.html).
See COPYING for licensing information.
