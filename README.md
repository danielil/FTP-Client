FTP-Client
================

An FTP Client Running in Passive Mode and implemented in C++.

About
------------------

version: `1.0.0`
author: `Daniel Sebastian Iliescu, http://dansil.net`
created: `2014-12-12`

Demo
------------------

You can compile the client using g++ with the following syntax:

	g++ Ftp.cpp FtpObj.cpp FtpSock.cpp -o FtpClient

You should be able to run it from the shell using:

	./FtpClient [ip-address] [port-number]

Known Issues
------------------

Only basic commands are implemented for now.
