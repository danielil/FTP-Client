FTP-Client
================

An FTP Client Running in Passive Mode implemented in C++.

About
------------------

version: `1.0.0`  
author: `Daniel Sebastian Iliescu, http://dansil.net`  
license: `MIT License (MIT), http://opensource.org/licenses/MIT`  
created: `2014-12-12`  
updated: '2016-12-04'  

Usage
------------------

The FTP client should be able to compile on most linux distributions with clang/gcc as well as Windows with MSVC.
Note that on Windows, you must statically link with the Winsock library by adding Ws2_32.lib to the linker.

You should be able to run it from any shell with the following syntax:

	./FTPClient [ip-address] [port-number]

Known Issues
------------------

Only basic commands are implemented for now.
