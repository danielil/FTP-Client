/**
* @author Daniel Sebastian Iliescu
* @file FtpSock.cpp
* @date 2014-12-12

* This file contains the implementation of the FTP socket wrapper class
*/

#include "FtpSock.h"	// FtpSocket

void ShowError(const char* msg)
{
	perror(msg);
}

FtpSocket::FtpSocket(void) { }

FtpSocket::~FtpSocket(void)
{
	Close();
}

void FtpSocket::Close(void)
{
	close(_socket);
}

// Connects the socket to the given host through specified communication port
bool FtpSocket::ConnectClientSocket(const char* hostAddress, int port)
{
	if (hostAddress == NULL)
	{
		return false;
	}

	if (port == 0)
	{
		port = DEFAULT_FTP_PORT;
	}

	// Fill in the data needed for connecting to the server.
	SOCKADDR_IN sockAddr;

	memset(&sockAddr, 0, sizeof(SOCKADDR_IN));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr(hostAddress);
	sockAddr.sin_port = htons(port);

	if (sockAddr.sin_addr.s_addr == INADDR_NONE)
	{
		LPHOSTENT pHost = gethostbyname(hostAddress);

		if (pHost == NULL)
		{
			ShowError("Cannot find hostname.");
			return false;
		}

		LPIN_ADDR pInAddr = (LPIN_ADDR)(*pHost->h_addr_list);
		sockAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*pInAddr));
	}

	// Open a TCP socket (an Internet stream socket)
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket < 0)
	{
		ShowError("Cannot open a client TCP socket.");
		return false;
	}

	// Connect to the server
	int pass = 0;
	while (connect(_socket, (const SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
	{
		if (++pass >= CONNECT_RETRIES)
		{
			Close();
			ShowError("Cannot connect client TCP socket.");
			return false;
		}
	}

	return true;
}

// Sends a message to partner socket
int FtpSocket::SendMessage(void* pDataBuffer, int dataSize)
{
	int noBytes = 0;	// number of bytes sent

	if ((pDataBuffer != NULL) && (dataSize > 0))
	{
		noBytes = send(_socket, (char *)pDataBuffer, dataSize, 0);
		if (noBytes == SOCKET_ERROR)
		{
			ShowError("Failed to send data.");
			noBytes = 0;
		}
	}

	return noBytes;
}

// Receives a message from partner socket 
int FtpSocket::ReceiveMessage(void* pDataBuffer, int bufferSize)
{
	int noBytes = 0;	// number of bytes received

	if ((pDataBuffer != NULL) && (bufferSize > 0))
	{
		noBytes = recv(_socket, (char *)pDataBuffer, bufferSize, 0);
		if (noBytes == SOCKET_ERROR)
		{
			ShowError("Failed to receive data.");
			noBytes = 0;
		}
	}

	return noBytes;
}

// Receives all pending messages from partner socket 
int FtpSocket::ReceiveMessageAll(void* pDataBuffer, int bufferSize)
{
	int noBytes = 0;	// total number of bytes received

	while (noBytes < bufferSize)
	{
		unsigned char* pBuffer = (unsigned char *)pDataBuffer + noBytes;
		int maxSize = bufferSize - noBytes;
		int recvSize = ReceiveMessage((void *)pBuffer, maxSize);

		if (recvSize > 0)
		{
			noBytes += recvSize;
		}
		else
		{
			break;
		}
	}

	return noBytes;
}
