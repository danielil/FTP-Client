/**
 * Daniel Sebastian Iliescu, http://dansil.net
 * MIT License (MIT), http://opensource.org/licenses/MIT
 *
 * This file contains main entry point of the program.
 */

#include "ftp_processor.hpp"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace std;

bool getCommand(std::string& command, std::string& param1, std::string& param2)
{
	std::string line;
	int index = 0;

	command.clear();
	param1.clear();
	param2.clear();

	getline(cin, line);

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
	FtpObj ftpObj;
	bool run = true;

	switch (argc)
	{
	case 0:
	case 1:
		break;

	case 2:
		ftpObj.Connect(argv[1]);
		break;

	case 3:
	default:
		ftpObj.Connect(argv[1], atoi(argv[2]));
		break;
	}

	std::string command;
	std::string param1;
	std::string param2;

	// Connect
	while (run && !ftpObj.IsConnected())
	{
		cout << "Please provide host info; expected format is:" << endl;
		cout << "\topen ip [port]" << endl;

		if (!getCommand(command, param1, param2))
		{
			run = false;
		}
		else if ((command.compare("open") == 0) && !param1.empty())
		{
			if (param2.empty())
			{
				ftpObj.Connect(param1.c_str());
			}
			else
			{
				ftpObj.Connect(param1.c_str(), atoi(param2.c_str()));
			}
		}
	}

	// Login
	while (run)
	{
		string userString( getlogin( ) );
		cout << "Name (" << ftpObj.GetHostAddress().c_str() << ":" << userString << "): ";

		if (!getCommand(command, param1, param2))
		{
			run = false;
		}
		else if (ftpObj.SendUserName(command.c_str()))
		{
			cout << "Password: ";
			if (!getCommand(command, param1, param2))
			{
				run = false;
			}
			else if (ftpObj.SendUserPassword(command.c_str()))
			{
				ftpObj.ShowOS();	

				break;
			}
		}
	}

	// Execute commands
	while (run)
	{
		cout << "ftp> ";

		if (!getCommand(command, param1, param2))
		{
			continue;
		}

		bool success = false;

		if (command.compare("cd") == 0)
		{
			if (!param1.empty())
			{
				success = ftpObj.SetDir(param1.c_str());
			}
		}
		else if (command.compare("ls") == 0)
		{
			success = ftpObj.ListDir();
		}
		else if (command.compare("ldname") == 0)
		{
			success = ftpObj.ListDirName();
		}
		else if (command.compare("currentdir") == 0)
		{
			success = ftpObj.GetDir();
		}
		else if (command.compare("cdup") == 0)
		{
			success = ftpObj.SetDirToParent();
		}
		else if (command.compare("removedir") == 0)
		{
			if (!param1.empty())
			{
				success = ftpObj.RemDir(param1.c_str());
			}
		}
		else if (command.compare("makedir") == 0)
		{
			if (!param1.empty())
			{
				success = ftpObj.MakeDir(param1.c_str());
			}
		}
		else if (command.compare("reinitialize") == 0)
		{
			success = ftpObj.Reinitialize();
		}
		else if (command.compare("status") == 0)
		{
			success = ftpObj.Status();
		}
		else if (command.compare("del") == 0)
		{
			if (!param1.empty())
			{
				success = ftpObj.DelFile(param1.c_str());
			}
		}
		else if (command.compare("sys") == 0)
		{
			success = ftpObj.ShowOS();
		}
		else if (command.compare("get") == 0)
		{
			if (!param1.empty())
			{
				success = ftpObj.GetFile(param1.c_str());
			}
		}
		else if (command.compare("put") == 0)
		{
			if (!param1.empty())
			{
				success = ftpObj.PutFile(param1.c_str());
			}
		}
		else if (command.compare("type") == 0)
		{
			if (!param1.empty())
			{
				if (param1 == "binary")
				{
					success = ftpObj.SetTransferType(0);
				}
				else if (param1 == "ascii")
				{
					success = ftpObj.SetTransferType(1);
				}
			}
		}
		else if (command.compare("close") == 0)
		{
			ftpObj.Terminate();
			success = true;
		}
		else if (command.compare("quit") == 0)
		{
			ftpObj.Terminate();
			run = false;
			success = true;
		}

		if (!success)
		{
			cout << command.c_str() << " " << param1.c_str();
			cout << " failed" << endl;
		}
	}
	
	return 0;
}
