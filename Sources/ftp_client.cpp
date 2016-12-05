/**
 * Daniel Sebastian Iliescu, http://dansil.net
 * MIT License (MIT), http://opensource.org/licenses/MIT
 */

#include "ftp_processor.hpp"

#include <iostream>
#include <stdlib.h>
#include <string>

#ifdef _WIN32
	#include <Windows.h>
	#include <lmcons.h>
#endif

bool get_command(std::string& command, std::string& param1, std::string& param2)
{
	std::string line;
	int index = 0;

	command.clear();
	param1.clear();
	param2.clear();

	getline(std::cin, line);

	while (!line.empty())
	{
		// Remove starting white spaces
		size_t cmdPos = line.find_first_not_of(" \t");
		if (cmdPos != std::string::npos)
		{
			line = line.substr(cmdPos);
		}

		cmdPos = line.find_first_of(" \t");
		switch (index++)
		{
		case 0:
			command = line.substr(0, cmdPos);
			break;

		case 1:
			param1 = line.substr(0, cmdPos);
			break;

		case 2:
			param2 = line.substr(0, cmdPos);
			break;

		default:
			line.clear();
		}

		if (cmdPos == std::string::npos)
		{
			line.clear();
		}
		else
		{
			line = line.substr(cmdPos + 1);
		}
	}

	return !command.empty();
}

int main(int argc, char* argv[])
{
	networking::ftp_processor ftpObj;
	bool run = true;

	switch (argc)
	{
	case 0:
	case 1:
		break;

	case 2:
		ftpObj.connect(argv[1]);
		break;

	case 3:
	default:
		ftpObj.connect(argv[1], atoi(argv[2]));
		break;
	}

	std::string command;
	std::string param1;
	std::string param2;

	// Connect
	while (run && !ftpObj.is_connected())
	{
		std::cout << "Please provide host info; expected format is:" << std::endl;
		std::cout << "\topen ip [port]" << std::endl;

		if (!get_command(command, param1, param2))
		{
			run = false;
		}
		else if ((command.compare("open") == 0) && !param1.empty())
		{
			if (param2.empty())
			{
				ftpObj.connect(param1);
			}
			else
			{
				ftpObj.connect(param1, atoi(param2.c_str()));
			}
		}
	}

	// Login
	while (run)
	{
#ifdef __linux__
		string userString( getlogin( ) );
#elif _WIN32
		char userString[UNLEN + 1];
		DWORD username_len = UNLEN + 1;
		GetUserNameA( userString, &username_len );
#endif
		std::cout << "Name (" << ftpObj.get_host_address() << ":" << userString << "): ";

		if (!get_command(command, param1, param2))
		{
			run = false;
		}
		else if (ftpObj.send_user_name(command))
		{
			std::cout << "Password: ";
			if (!get_command(command, param1, param2))
			{
				run = false;
			}
			else if (ftpObj.send_user_password(command))
			{
				ftpObj.show_os();

				break;
			}
		}
	}

	// Execute commands
	while (run)
	{
		std::cout << "ftp> ";

		if (!get_command(command, param1, param2))
		{
			continue;
		}

		bool success = false;

		if (command.compare("cd") == 0)
		{
			if (!param1.empty())
			{
				success = ftpObj.set_directory(param1);
			}
		}
		else if (command.compare("ls") == 0)
		{
			success = ftpObj.list_directories();
		}
		else if (command.compare("ldname") == 0)
		{
			success = ftpObj.list_directory_name();
		}
		else if (command.compare("currentdir") == 0)
		{
			success = ftpObj.get_directory();
		}
		else if (command.compare("cdup") == 0)
		{
			success = ftpObj.set_directory_to_parent();
		}
		else if (command.compare("removedir") == 0)
		{
			if (!param1.empty())
			{
				success = ftpObj.remove_directory(param1);
			}
		}
		else if (command.compare("makedir") == 0)
		{
			if (!param1.empty())
			{
				success = ftpObj.make_directory(param1);
			}
		}
		else if (command.compare("reinitialize") == 0)
		{
			success = ftpObj.reinitialize();
		}
		else if (command.compare("status") == 0)
		{
			success = ftpObj.status();
		}
		else if (command.compare("del") == 0)
		{
			if (!param1.empty())
			{
				success = ftpObj.delete_file(param1);
			}
		}
		else if (command.compare("sys") == 0)
		{
			success = ftpObj.show_os();
		}
		else if (command.compare("get") == 0)
		{
			if (!param1.empty())
			{
				success = ftpObj.get_file(param1);
			}
		}
		else if (command.compare("put") == 0)
		{
			if (!param1.empty())
			{
				success = ftpObj.put_file(param1);
			}
		}
		else if (command.compare("type") == 0)
		{
			if (!param1.empty())
			{
				if (param1 == "binary")
				{
					success = ftpObj.set_transfer_type( false );
				}
				else if (param1 == "ascii")
				{
					success = ftpObj.set_transfer_type( true );
				}
			}
		}
		else if (command.compare("close") == 0)
		{
			ftpObj.terminate();
			success = true;
		}
		else if (command.compare("quit") == 0)
		{
			ftpObj.terminate();
			run = false;
			success = true;
		}

		if (!success)
		{
			std::cout << command << " " << param1;
			std::cout << " failed" << std::endl;
		}
	}
	
	return 0;
}
