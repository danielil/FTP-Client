/**
 * Daniel Sebastian Iliescu, http://dansil.net
 * MIT License (MIT), http://opensource.org/licenses/MIT
 */

#include "ftp_processor.hpp"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

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
		std::string const & host_address,
		std::uint16_t port )
	{
		if ( !this->is_connected() &&
			 !this->is_data_connected() &&
			 this->host_address.empty() )
		{
			this->command_socket.close();

			if ( this->command_socket.connect_client_socket( host_address, port ) )
			{
				// Expected connection reply
				static constexpr auto CONNECTED_OK = 220;

				if ( this->receive_reply() && std::atoi( message.data() ) == CONNECTED_OK )
				{
					// Memorize the host address for subsequent data connection
					this->host_address = host_address;

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
	void ftp_processor::disconnect( bool full )
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
	bool ftp_processor::ftp_command(
		std::string const & command,
		std::string const & param )
	{
		if ( this->is_connected() )
		{
			std::fill( std::begin( this->message ), std::end( this->message ), 0 );
			if ( param.empty() )
			{
				sprintf( this->message.data(), "%s %s\r\n", command, param );
			}
			else
			{
				sprintf( this->message.data(), "%s\r\n", command );
			}

			return this->send_command();
		}

		return false;
	}

	// Terminates all connections and optionally, quits FTP. 
	void ftp_processor::terminate()
	{
		if ( this->is_connected() )
		{
			this->ftp_command( "QUIT", "" );
			this->disconnect( true );
		}
	}

	// Sets the type of the transfer to Binary or ASCII (TYPE command) 
	bool ftp_processor::set_transfer_type( bool type )
	{
		if ( this->is_connected() )
		{
			if ( this->ftp_command( "TYPE", ( type ? "A" : "I" ) ) )
			{
				this->transfer_type = type;

				return true;
			}
		}

		return false;
	}

	// Sends the user name to the FTP server (USER command)
	bool ftp_processor::send_user_name( std::string const & name )
	{
		if ( this->is_connected() )
		{
			// Expected USER command reply
			static constexpr auto USERNAME_OK = 331;

			return this->ftp_command( "USER", name ) && ( atoi( this->message.data() ) == USERNAME_OK );
		}

		return false;
	}

	// Sends the user password to the FTP server (PASS command)
	bool ftp_processor::send_user_password( std::string const & password )
	{
		if ( this->is_connected() )
		{
			if ( this->ftp_command( "PASS", password ) )
			{
				return this->receive_reply();
			}
		}

		return false;
	}

	// Displays the operating system (SYST command)
	bool ftp_processor::show_os()
	{
		if ( this->is_connected() )
		{
			return this->ftp_command( "SYST", "" );
		}

		return false;
	}

	// Retrieves the content of the present working directory (LIST command) 
	bool ftp_processor::list_directories( void )
	{
		bool success = false;

		if ( this->is_connected() )
		{
			if ( this->start_data_connection( "LIST", nullptr ) )
			{
				success = true;

				while ( success )
				{
					// Retrieves the directory content and displays it on the console
					std::fill( std::begin( this->message ), std::end( this->message ), 0 );

					int numBytes = this->data_socket.receive_message( static_cast<void *>(this->message.data()), sizeof( this->message.data() ) - 1 );
					if ( numBytes == 0 )
					{
						break;
					}

					// Display the data retrieved
					std::cout << this->message.data();
				}
				std::cout << std::endl;

				this->stop_data_connection( false );
			}
		}

		return success;
	}

	// Retrieves only the names of the present working directory (NLST command) 
	bool ftp_processor::list_directory_name()
	{
		bool success = false;

		if ( this->is_connected() )
		{
			if ( this->start_data_connection( "NLST", "" ) )
			{
				success = true;

				while ( success )
				{
					// Retrieves the directory content and displays it on the console
					std::fill( std::begin( this->message ), std::end( this->message ), 0 );

					const auto numBytes = this->data_socket.receive_message( static_cast< void* >( this->message.data() ), sizeof( this->message.data() ) - 1 );
					
					if ( numBytes == 0 )
					{
						break;
					}

					// Display the data retrieved
					std::cout << this->message.data();
				}

				std::cout << std::endl;

				this->stop_data_connection( false );
			}
		}

		return success;
	}

	// Retrieves the present working directory (PWD command) 
	bool ftp_processor::get_directory()
	{
		if ( this->is_connected() )
		{
			return this->ftp_command( "PWD", "" );
		}

		return false;
	}

	// Changes the current directory on the the FTP server (CWD command) 
	bool ftp_processor::set_directory( std::string const & directory )
	{
		if ( this->is_connected() )
		{
			return this->ftp_command( "CWD", directory );
		}

		return false;
	}

	// Changes the current directory to the parent directory (CDUP command)
	bool ftp_processor::set_directory_to_parent()
	{
		if ( this->is_connected() )
		{
			return this->ftp_command( "CDUP", "" );
		}

		return false;
	}

	// Removes the selected directory on the FTP server (RMD command)
	// This functions doesn’t support recursive deletion (if the folder has files in it),
	// and will fail if the folder is not empty.
	bool ftp_processor::remove_directory( std::string const & directory )
	{
		if ( this->is_connected() )
		{
			return this->ftp_command( "RMD", directory );
		}

		return false;
	}

	// Makes a directory on the server (MKD command)
	bool ftp_processor::make_directory( std::string const & directory )
	{
		if ( this->is_connected() )
		{
			return this->ftp_command( "MKD", directory );
		}

		return false;
	}

	// Terminate the USER session and purge all account information (REIN command)
	bool ftp_processor::reinitialize()
	{
		if ( this->is_connected() )
		{
			return this->ftp_command( "REIN", "" );
		}

		return false;
	}

	// Returns server status (STAT command)
	bool ftp_processor::status()
	{
		if ( this->is_connected() )
		{
			return this->ftp_command( "STAT", "" );
		}

		return false;
	}

	// Delete a file from the FTP server (DELE command)
	bool ftp_processor::delete_file( std::string const & fileName )
	{
		if ( this->is_connected() )
		{
			return this->ftp_command( "DELE", fileName );
		}

		return false;
	}

	// Downloads a file from the FTP server (RETR command)
	bool ftp_processor::get_file( std::string const & fileName )
	{
		bool success = false;

		if ( this->is_connected() )
		{
			FILE* fp = std::fopen( fileName.c_str(), ( this->transfer_type ? "wt" : "wb" ) );

			if ( fp != nullptr )
			{
				set_transfer_type( this->transfer_type );
				if ( this->start_data_connection( "RETR", fileName ) )
				{
					success = true;
					while ( success )
					{
						// Get data from server and writes to the local file
						memset( this->message.data(), 0, sizeof( this->message.data() ) );
						int numBytes = this->data_socket.receive_message( static_cast<void *>(this->message.data()), sizeof( this->message.data() ) );
						if ( numBytes == 0 )
						{
							break;
						}
						fwrite( static_cast<const void*>(this->message.data()), sizeof( char ), numBytes, fp );
					}
					this->stop_data_connection( false );
				}
				fclose( fp );
			}
		}

		return success;
	}

	// Uploads a file to the FTP server (STOR command)
	bool ftp_processor::put_file( std::string const & fileName )
	{
		bool success = false;

		if ( this->is_connected() )
		{
			FILE* fp = fopen( fileName.c_str(), ( this->transfer_type ? "rt" : "rb" ) );
			if ( fp != nullptr )
			{
				set_transfer_type( this->transfer_type );
				if ( this->start_data_connection( "STOR", fileName ) )
				{
					success = true;
					while ( success && ( feof( fp ) == 0 ) )
					{
						// Reads file content and sends it to the server
						std::fill( std::begin( this->message ), std::end( this->message ), 0 );

						int numBytes = fread( static_cast<void *>(this->message.data()), sizeof( char ), sizeof( this->message.data() ), fp );
						if ( numBytes == 0 )
						{
							success = false;
							break;
						}
						if ( this->data_socket.send_message( static_cast<void *>(this->message.data()), numBytes ) == 0 )
						{
							success = false;
							break;
						}
					}
					this->stop_data_connection( false );
				}
				fclose( fp );
			}
		}

		return success;
	}

	// Initialization method
	void ftp_processor::init()
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
	bool ftp_processor::send_pasv()
	{
		if ( this->ftp_command( "PASV", "" ) )
		{
			std::string msg = this->message.data();

			// Wait for Passive mode indicator
			const std::string passive_mode_flag = "Entering Passive Mode";
			if ( msg.find( passive_mode_flag ) == std::string::npos )
			{
				if ( !this->receive_reply() )
				{
					return false;
				}
			}

			// Parse the response to retrieve the data port
			// The expected response includes the following sequence of numbers:
			//      A1, A2, A3, A4, P1, P2 
			// where A1.A2.A3.A4 is the IP address of the host, while
			// P1 and P2 should be used to calculate the port number:
			//      Data port = P1 * 256 + P2
			std::uint16_t nPort = 0;

			for ( int nPos = 1, i = strlen( this->message.data() ) - 1; i > 0; i-- )
			{
				if ( isdigit( static_cast<int>(this->message.data()[i]) ) )
				{
					nPort += static_cast<unsigned int>(nPos * (this->message.data()[i] - '0'));
					nPos *= 10;
				}
				else if ( nPos > 1 )
				{
					for ( nPos = 256; i > 0; i-- )
					{
						if ( isdigit( static_cast<int>(this->message.data()[i]) ) )
						{
							nPort += static_cast<unsigned int>(nPos * (this->message.data()[i] - '0'));
							nPos *= 10;
						}
						else if ( nPos > 256 )
						{
							this->data_port = nPort;
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
		std::string const & dataCommand,
		std::string const & dataParam )
	{
		if ( this->is_connected() && ( this->is_data_connected() ) && this->send_pasv() )
		{
			this->data_socket.close();

			if ( !this->data_socket.connect_client_socket( this->host_address.c_str(), this->data_port ) )
			{
				return false;
			}

			if ( this->ftp_command( dataCommand, dataParam ) )
			{
				return true;
			}

			this->stop_data_connection( true );
		}

		return ( false );
	}

	// this->disconnects the socket used for data transfer.
	// If "abort" flag is not set, the we wait for a server reply. 
	void ftp_processor::stop_data_connection( bool abort )
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
	bool ftp_processor::send_command()
	{
		if ( this->is_connected() )
		{
			if ( command_socket.send_message( static_cast<void *>(this->message.data()), strlen( this->message.data() ) ) )
			{
				return this->receive_reply();
			}
		}

		return false;
	}

	// Receives the reply message on a command request sent to the FTP server 
	bool ftp_processor::receive_reply()
	{
		bool success = false;

		if ( this->is_connected() )
		{
			memset( this->message.data(), 0, sizeof( this->message.data() ) );
			if ( command_socket.receive_message( static_cast<void *>(this->message.data()), sizeof( this->message.data() ) ) )
			{
				// Check reply
				int nPos = 0;
				int maxPos = static_cast<int>(sizeof(this->message.data())) - 5;

				std::cout << this->message.data();
				while ( !success && ( nPos < maxPos ) &&
					( ( this->message.data()[nPos + 0] == '-' ) ||
					( ( this->message.data()[nPos + 1] == ' ' ) &&
						( this->message.data()[nPos + 2] == ' ' ) &&
						( this->message.data()[nPos + 3] == ' ' ) ) ) )
				{
					// skip continuation lines (denoted by dash or spaces)
					int nLen = maxPos - nPos;

					for ( int i = 0; i < nLen; i++ )
					{
						if ( this->message.data()[nPos + i] == 0x00 )
						{
							success = true;
							break;
						}
						else if ( this->message.data()[nPos + i] == 0x0A )
						{
							nPos = i + 1;
						}
					}
				}

				if ( !success && ( nPos < maxPos ) )
				{
					// FTP Error Threshold
					static constexpr auto ERROR_THRESHOLD = 400;

					if ( atoi( static_cast<const char*>(&this->message.data()[nPos]) ) < ERROR_THRESHOLD )
					{
						success = true;
					}
				}
			}
		}

		return success;
	}

	std::string ftp_processor::get_host_address() const noexcept
	{
		return this->host_address;
	}
}