/**
* @author Daniel Sebastian Iliescu
* @file FtpObj.cpp
* @date 2014-12-12

* This file contains the implementation of the FTP object class
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "FtpObj.h"

#define PASSIVE_MODE_FLAG				"Entering Passive Mode"
const int CONNECTED_OK					= 220;	// Expected connect reply
const int USERNAME_OK					= 331;	// Expected USER command reply
const int ERROR_THRESHOLD				= 400;	// FTP Error Threshold
const unsigned long TERMINATE_LATENCY	= 500L;	// 500 milliseconds

// Constructor
FtpObj::FtpObj()
{
	Init();
}

// Destructor
FtpObj::~FtpObj()
{
	Disconnect(true);
}

// Checks whether the command socket is connected to the FTP server
bool FtpObj::IsConnected()
{
	return (_commandSocket != NULL);
}

// Checks whether the data socket is connected to the FTP server 
bool FtpObj::IsDataConnected()
{
	return (_dataSocket != NULL);
}

// Opens an FTP session by connecting the command socket to the FTP server
bool FtpObj::Connect(const char* hostAddress, int port)
{
	if ((_commandSocket == NULL) &&
		(_dataSocket == NULL) &&
		(_hostAddress.empty()))
	{
		_commandSocket = new FtpSocket();
		if (_commandSocket->ConnectClientSocket(hostAddress, port))
		{
			if (ReceiveReply() && (atoi(_message) == CONNECTED_OK))
			{
				// Memorize the host address for subsequent data connection
				_hostAddress = hostAddress;
				return true;
			}
		}

		Disconnect(true);
	}

	return (false);
}

// Disconnects and destroys the command and the data sockets.
// If full == true, then it disconnects both sockets;
// Otherwise, it disconnects only the data socket.
void FtpObj::Disconnect(bool full)
{
	StopDataConnection(true);
	if (full)
	{
		if (_commandSocket != NULL)
		{
			delete _commandSocket;
		}
		Init();
	}
}

// Sends an FTP command with or without parameters to the FTP server.
// It checks the response and returns true if successful.
bool FtpObj::FtpCommand(const char* command, const char* param)
{
	if (IsConnected() && (command != NULL))
	{
		memset(_message, 0, sizeof(_message));
		if (param != NULL)
		{
			sprintf(_message, "%s %s\r\n", command, param);
		}
		else
		{
			sprintf(_message, "%s\r\n", command);
		}

		if (SendCommand())
		{
			return true;
		}
	}

	return false;
}

// Terminates all connections and optionally, quits FTP. 
void FtpObj::Terminate()
{
	if (IsConnected())
	{
		FtpCommand("QUIT");
		Disconnect(true);
	}
}

// Sets the type of the transfer to Binary or ASCII (TYPE command) 
bool FtpObj::SetTransferType(bool bType)
{
	if (IsConnected())
	{
		if (FtpCommand("TYPE", (bType ? "A" : "I")))
		{
			_transferType = bType;
			return true;
		}
	}

   return false;
}

// Sends the user name to the FTP server (USER command)
bool FtpObj::SendUserName(const char* name)
{
	if (IsConnected() && (name != NULL))
	{
		if (FtpCommand("USER", name) && (atoi(_message) == USERNAME_OK))
		{
			return true;
		}
	}

	return false;
}

// Sends the user password to the FTP server (PASS command)
bool FtpObj::SendUserPassword(const char* password)
{
	if (IsConnected() && (password != NULL))
	{
		if (FtpCommand("PASS", password))
		{
			if (!ReceiveReply())
			{
				return false;
			}

			return true;
		}
	}

	return false;
}

// Displays the operating system (SYST command)
bool FtpObj::ShowOS()
{
	if (IsConnected())
	{
		if (FtpCommand("SYST"))
		{
			return true;
		}
	}

	return false;
}

// Retrieves the content of the present working directory (LIST command) 
bool FtpObj::ListDir(void)
{
	bool success = false;

	if (IsConnected())
	{
		if (StartDataConnection("LIST", NULL))
		{
			success = true;

			while (success)
			{
				// Retrieves the directory content and displays it on the console
				memset(_message, 0, sizeof(_message));
				int numBytes = _dataSocket->ReceiveMessage((void *)_message, sizeof(_message)-1);
				if (numBytes == 0)
				{
					break;
				}

				// Display the data retrieved
				std::cout << _message;
			}
			std::cout << std::endl;

			StopDataConnection(false);
		}
	}

	return success;
}

// Retrieves only the names of the present working directory (NLST command) 
bool FtpObj::ListDirName(void)
{
	bool success = false;

	if (IsConnected())
	{
		if (StartDataConnection("NLST", NULL))
		{
			success = true;

			while (success)
			{
				// Retrieves the directory content and displays it on the console
				memset(_message, 0, sizeof(_message));
				int numBytes = _dataSocket->ReceiveMessage((void *)_message, sizeof(_message)-1);
				if (numBytes == 0)
				{
					break;
				}

				// Display the data retrieved
				std::cout << _message;
			}
			std::cout << std::endl;

			StopDataConnection(false);
		}
	}

	return success;
}

// Retrieves the present working directory (PWD command) 
bool FtpObj::GetDir()
{
	if (IsConnected())
	{
		if (FtpCommand("PWD"))
		{
			return true;
		}
	}

	return NULL;
}

// Changes the current directory on the the FTP server (CWD command) 
bool FtpObj::SetDir(const char* directory)
{
	if (IsConnected() && (directory != NULL))
	{
		if (FtpCommand("CWD", directory))
		{
			return true;
		}
	}

	return false;
}

// Changes the current directory to the parent directory (CDUP command)
bool FtpObj::SetDirToParent()
{
	if (IsConnected())
	{
		if (FtpCommand("CDUP"))
		{
			return true;
		}
	}

	return false;
}

// Removes the selected directory on the FTP server (RMD command)
// This functions doesn’t support recursive deletion (if the folder has files in it),
// and will fail if the folder is not empty.
bool FtpObj::RemDir(const char* directory)
{
	if (IsConnected() && (directory != NULL))
	{
		if (FtpCommand("RMD", directory))
		{
			return true;
		}
	}

	return false;
}

// Makes a directory on the server (MKD command)
bool FtpObj::MakeDir(const char* directory)
{
	if (IsConnected() && (directory != NULL))
	{
		if (FtpCommand("MKD", directory))
		{
			return true;
		}
	}

	return false;
}

// Terminate the USER session and purge all account information (REIN command)
bool FtpObj::Reinitialize()
{
	if (IsConnected())
	{
		if (FtpCommand("REIN"))
		{
			return true;
		}
	}

	return false;
}

// Returns server status (STAT command)
bool FtpObj::Status()
{
	if (IsConnected())
	{
		if (FtpCommand("STAT"))
		{
			return true;
		}
	}

	return false;
}

// Delete a file from the FTP server (DELE command)
bool FtpObj::DelFile(const char* fileName)
{
	if (IsConnected() && (fileName != NULL))
	{
		if (FtpCommand("DELE", fileName))
		{
			return true;
		}
	}

	return false;
}

// Downloads a file from the FTP server (RETR command)
bool FtpObj::GetFile(const char* fileName)
{
	bool success = false;

	if (IsConnected() && (fileName != NULL))
	{
		FILE* fp = fopen(fileName, (_transferType ? "wt" : "wb"));
		if (fp != NULL)
		{
			SetTransferType(_transferType);
			if (StartDataConnection("RETR", fileName))
			{
				success = true;
				while (success)
				{
					// Get data from server and writes to the local file
					memset(_message, 0, sizeof(_message));
					int numBytes = _dataSocket->ReceiveMessage((void *)_message, sizeof(_message));
					if (numBytes == 0)
					{
						break;
					}
					fwrite((const void*)_message, sizeof(char), numBytes, fp);
				}
				StopDataConnection(false);
			}
			fclose(fp);
		}
	}

	return success;
}

// Uploads a file to the FTP server (STOR command)
bool FtpObj::PutFile(const char* fileName)
{
	bool success = false;

	if (IsConnected() && (fileName != NULL))
	{
		FILE* fp = fopen(fileName, (_transferType ? "rt" : "rb"));
		if (fp != NULL)
		{
			SetTransferType(_transferType);
			if (StartDataConnection("STOR", fileName))
			{
				success = true;
				while (success && (feof(fp) == 0))
				{
					// Reads file content and sends it to the server
					memset(_message, 0, sizeof(_message));
					int numBytes = fread((void *)_message, sizeof(char), sizeof(_message), fp);
					if (numBytes == 0)
					{
						success = false;
						break;
					}
					if (_dataSocket->SendMessage((void *)_message, numBytes) == 0)
					{
						success = false;
						break;
					}
				}
				StopDataConnection(false);
			}
			fclose(fp);
		}
	}

	return success;
}

// Initialization method
void FtpObj::Init()
{
	_commandSocket = NULL;
	_dataSocket = NULL;
	memset(_message, 0, sizeof(_message));
	_transferType = false;
	_hostAddress.clear();
	_dataPort = 0;
}

// Sends the PASV request to the server, forcing it to start listening. 
// The response should include the host address and the data port;
// since we saved the host address at connection time, we only retrieve
// the data port.
bool FtpObj::SendPASV()
{
	if (FtpCommand("PASV"))
	{
		std::string msg = _message;

		// Wait for Passive mode indicator
		if (msg.find(PASSIVE_MODE_FLAG) == std::string::npos)
		{
			if (!ReceiveReply())
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
		int nPort = 0;

		for (int nPos = 1, i = strlen(_message) - 1; i > 0; i--)
		{
			if (isdigit((int)_message[i]))
			{
				nPort += (unsigned int)(nPos * (_message[i] - '0'));
				nPos *= 10;
			}
			else if (nPos > 1)
			{
				for (nPos = 256; i > 0; i--)
				{
					if (isdigit((int)_message[i]))
					{
						nPort += (unsigned int)(nPos * (_message[i] - '0'));
						nPos *= 10;
					}
					else if (nPos > 256)
					{
						_dataPort = nPort;
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
bool FtpObj::StartDataConnection(const char* dataCommand, const char* dataParam)
{
	if (IsConnected() && (_dataSocket == NULL) && (dataCommand != NULL) && SendPASV())
	{
		FtpSocket* pDataSocket = new FtpSocket();

		if (pDataSocket->ConnectClientSocket(_hostAddress.c_str(), _dataPort))
		{
			_dataSocket = pDataSocket;
		}
		else
		{
			delete pDataSocket;
			return false;
		}

		if (FtpCommand(dataCommand, dataParam))
		{
			return true;
		}

		StopDataConnection(true);
	}

	return (false);
}

// Disconnects the socket used for data transfer.
// If "abort" flag is not set, the we wait for a server reply. 
void FtpObj::StopDataConnection(bool abort)
{
	if (_dataSocket != NULL)
	{
		delete _dataSocket;
		_dataSocket = NULL;
	}

	if (!abort)
	{
		ReceiveReply();
	}
}

// Sends a command message to the FTP server and retrieves the reply.
// If the reply includes an TCP/IP transfer code < 400, then we consider
// that the command transmission was successful.
bool FtpObj::SendCommand()
{
	if (IsConnected())
	{
		if (_commandSocket->SendMessage((void *)_message, strlen(_message)))
		{
			if (ReceiveReply())
			{
				return true;
			}
		}
	}

	return false;
}

// Receives the reply message on a command request sent to the FTP server 
bool FtpObj::ReceiveReply()
{
	bool success = false;

	if (IsConnected())
	{
		memset(_message, 0, sizeof(_message));
		if (_commandSocket->ReceiveMessage((void *)_message, sizeof(_message)))
		{
			// Check reply
			int nPos = 0;
			int maxPos = (int)sizeof(_message) - 5;

			std::cout << _message;
			while (!success && (nPos < maxPos) &&
				((_message[nPos + 0] == '-') ||
				((_message[nPos + 1] == ' ') &&
				 (_message[nPos + 2] == ' ') &&
				 (_message[nPos + 3] == ' '))))
			{
				// skip continuation lines (denoted by dash or spaces)
				int nLen = maxPos - nPos;

				for (int i = 0; i < nLen; i++)
				{
					if (_message[nPos + i] == 0x00)
					{
						success = true;
						break;
					}
					else if (_message[nPos + i] == 0x0A)
					{
						nPos = i + 1;
					}
				}
			}

			if (!success && (nPos < maxPos))
			{
				if (atoi((const char*)&_message[nPos]) < ERROR_THRESHOLD)
				{
					success = true;
				}
			}
		}
	}

	return success;
}

std::string FtpObj::GetHostAddress()
{
	return _hostAddress;
}
