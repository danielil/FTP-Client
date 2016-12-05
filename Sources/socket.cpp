/**
 * Daniel Sebastian Iliescu, http://dansil.net
 * MIT License (MIT), http://opensource.org/licenses/MIT
 */

#include "socket.hpp"

#ifdef __linux__
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <netinet/tcp.h>

	using SOCKADDR = struct sockaddr;
	using SOCKADDR_IN = struct sockaddr_in;
	using LPHOSTENT = struct hostent*;
	using LPIN_ADDR = struct in_addr*;

	static constexpr auto SOCKET_ERROR = -1;
#elif _WIN32
	#include <WinSock2.h>
#endif

#include <iostream>

namespace networking
{
	socket::~socket() noexcept
	{
		this->close();
	}

	bool
	socket::is_connected() const noexcept
	{
		return ( this->socket_handle != INVALID_SOCKET );
	}

	// Connects the socket to the given host through specified communication port
	bool
	socket::connect_client_socket(
		std::string const & host_address,
		std::uint16_t port ) noexcept
	{
		if ( port == 0 )
		{
			static constexpr auto default_ftp_port = 21;

			port = default_ftp_port;
		}

		// Fill in the data needed for connecting to the server.
		SOCKADDR_IN sock_addr = {};

		sock_addr.sin_family = AF_INET;
		sock_addr.sin_addr.s_addr = ::inet_addr( host_address.c_str() );
		sock_addr.sin_port = ::htons( port );

		if ( sock_addr.sin_addr.s_addr == INADDR_NONE )
		{
			LPHOSTENT host = ::gethostbyname( host_address.c_str() );

			if ( host == nullptr )
			{
				std::cerr << "Cannot find hostname.";
				return false;
			}

			const auto internet_address = reinterpret_cast< LPIN_ADDR >( *host->h_addr_list );
			sock_addr.sin_addr.s_addr = ::inet_addr( ::inet_ntoa( *internet_address ) );
		}

		// Open a TCP socket (an Internet stream socket)
		this->socket_handle = ::socket( AF_INET, SOCK_STREAM, 0 );

		if ( this->socket_handle == INVALID_SOCKET )
		{
			std::cerr << "Cannot open a client TCP socket.";
			return false;
		}

		// Connect to the server
		auto pass = 0;
		while ( ::connect( this->socket_handle, reinterpret_cast< const SOCKADDR* >( &sock_addr ), sizeof( sock_addr ) ) == SOCKET_ERROR )
		{
			// Max number of connection retries
			static constexpr auto connection_retries = 10;

			if ( ++pass >= connection_retries )
			{
				this->close();

				std::cerr << "Cannot connect client TCP socket.";
				return false;
			}
		}

		return true;
	}

	void
	socket::close() noexcept
	{
	#ifdef __linux__ 
		close( this->socket );
	#elif _WIN32
		::closesocket( this->socket_handle );
	#endif

		this->socket_handle = INVALID_SOCKET;
	}

	// Sends a message to partner socket
	int
	socket::send_message(
		void* buffer,
		std::size_t buffer_size ) const noexcept
	{
		auto bytes_sent = 0;

		if ( buffer != nullptr )
		{
			bytes_sent = ::send( this->socket_handle, static_cast< char* >( buffer ), buffer_size, 0 );

			if ( bytes_sent == SOCKET_ERROR )
			{
				std::cerr << "Failed to send data.";
				bytes_sent = 0;
			}
		}

		return bytes_sent;
	}

	// Receives a message from partner socket 
	int
	socket::receive_message(
		void* buffer,
		std::size_t buffer_size ) const noexcept
	{
		auto bytes_received = 0;

		if ( buffer != nullptr )
		{
			bytes_received = ::recv( this->socket_handle, static_cast< char* >( buffer ), buffer_size, 0 );

			if ( bytes_received == SOCKET_ERROR )
			{
				std::cerr << "Failed to receive data.";
				bytes_received = 0;
			}
		}

		return bytes_received;
	}

	// Receives all pending messages from partner socket 
	int
	socket::receive_message_all(
		void* buffer,
		std::size_t buffer_size ) const noexcept
	{
		auto bytes_received = 0;

		while ( bytes_received < buffer_size )
		{
			auto* offsetted_buffer = static_cast< unsigned char* >( buffer ) + bytes_received;

			const auto max_size = buffer_size - bytes_received;
			const auto size_received = this->receive_message( static_cast< void* >( offsetted_buffer ), max_size );

			if ( size_received > 0)
			{
				bytes_received += size_received;
			}
			else
			{
				break;
			}
		}

		return bytes_received;
	}
}