/**
 * Daniel Sebastian Iliescu, http://dansil.net
 * MIT License (MIT), http://opensource.org/licenses/MIT
 */

#include "socket.hpp"

#include <array>

/*
	 Access Control Commands
	 -----------------------
		USER <username> <CRLF>
		PASS <password> <CRLF>
		ACCT <account-information> <CRLF>
		CWD  <pathname> <CRLF>				change working directory
		CDUP <CRLF>							change to parent directory
		SMNT <pathname> <CRLF>				structure mount
		QUIT <CRLF>							logout
		REIN <CRLF>							reinitialize
		PORT <host-port> <CRLF>				listening data port
		PASV <CRLF>							passive mode
		TYPE <type-code> <CRLF>				A/E/I = ASCII/EBCDIC/Image(binary)
		STRU <structure-code> <CRLF>		F/R/P = File/Record/Page
		MODE <transfer-code> <CRLF>			S/B/C = Stream/Block/Compressed

	 Service Commands
	 ----------------
		RETR <pathname> <CRLF>				download file
		STOR <pathname> <CRLF>				upload file
		STOU <CRLF>							store unique
		APPE <pathname> <CRLF>				append (with create)
		ALLO <decimal-integer> <CRLF>		allocate
		REST <marker> <CRLF>				restart
		RNFR <pathname> <CRLF>				rename from
		RNTO <pathname> <CRLF>				rename to
		ABOR <CRLF>							abort
		DELE <pathname> <CRLF>				delete file
		RMD  <pathname> <CRLF>				remove directory
		MKD  <pathname> <CRLF>				make directory
		PWD	<CRLF>							print working directory
		LIST <pathname> <CRLF>				list server files
		NLST <pathname> <CRLF>				list server directory
		SITE <string> <CRLF>				site parameters
		SYST <CRLF>							type server OS
		STAT <pathname> <CRLF>				status
		HELP <string> <CRLF>				help
		NOOP <CRLF>							No operation (no action)
*/

namespace networking
{
	// Class implementing an FTP client operating in Passive mode
	// It connects two sockets to the FTP server, one for commands
	// and the other one for data.
	class ftp_processor
	{
	public:
		ftp_processor() = default;
		virtual ~ftp_processor() noexcept;

		ftp_processor( ftp_processor& const ) = delete;
		ftp_processor( ftp_processor&& ) noexcept = delete;

		ftp_processor& operator=( ftp_processor const & ) = delete;
		ftp_processor& operator=( ftp_processor&& ) noexcept = delete;

		// Connection
		bool is_connected() const noexcept;
		bool is_data_connected() const noexcept;
		bool connect(
			std::string const & host_address,
			std::uint16_t port = 0 );
		void disconnect( bool full );

		// Commands
		bool ftp_command(
			std::string const & command,
			std::string const & param );
		void terminate();
		bool set_transfer_type( bool type );
		bool send_user_name( std::string const & name );
		bool send_user_password( std::string const & password );
		bool show_os();
		bool list_directories();
		bool list_directory_name();
		bool get_directory();
		bool set_directory( std::string const & directory );
		bool set_directory_to_parent();
		bool remove_directory( std::string const & directory );
		bool make_directory( std::string const & directory );
		bool reinitialize();
		bool status();
		bool delete_file( std::string const & filename );
		bool get_file( std::string const & filename );
		bool put_file( std::string const & filename );

		std::string get_host_address() const noexcept;

	private:
		void init();
		bool send_pasv();
		bool start_data_connection(
			std::string const & command,
			std::string const & parameter );
		void stop_data_connection( bool abort );
		bool send_command();
		bool receive_reply();

		// Command socket
		socket command_socket;
		// Data socket
		socket data_socket;
		// Max message size
		static constexpr auto FTP_MAX_MSG = 4096;
		// Message buffer
		std::array< char, FTP_MAX_MSG > message {};
		// True for ASCII, false for binary
		bool transfer_type = false;
		// Host address
		std::string host_address;
		// Port for transferring data
		std::uint16_t data_port = 0;
	};
}