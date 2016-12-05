/**
 * Daniel Sebastian Iliescu, http://dansil.net
 * MIT License (MIT), http://opensource.org/licenses/MIT
 *
 * This file contains the definition of the FTP socket wrapper classic
 */

extern "C"
{
	#include <sys/types.h>		// socket, bind
	#include <sys/socket.h>		// socket, bind, listen, inet_ntoa
	#include <netinet/in.h>		// htonl, htons, inet_ntoa
	#include <arpa/inet.h>		// inet_ntoa
	#include <netdb.h>			// gethostbyname
	#include <unistd.h>			// read, write, close
	#include <string.h>			// bzero
	#include <netinet/tcp.h>	// TCP_NODELAY
	#include <stdlib.h>
	#include <stdio.h>
}

typedef int SOCKET;
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct hostent* LPHOSTENT;
typedef struct in_addr* LPIN_ADDR;

#define INVALID_SOCKET (SOCKET)(-1)
#define SOCKET_ERROR (-1)

const int DEFAULT_FTP_PORT = 21;	// Default FTP port
const int CONNECT_RETRIES = 10;		// max. no. of connect retries

void ShowError(const char* msg);

class FtpSocket
{
public:
	FtpSocket(void);
	virtual ~FtpSocket(void);

	SOCKET GetSocketHandle(void) { return (_socket); }

	// Operations
	void Close(void);
	bool ConnectClientSocket(const char* hostAddress, int port = 0);
	int SendMessage(void* pDataBuffer, int nDataSize);
	int ReceiveMessage(void* pDataBuffer, int nBufferSize);
	int ReceiveMessageAll(void* pDataBuffer, int nBufferSize);

private:
	FtpSocket(const FtpSocket&);
	FtpSocket& operator =(const FtpSocket&);

	SOCKET _socket; // socket handle
};