/**
 * Daniel Sebastian Iliescu, http://dansil.net
 * MIT License (MIT), http://opensource.org/licenses/MIT
 *
 * This file contains the definition of a socket wrapper.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <stdio.h>

using SOCKET = int;
using SOCKADDR = struct sockaddr;
using SOCKADDR_IN = struct sockaddr_in;
using LPHOSTENT = struct hostent*;
using LPIN_ADDR = struct in_addr*;

static constexpr auto INVALID_SOCKET = -1;
static constexpr auto SOCKET_ERROR = -1;

// Default FTP port
static constexpr auto DEFAULT_FTP_PORT = 21;

// Max number of connection retries
static constexpr auto CONNECT_RETRIES = 10;

void show_error( std::string const & message );

class socket
{
public:
	socket() = default;
	virtual ~socket() noexcept;

	socket( socket const & ) = delete;
	socket( socket&& ) = delete;
	
	socket& operator=( socket const & ) = delete;
	socket& operator=( socket&& ) = delete;
	
	SOCKET get_socket_handle();

	void close();
	bool connect_client_socket(
		std::string const & host_address,
		int port = 0 );
	int send_message(
		void* buffer,
		int buffer_size );
	int receive_message(
		void* buffer,
		std::size_t buffer_size );
	int receive_message_all(
		void* buffer,
		std::size_t buffer_size );

private:
	SOCKET socket;
};
