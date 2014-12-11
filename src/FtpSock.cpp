/**
* @author Daniel Sebastian Iliescu
* @file FtpSock.cpp
* @date 2014-12-12

* This file contains the implementation of the FTP socket wrapper class
*/

#include "FtpSock.h"    // FtpSocket

void ShowError(const char* msg)
{
	perror(msg);
}

FtpSocket::FtpSocket(void)
{
   _socket = INVALID_SOCKET;        // socket handle
   _socketState = UNKNOWN_SOCKET;   // socket state
}

FtpSocket::~FtpSocket(void)
{
   Close();
}

void FtpSocket::Close(void)
{
	bool bSuccess = true;

	if (_socket != INVALID_SOCKET)
	{
		::close(_socket);
	}
	_socket = INVALID_SOCKET;
	_socketState = UNKNOWN_SOCKET;
}

// Connects the socket to the given host through specified communication port
bool FtpSocket::ConnectClientSocket(const char* hostAddress, int port)
{
	if ((_socket != INVALID_SOCKET) || (hostAddress == NULL))
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
	if (_socket == INVALID_SOCKET)
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

	// Connected
	_socketState = CLIENT_SOCKET;
	return true;
}

// Connect a server socket and starts "listening" for incomming
// connection requests to a given communication port port 
bool FtpSocket::ConnectServerSocket(int port, int backLog)
{
	if (_socket != INVALID_SOCKET)
	{
		return false;
	}

	if (port == 0)
	{
		port = DEFAULT_FTP_PORT;
	}

	// Open a TCP socket (an Internet stream socket).
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket == INVALID_SOCKET)
	{
		ShowError("Cannot open a server TCP socket.");
		return false;
	}

	// Set the SO_REUSEADDR option
	if (!SetReuseAddress(true))
	{
		Close();
		ShowError("Cannot set SO_REUSEADDR option.");
		return false;
	}

	// Bind our local address so that the client can connect to us
	SOCKADDR_IN SockAddr;

	memset(&SockAddr, 0, sizeof(SOCKADDR));
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	SockAddr.sin_port = htons(port);
	if (bind(_socket, (const SOCKADDR*)&SockAddr, sizeof(SockAddr)) == SOCKET_ERROR)
	{
		Close();
		ShowError("Cannot bind the local address to the server socket.");
		return false;
	}

	// Start listening for incoming connection requests
	if ((listen(_socket, backLog) == SOCKET_ERROR) ||
		!CanAcceptConnection())
	{
		Close();
		ShowError("Failed to start listening for client requests.");
		return false;
	}

	// Listening
	_socketState = SERVER_SOCKET;
	return true;
}

// Accepts a new socket connection requested at the "listening" port 
FtpSocket* FtpSocket::Accept(void)
{
	FtpSocket* pSocket = NULL;

	if (IsServerSocket())
	{
		SOCKADDR_IN SockAddr;;
		socklen_t nSize = (socklen_t)sizeof(SockAddr);

		memset(&SockAddr, 0, sizeof(SOCKADDR_IN));

		SOCKET hSocket = accept(_socket, (SOCKADDR *)&SockAddr, &nSize);
		if (hSocket == INVALID_SOCKET)
		{
			ShowError("Failed to accept connection request.");
		}
		else
		{
			pSocket = new FtpSocket();
			pSocket->_socket = hSocket;
			pSocket->_socketState = SERVER_SOCKET;
		}
	}

	return (pSocket);
}

// Sends a message to partner socket
int FtpSocket::SendMessage(void* pDataBuffer, int dataSize)
{
	int noBytes = 0;        // number of bytes sent

	if (IsOperational() && (pDataBuffer != NULL) && (dataSize > 0))
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
	int noBytes = 0;       // number of bytes received

	if (IsOperational() && (pDataBuffer != NULL) && (bufferSize > 0))
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
	int noBytes = 0;       // total number of bytes received

	while (IsOperational() && (noBytes < bufferSize))
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

// Retrieves a given socket option 
int FtpSocket::GetOption(int option)
{
	int value = 0;

	if (HasSocket())
	{
		socklen_t nSize = (socklen_t)sizeof(value);

		if (getsockopt(_socket, SOL_SOCKET, option, (char *)&value, &nSize) == SOCKET_ERROR)
		{
			ShowError("Failed to retrieve socket option.");
			value = 0;
		}
	}

	return (value);
}

// Sets a given socket option 
bool FtpSocket::SetOption(int option, int value)
{
	if (!HasSocket())
	{
		return false;
	}
	else if (GetOption(option) != value)
	{
		int newValue = value;
		int nSize = sizeof(newValue);

		if (setsockopt(_socket, SOL_SOCKET, option, (char *)&newValue, nSize) == SOCKET_ERROR)
		{
			ShowError("Failed to set socket option.");
			return false;
		}
	}

	return true;
}
