/**
 * Daniel Sebastian Iliescu, http://dansil.net
 * MIT License (MIT), http://opensource.org/licenses/MIT
 */

#ifdef __linux__
	using SOCKET = int;
#elif _WIN32
	#include <WinSock2.h>
#endif

#include <stdint.h>
#include <string>

namespace networking
{
	class socket
	{
	public:

		socket() = default;
		virtual ~socket() noexcept;

		socket( socket const & ) = delete;
		socket( socket&& ) noexcept = delete;

		socket& operator=( socket const & ) = delete;
		socket& operator=( socket&& ) noexcept = delete;

		bool is_connected() const noexcept;

		bool connect_client_socket(
			std::string const & host_address,
			std::uint16_t port = 0 ) noexcept;
		void close() noexcept;

		int send_message(
			void* buffer,
			std::size_t buffer_size ) const noexcept;
		int receive_message(
			void* buffer,
			std::size_t buffer_size ) const noexcept;
		int receive_message_all(
			void* buffer,
			std::size_t buffer_size ) const noexcept;

	private:
		SOCKET socket_handle = 0;
	};
}