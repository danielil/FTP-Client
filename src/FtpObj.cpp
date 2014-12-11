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

const int ERROR_THRESHOLD = 400;                // FTP Error Threshold
const unsigned long TERMINATE_LATENCY = 400L;   // 400 milliseconds

static void ExecDelay(unsigned long delay);

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

// Checks if command socket is connected to the FTP server
bool FtpObj::IsConnected()
{
	return (_commandSocket != NULL) && _commandSocket->IsOperational();
}

// Checks if data socket is connected to the FTP server 
bool FtpObj::IsDataConnected()
{
	return (_dataSocket != NULL) && _dataSocket->IsOperational();
}

// Opens an FTP session by connecting the command socket
bool FtpObj::Connect(const char* hostAddress, int port)
{
	if ((_commandSocket == NULL) &&
		(_dataSocket == NULL) &&
		(_hostAddress.empty()))
	{
		_commandSocket = new FtpSocket();
		if (_commandSocket->ConnectClientSocket(hostAddress, port))
		{
			_hostAddress = hostAddress;
			if (ReceiveReply())
			{
				return true;
			}
		}

		Disconnect(true);
	}

	return (false);
}

// Disconnects and destroys the command, the data and the listening sockets 
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
void FtpObj::Terminate(bool quit)
{
	bool connected = false;

	if (quit)
	{
		connected = IsConnected();
		if (connected)
		{
			FtpCommand("QUIT");
		}
	}

	Disconnect(true);
	Init();
  
	if (connected)
	{
		ExecDelay(TERMINATE_LATENCY);
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

// Sends the user name to the FTP server
bool FtpObj::SendUserName(const char* name)
{
	if (IsConnected() && (name != NULL))
	{
		if (FtpCommand("USER", name))
		{
			return true;
		}
	}

	return false;
}

// Sends the user name to the FTP server
bool FtpObj::SendUserPassword(const char* password)
{
	if (IsConnected() && (password != NULL))
	{
		if (FtpCommand("PASS", password))
		{
			if (FtpCommand("SYST"))
			{
				std::cout << _message << std::endl;
				return true;
			}
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
				memset(_message, 0, sizeof(_message));
				int numBytes = _dataSocket->ReceiveMessage((void *)_message, sizeof(_message)-1);
				if (numBytes == 0)
				{
					break;
				}

				std::cout << _message;

			}

			StopDataConnection(!success);
			std::cout << std::endl;
		}
	}

	return success;
}

// Retrieves the present working directory (PWD command) 
const char* FtpObj::GetDir(void)
{
	if (IsConnected())
	{
		if (FtpCommand("PWD"))
		{
			return _message;
		}
	}

	return NULL;
}

// Changes the directory on the the FTP server (CWD command) 
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

// Gets file from the FTP server (RETR command)
bool FtpObj::GetFile(const char* fileName)
{
	bool success = false;

	if (IsConnected() && (fileName != NULL))
	{
		FILE* fp = fopen(fileName, (_transferType ? "wt" : "wb"));
		if (fp != NULL)
		{
			if (StartDataConnection("RETR", fileName))
			{
				success = true;
				while (success)
				{
					memset(_message, 0, sizeof(_message));
					int numBytes = _dataSocket->ReceiveMessage((void *)_message, sizeof(_message));
					if (numBytes == 0)
					{
						break;
					}
					fwrite((const void*)_message, sizeof(char), numBytes, fp);
				}
				StopDataConnection(!success);
			}
			fclose(fp);
		}
	}

   return success;
}

// Puts file into the FTP server (STOR command)
bool FtpObj::PutFile(const char* fileName)
{
	bool success = false;

	if (IsConnected() && (fileName != NULL))
	{
		FILE* fp = fopen(fileName, (_transferType ? "rt" : "rb"));
		if (fp != NULL)
		{
			if (StartDataConnection("STOR", fileName))
			{
				success = true;
				while (success && (feof(fp) == 0))
				{
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
				StopDataConnection(!success);
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

// Sends the PASV request to the serve, forcing it to start listening. 
bool FtpObj::SendPASV()
{
	if (FtpCommand("PASV", "A"))
	{
		std::string msg = _message;

		if (msg.find("Entering Passive Mode") == std::string::npos)
		{
			if (!ReceiveReply())
			{
				return false;
			}
		}

		int nPort = 0;

		// Retrieve the data port
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

		std::cout << _hostAddress.c_str() << " " << _dataPort << std::endl;

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

// Sends a command message to the FTP server and retrieves the reply
bool FtpObj::SendCommand()
{
	if (IsConnected())
	{
		std::cout << "SEND: " << _message << std::endl;
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

			std::cout << "RECEIVE: " << _message << std::endl;
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

void ExecDelay(unsigned long delay)
{
	if (delay != 0L)
	{
		sleep(delay / 1000);
	}
}
