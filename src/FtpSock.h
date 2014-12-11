/**
* @author Daniel Sebastian Iliescu
* @file FtpSock.h
* @date 2014-12-12

* This file contains the definition of the FTP socket wrapper class
*/

extern "C"
{
	#include <sys/types.h>    // socket, bind
	#include <sys/socket.h>   // socket, bind, listen, inet_ntoa
	#include <netinet/in.h>   // htonl, htons, inet_ntoa
	#include <arpa/inet.h>    // inet_ntoa
	#include <netdb.h>        // gethostbyname
	#include <unistd.h>       // read, write, close
	#include <string.h>       // bzero
	#include <netinet/tcp.h>  // TCP_NODELAY
	#include <stdlib.h>
	#include <stdio.h>
}

typedef int SOCKET;
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct hostent* LPHOSTENT;
typedef struct in_addr* LPIN_ADDR;

#define INVALID_SOCKET  (SOCKET)(-1)
#define SOCKET_ERROR    (-1)

const int DEFAULT_FTP_PORT = 21;    // Default FTP port
const int DEFAULT_BACKLOG = 5;      // Max. listening backlog
const int CONNECT_RETRIES = 10;     // max. no. of connect retries

// Socket states
const int UNKNOWN_SOCKET = 0;
const int CLIENT_SOCKET = 1;    // Client socket connected to server
const int SERVER_SOCKET = 2;    // Server socket listening for connection requests

bool LoadSocketLib(void);
void UnloadSocketLib(void);
void ShowError(const char* msg);

class FtpSocket
{
public:
	FtpSocket(void);
	virtual ~FtpSocket(void);

	SOCKET GetSocketHandle(void)    { return (_socket); }
	bool HasSocket()                { return (_socket != INVALID_SOCKET); }
	bool IsClientSocket(void)       { return HasSocket() && (_socketState == CLIENT_SOCKET); }
	bool IsServerSocket(void)       { return HasSocket() && (_socketState == SERVER_SOCKET); }
	bool IsOperational()            { return HasSocket() && (_socketState != UNKNOWN_SOCKET); }

	// Operations
	void Close(void);
	bool ConnectClientSocket(const char* hostAddress, int port = 0);
	bool ConnectServerSocket(int port = DEFAULT_FTP_PORT, int backLog = DEFAULT_BACKLOG);
	FtpSocket* Accept(void);
	int SendMessage(void* pDataBuffer, int nDataSize);
	int ReceiveMessage(void* pDataBuffer, int nBufferSize);
	int ReceiveMessageAll(void* pDataBuffer, int nBufferSize);

	// Socket options
	bool CanAcceptConnection(void)  { return (GetOption(SO_ACCEPTCONN) == 0); }
	bool GetBroadcast(void)         { return (GetOption(SO_BROADCAST) == 0); }
	bool GetKeepAlive(void)         { return (GetOption(SO_KEEPALIVE) == 0); }
	bool GetReuseAddress(void)      { return (GetOption(SO_REUSEADDR) == 0); }
	int GetRecvSize(void)           { return (GetOption(SO_RCVBUF)); }
	int GetSendSize(void)           { return (GetOption(SO_SNDBUF)); }
	bool SetBroadcast(bool bSet)    { return SetOption(SO_BROADCAST, (int)bSet); }
	bool SetKeepAlive(bool bSet)    { return SetOption(SO_KEEPALIVE, (int)bSet); }
	bool SetReuseAddress(bool bSet) { return SetOption(SO_REUSEADDR, (int)bSet); }
	bool SetRecvSize(int nSize)     { return SetOption(SO_RCVBUF, nSize); }
	bool SetSendSize(int nSize)     { return SetOption(SO_SNDBUF, nSize); }

private:
	FtpSocket(const FtpSocket&);
	FtpSocket& operator =(const FtpSocket&);

	int GetOption(int option);
	bool SetOption(int option, int value);

	SOCKET _socket;         // socket handle
	int _socketState;       // socket state
};
