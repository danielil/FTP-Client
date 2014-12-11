/**
* @author Daniel Sebastian Iliescu
* @file Ftp.cpp
* @date 2014-12-12

* This file contains main entry point of the program.
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace std;

#include "FtpObj.h"

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

	cout << "Command: [" << command.c_str();
	cout << "][" << param1.c_str();
	cout << "][" << param2.c_str();
	cout << "]" << endl;

	return !command.empty();
}

int main(int argc, char* argv[])
{
	FtpObj ftpObj;

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
	while (!ftpObj.IsConnected())
	{
		cout << "Please provide host info; expected format is:" << endl;
		cout << "\topen ip [port]" << endl;

		if (!getCommand(command, param1, param2))
		{
			break;
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
	while (ftpObj.IsConnected())
	{
		cout << "Please provide user name; expected format is:" << endl;
		cout << "\tname: account" << endl;

		if (!getCommand(command, param1, param2))
		{
			break;
		}
		else if ((command.compare("name:") == 0) && !param1.empty())
		{
			if (ftpObj.SendUserName(param1.c_str()))
			{
				break;
			}
		}
	}

	while (ftpObj.IsConnected())
	{
		cout << "Please provide user password; expected format is:" << endl;
		cout << "\tpassword: user_password" << endl;

		if (!getCommand(command, param1, param2))
		{
			break;
		}
		else if (command.compare("password:") == 0)
		{
			if (ftpObj.SendUserPassword(param1.c_str()))
			{
				break;
			}
		}
	}

	// Execute commands
	while (ftpObj.IsConnected())
	{
		cout << "Please enter a command: ";

		if (!getCommand(command, param1, param2))
		{
			break;
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
		else if (command.compare("close") == 0)
		{
			ftpObj.Terminate(false);
			success = true;
		}
		else if (command.compare("quit") == 0)
		{
			ftpObj.Terminate(true);
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
