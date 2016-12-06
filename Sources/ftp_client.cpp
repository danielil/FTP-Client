/**
 * Daniel Sebastian Iliescu, http://dansil.net
 * MIT License (MIT), http://opensource.org/licenses/MIT
 */

#include "ftp_processor.hpp"

#include <iostream>
#include <stdlib.h>
#include <string>

#ifdef __linux__
	#include <unistd.h>
#elif _WIN32
	#include <Windows.h>
	#include <lmcons.h>
#endif

bool
get_command(
	std::string& command,
	std::string& param1,
	std::string& param2)
{
	command.clear();
	param1.clear();
	param2.clear();

	std::string line;
	std::getline( std::cin, line );

	while ( !line.empty() )
	{
		// Remove starting white spaces
		auto command_position = line.find_first_not_of(" \t");

		if ( command_position != std::string::npos )
		{
			line = line.substr( command_position );
		}

		command_position = line.find_first_of(" \t");

		auto index = 0;
		switch (index++)
		{
		case 0:
			command = line.substr( 0, command_position );
			break;

		case 1:
			param1 = line.substr( 0, command_position );
			break;

		case 2:
			param2 = line.substr( 0, command_position );
			break;

		default:
			line.clear();
		}

		if ( command_position == std::string::npos)
		{
			line.clear();
		}
		else
		{
			line = line.substr( command_position + 1);
		}
	}

	return !command.empty();
}

int
main(
	int argc,
	char** argv )
{
	networking::ftp_processor ftp_processor;

	bool run = true;

	switch (argc)
	{
	case 0:
	case 1:
		break;

	case 2:
		ftp_processor.connect( argv[1] );
		break;

	case 3:
	default:
		ftp_processor.connect( argv[1], static_cast< std::uint16_t >( std::atoi( argv[2] ) ) );
		break;
	}

	std::string command;
	std::string param1;
	std::string param2;

	// Connect
	while ( run && !ftp_processor.is_connected() )
	{
		std::cout << "Please provide host info; expected format is:" << std::endl;
		std::cout << "\topen ip [port]" << std::endl;

		if ( !get_command( command, param1, param2 ) )
		{
			run = false;
		}
		else if ( ( command.compare("open") == 0 ) && !param1.empty() )
		{
			if ( param2.empty() )
			{
				ftp_processor.connect( param1 );
			}
			else
			{
				ftp_processor.connect( param1, static_cast< std::uint16_t >( std::atoi( param2.c_str() ) ) );
			}
		}
	}

	// Login
	while ( run )
	{
#ifdef __linux__
		std::string username( getlogin( ) );
#elif _WIN32
		char username[UNLEN + 1];
		DWORD username_length = UNLEN + 1;
		GetUserNameA( username, &username_length );
#endif
		std::cout << "Name (" << ftp_processor.get_host_address() << ":" << username << "): ";

		if ( !get_command( command, param1, param2 ) )
		{
			run = false;
		}
		else if ( ftp_processor.send_user_name( command ) )
		{
			std::cout << "Password: ";
			if ( !get_command( command, param1, param2 ) )
			{
				run = false;
			}
			else if ( ftp_processor.send_user_password( command ) )
			{
				ftp_processor.show_os();

				break;
			}
		}
	}

	// Execute commands
	while ( run )
	{
		std::cout << "ftp> ";

		if ( !get_command( command, param1, param2 ) )
		{
			continue;
		}

		bool success = false;

		if ( command.compare("cd") == 0 )
		{
			if ( !param1.empty() )
			{
				success = ftp_processor.set_directory( param1 );
			}
		}
		else if ( command.compare("ls") == 0 )
		{
			success = ftp_processor.list_directories();
		}
		else if ( command.compare("ldname") == 0 )
		{
			success = ftp_processor.list_directory_name();
		}
		else if ( command.compare("currentdir") == 0 )
		{
			success = ftp_processor.get_directory();
		}
		else if ( command.compare("cdup") == 0 )
		{
			success = ftp_processor.set_directory_to_parent();
		}
		else if ( command.compare("removedir") == 0 )
		{
			if ( !param1.empty() )
			{
				success = ftp_processor.remove_directory(param1);
			}
		}
		else if ( command.compare("makedir") == 0 )
		{
			if ( !param1.empty() )
			{
				success = ftp_processor.make_directory(param1);
			}
		}
		else if ( command.compare("reinitialize") == 0 )
		{
			success = ftp_processor.reinitialize();
		}
		else if ( command.compare("status") == 0 )
		{
			success = ftp_processor.status();
		}
		else if ( command.compare("del") == 0 )
		{
			if ( !param1.empty() )
			{
				success = ftp_processor.delete_file(param1);
			}
		}
		else if ( command.compare("sys") == 0 )
		{
			success = ftp_processor.show_os();
		}
		else if ( command.compare("get") == 0 )
		{
			if ( !param1.empty() )
			{
				success = ftp_processor.get_file(param1);
			}
		}
		else if ( command.compare("put") == 0 )
		{
			if ( !param1.empty() )
			{
				success = ftp_processor.put_file(param1);
			}
		}
		else if ( command.compare("type") == 0 )
		{
			if ( !param1.empty() )
			{
				if ( param1 == "binary" )
				{
					success = ftp_processor.set_transfer_type( false );
				}
				else if ( param1 == "ascii" )
				{
					success = ftp_processor.set_transfer_type( true );
				}
			}
		}
		else if ( command.compare("close") == 0 )
		{
			ftp_processor.terminate();
			success = true;
		}
		else if ( command.compare("quit") == 0 )
		{
			ftp_processor.terminate();
			run = false;
			success = true;
		}

		if ( !success )
		{
			std::cout << command << " " << param1;
			std::cout << " failed" << std::endl;
		}
	}
	
	return 0;
}
