/**
 * Daniel Sebastian Iliescu, http://dansil.net
 * MIT License (MIT), http://opensource.org/licenses/MIT
 */

#include "ftp_processor.hpp"

#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <fstream>

namespace networking
{
	// Destructor
	ftp_processor::~ftp_processor() noexcept
	{
		this->disconnect( true );
	}

	// Checks whether the command socket is connected to the FTP server
	bool
	ftp_processor::is_connected() const noexcept
	{
		return ( this->command_socket.is_connected() );
	}

	// Checks whether the data socket is connected to the FTP server 
	bool
	ftp_processor::is_data_connected() const noexcept
	{
		return ( this->data_socket.is_connected() );
	}

	// Opens an FTP session by connecting the command socket to the FTP server
	bool
	ftp_processor::connect(
		std::string const & host,
		std::uint16_t port )
	{
		if ( !this->is_connected() &&
			 !this->is_data_connected() &&
			 this->host_address.empty() )
		{
			this->command_socket.close();

			if ( this->command_socket.connect_client_socket( host, port ) )
			{
				// Expected connection reply
				static constexpr auto CONNECTED_OK = 220;

				if ( this->receive_reply() && std::atoi( message.data() ) == CONNECTED_OK )
				{
					// Memorize the host address for subsequent data connection
					this->host_address = host;

					return true;
				}
			}

			this->disconnect( true );
		}

		return ( false );
	}

	// this->disconnects and destroys the command and the data sockets.
	// If full == true, then it this->disconnects both sockets;
	// Otherwise, it this->disconnects only the data socket.
	void
	ftp_processor::disconnect( bool full )
	{
		this->stop_data_connection( true );

		if ( full )
		{
			this->command_socket.close();
			this->init();
		}
	}

	// Sends an FTP command with or without parameters to the FTP server.
	// It checks the response and returns true if successful.
	bool
	ftp_processor::ftp_command(
		const std::string command,
		const std::string parameter )
	{
		if ( this->is_connected() )
		{
			std::fill( std::begin( this->message ), std::end( this->message ), 0 );

			std::stringstream formatter;
			formatter << command;

			if ( !parameter.empty() )
			{
				formatter << " " << parameter;
			}

			formatter << std::endl;

			const auto formatted_message = formatter.str();

			std::copy(
				std::begin( formatted_message ),
				std::end( formatted_message ),
				std::begin( this->message ) );

			return this->send_command();
		}

		return false;
	}

	// Terminates all connections and optionally, quits FTP. 
	void
	ftp_processor::terminate()
	{
		this->ftp_command( "QUIT", "" );
		this->disconnect( true );
	}

	// Sets the type of the transfer to Binary or ASCII (TYPE command) 
	bool
	ftp_processor::set_transfer_type( bool type )
	{
		if ( this->ftp_command( "TYPE", ( type ? "A" : "I" ) ) )
		{
			this->transfer_type = type;

			return true;
		}

		return false;
	}

	// Sends the user name to the FTP server (USER command)
	bool
	ftp_processor::send_user_name( std::string const & name )
	{
		// Expected USER command reply
		static constexpr auto USERNAME_OK = 331;

		return this->ftp_command( "USER", name ) && ( atoi( this->message.data() ) == USERNAME_OK );
	}

	// Sends the user password to the FTP server (PASS command)
	bool
	ftp_processor::send_user_password( std::string const & password )
	{
		return this->ftp_command( "PASS", password ) && this->receive_reply();
	}

	// Displays the operating system (SYST command)
	bool
	ftp_processor::show_os()
	{
		return this->ftp_command( "SYST", "" );
	}

	// Retrieves the content of the present working directory (LIST command) 
	bool
	ftp_processor::list_directories()
	{
		if ( this->start_data_connection( "LIST", nullptr ) )
		{
			std::fill( std::begin( this->message ), std::end( this->message ), 0 );

			while ( this->data_socket.receive_message( static_cast<void *>( this->message.data() ), std::strlen( this->message.data() ) - 1 ) != 0 )
			{
				// Retrieves the directory content and displays it on the console
				std::fill( std::begin( this->message ), std::end( this->message ), 0 );

				// Display the data retrieved
				std::cout << this->message.data();
			}

			std::cout << std::endl;

			this->stop_data_connection( false );

			return true;
		}

		return false;
	}

	// Retrieves only the names of the present working directory (NLST command) 
	bool
	ftp_processor::list_directory_name()
	{
		if ( this->start_data_connection( "NLST", "" ) )
		{
			std::fill( std::begin( this->message ), std::end( this->message ), 0 );

			while ( this->data_socket.receive_message( static_cast< void* >( this->message.data() ), std::strlen( this->message.data() ) - 1 ) != 0 )
			{
				// Retrieves the directory content and displays it on the console
				std::fill( std::begin( this->message ), std::end( this->message ), 0 );

				// Display the data retrieved
				std::cout << this->message.data();
			}

			std::cout << std::endl;

			this->stop_data_connection( false );

			return true;
		}

		return false;
	}

	// Retrieves the present working directory (PWD command) 
	bool
	ftp_processor::get_directory()
	{
		return this->ftp_command( "PWD", "" );
	}

	// Changes the current directory on the the FTP server (CWD command) 
	bool
	ftp_processor::set_directory( std::string const & directory )
	{
		return this->ftp_command( "CWD", directory );
	}

	// Changes the current directory to the parent directory (CDUP command)
	bool
	ftp_processor::set_directory_to_parent()
	{
		return this->ftp_command( "CDUP", "" );
	}

	// Removes the selected directory on the FTP server (RMD command)
	// This functions doesn’t support recursive deletion (if the folder has files in it),
	// and will fail if the folder is not empty.
	bool
	ftp_processor::remove_directory( std::string const & directory )
	{
		return this->ftp_command( "RMD", directory );
	}

	// Makes a directory on the server (MKD command)
	bool
	ftp_processor::make_directory( std::string const & directory )
	{
		return this->ftp_command( "MKD", directory );
	}

	// Terminate the USER session and purge all account information (REIN command)
	bool
	ftp_processor::reinitialize()
	{
		return this->ftp_command( "REIN", "" );
	}

	// Returns server status (STAT command)
	bool
	ftp_processor::status()
	{
		return this->ftp_command( "STAT", "" );
	}

	// Delete a file from the FTP server (DELE command)
	bool
	ftp_processor::delete_file( std::string const & filename )
	{
		return this->ftp_command( "DELE", filename );
	}

	// Downloads a file from the FTP server (RETR command)
	bool
	ftp_processor::get_file( std::string const & filename )
	{
		if ( this->is_connected() )
		{
			std::ios_base::open_mode mode = std::ios_base::out;
			
			if ( this->transfer_type )
			{
				mode |= std::ios_base::binary;
			}

			std::ofstream output( filename, mode );

			if ( output.is_open() )
			{
				this->set_transfer_type( this->transfer_type );

				if ( this->start_data_connection( "RETR", filename ) )
				{
					while ( true )
					{
						std::fill( std::begin( this->message ), std::end( this->message ), 0 );

						auto bytes = this->data_socket.receive_message( static_cast< void* >( this->message.data() ), std::strlen( this->message.data() ) );

						if ( bytes == 0 )
						{
							break;
						}

						// Get data from server and writes to the local file
						output << this->message.data();
					}

					this->stop_data_connection( false );

					return true;
				}
			}
		}

		return false;
	}

	// Uploads a file to the FTP server (STOR command)
	bool
	ftp_processor::put_file( std::string const & filename )
	{
		if ( this->is_connected() )
		{
			std::ios_base::open_mode mode = std::ios_base::in;

			if ( this->transfer_type )
			{
				mode |= std::ios_base::binary;
			}

			std::ifstream input( filename, mode );

			if ( input.is_open() )
			{
				this->set_transfer_type( this->transfer_type );

				if ( this->start_data_connection( "STOR", filename ) )
				{
					while ( !input.eof() )
					{
						// Reads file content and sends it to the server
						std::fill( std::begin( this->message ), std::end( this->message ), 0 );

						input >> this->message.data();

						if ( this->data_socket.send_message( static_cast< void* >( this->message.data() ), std::strlen( this->message.data() ) ) == 0 )
						{
							return false;
						}
					}

					this->stop_data_connection( false );

					return true;
				}
			}
		}

		return false;
	}

	// Initialization method
	void
	ftp_processor::init()
	{
		std::fill( std::begin( this->message ), std::end( this->message ), 0 );
		this->transfer_type = false;
		this->host_address.clear();
		this->data_port = 0;
	}

	// Sends the PASV request to the server, forcing it to start listening. 
	// The response should include the host address and the data port;
	// since we saved the host address at connection time, we only retrieve
	// the data port.
	bool
	ftp_processor::send_pasv()
	{
		if ( this->ftp_command( "PASV", "" ) )
		{
			const std::string msg = this->message.data();

			// Wait for passive mode indicator
			const std::string passive_mode_flag = "Entering Passive Mode";

			if ( msg.find( passive_mode_flag ) == std::string::npos )
			{
				if ( !this->receive_reply() )
				{
					return false;
				}
			}

			// Parse the response to retrieve the data port.
			// The expected response includes the following sequence of numbers:
			//		A1, A2, A3, A4, P1, P2 
			// where A1.A2.A3.A4 is the IP address of the host, while
			// P1 and P2 should be used to calculate the port number:
			//		Data port = P1 * 256 + P2
			std::uint16_t port = 0;

			auto position = 1;
			for ( std::size_t idx = std::strlen( this->message.data() ) - 1; idx > 0; --idx )
			{
				if ( ::isdigit( static_cast< int >( this->message.data()[idx] ) ) )
				{
					port += static_cast< std::uint16_t >( position * ( this->message.data()[idx] - '0' ) );
					position *= 10;
				} 
				else if ( position > 1 )
				{
					for ( position = 256; idx > 0; --idx )
					{
						if ( ::isdigit( static_cast< int >( this->message.data()[idx] ) ) )
						{
							port += static_cast< std::uint16_t >( position * ( this->message.data()[idx] - '0' ) );
							position *= 10;
						}
						else if ( position > 256 )
						{
							this->data_port = port;

							return true;
						}
					}
					break;
				}
			}
		}

		return false;
	}

	// Sets up the socket connection for data transfer. 
	bool ftp_processor::start_data_connection(
		std::string const & command,
		std::string const & parameter )
	{
		if ( this->is_connected() && this->is_data_connected() && this->send_pasv() )
		{
			this->data_socket.close();

			if ( !this->data_socket.connect_client_socket( this->host_address.c_str(), this->data_port ) )
			{
				return false;
			}

			if ( this->ftp_command( command, parameter ) )
			{
				return true;
			}

			this->stop_data_connection( true );
		}

		return false;
	}

	// this->disconnects the socket used for data transfer.
	// If "abort" flag is not set, the we wait for a server reply. 
	void
	ftp_processor::stop_data_connection( bool abort )
	{
		if ( this->data_socket.is_connected() )
		{
			this->data_socket.close();
		}

		if ( !abort )
		{
			this->receive_reply();
		}
	}

	// Sends a command message to the FTP server and retrieves the reply.
	// If the reply includes an TCP/IP transfer code < 400, then we consider
	// that the command transmission was successful.
	bool
	ftp_processor::send_command()
	{
		if ( this->is_connected() )
		{
			if ( this->command_socket.send_message( static_cast< void* >( this->message.data() ), strlen( this->message.data() ) ) )
			{
				return this->receive_reply();
			}
		}

		return false;
	}

	// Receives the reply message on a command request sent to the FTP server 
	bool
	ftp_processor::receive_reply()
	{
		if ( this->is_connected() )
		{
			std::fill( std::begin( this->message ), std::end( this->message ), 0 );

			if ( this->command_socket.receive_message( static_cast< void* >( this->message.data() ), std::strlen( this->message.data() ) ) )
			{
				// Check reply
				auto position = 0;
				const auto maximum_position = static_cast< int >( std::strlen( this->message.data() ) ) - 5;

				std::cout << this->message.data();
				while ( ( position < maximum_position ) &&
						( this->message.data()[position + 0] == '-' ) ||
						( this->message.data()[position + 1] == ' ' ) &&
						( this->message.data()[position + 2] == ' ' ) &&
						( this->message.data()[position + 3] == ' ' ) )
				{
					// Skip continuation lines (denoted by dash or spaces)
					const auto length = maximum_position - position;

					for ( int idx = 0; idx < length; ++idx )
					{
						if ( this->message.data()[position + idx] == 0x00 )
						{
							return true;
						}
						else if ( this->message.data()[position + idx] == 0x0A )
						{
							position = idx + 1;
						}
					}
				}

				if ( position < maximum_position )
				{
					// FTP Error Threshold
					static constexpr auto error_threshold = 400;

					return std::atoi( static_cast< char const * >( &this->message.data()[position] ) ) < error_threshold;
				}
			}
		}

		return false;
	}

	std::string
	ftp_processor::get_host_address() const noexcept
	{
		return this->host_address;
	}
}