/**
* @author Daniel Sebastian Iliescu
* @file FtpObj.h
* @date 2014-12-12

* This file contains the definition of the FTP object class
*/

#include "FtpSock.h"    // FtpSocket

/*
	Access Control Commands
	-----------------------
		USER <username> <CRLF>
		PASS <password> <CRLF>
		ACCT <account-information> <CRLF>
		CWD  <pathname> <CRLF>              change working directory
		CDUP <CRLF>                         change to parent directory
		SMNT <pathname> <CRLF>              structure mount
		QUIT <CRLF>                         logout
		REIN <CRLF>                         reinitialize
		PORT <host-port> <CRLF>             listening data port
		PASV <CRLF>                         passive mode
		TYPE <type-code> <CRLF>             A/E/I = ASCII/EBCDIC/Image(binary)
		STRU <structure-code> <CRLF>        F/R/P = File/Record/Page
		MODE <transfer-code> <CRLF>         S/B/C = Stream/Block/Compressed

	Service Commands
	----------------
		RETR <pathname> <CRLF>              download file
		STOR <pathname> <CRLF>              upload file
		STOU <CRLF>                         store unique
		APPE <pathname> <CRLF>              append (with create)
		ALLO <decimal-integer> <CRLF>       allocate
		REST <marker> <CRLF>                restart
		RNFR <pathname> <CRLF>              rename from
		RNTO <pathname> <CRLF>              rename to
		ABOR <CRLF>                         abort
		DELE <pathname> <CRLF>              delete file
		RMD  <pathname> <CRLF>              rmove directory
		MKD  <pathname> <CRLF>              make directory
		PWD <CRLF>                          print working directory
		LIST <pathname> <CRLF>              list server files
		NLST <pathname> <CRLF>              list server directory
		SITE <string> <CRLF>                site parameters
		SYST <CRLF>                         type server OS
		STAT <pathname> <CRLF>              status
		HELP <string> <CRLF>                help
		NOOP <CRLF>                         No operation (no action)
*/

const int FTP_MAX_MSG = 4096;

class FtpObj
{
public:
	FtpObj();
	virtual ~FtpObj();

	bool IsConnected();
	bool IsDataConnected();

	bool Connect(const char* hostAddress, int port = 0);
	void Disconnect(bool full);

	bool FtpCommand(const char* command, const char* param = NULL);
	void Terminate(bool quit);
	bool SetTransferType(bool bType);
	bool SendUserName(const char* name);
	bool SendUserPassword(const char* password);
	bool ListDir();
	const char* GetDir();
	bool SetDir(const char* directory);
	bool GetFile(const char* fileName);
	bool PutFile(const char* fileName);

private:
	FtpObj(const FtpObj&);
	FtpObj& operator =(const FtpObj&);

	void Init();
	bool SendPASV();
	bool StartDataConnection(const char* dataCommand, const char* dataParam = NULL);
	void StopDataConnection(bool abort = true);
	bool SendCommand();
	bool ReceiveReply();

	FtpSocket* _commandSocket;      // command socket
	FtpSocket* _dataSocket;         // data socket
	char _message[FTP_MAX_MSG];     // message buffer
	bool _transferType;             // FALSE/TRUE: Binary/ASCII
	std::string _hostAddress;       // host address
	int _dataPort;                  // port for transferring data
};
