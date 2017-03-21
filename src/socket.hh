#pragma once

#include "com.hh"
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace blueshift {
	
	struct socket {
		socket() { memset(&addr, 0, sizeof(addr)); }
		int fd = -1;
		struct sockaddr_in6 addr;
	};
	
	struct connection {
		virtual ~connection() = default;
		int get_fd() { return sock.fd; }
		std::string get_IP();
		virtual ssize_t read(char * buf, size_t buf_len) = 0;
		virtual ssize_t write(char const * buf, size_t buf_len) = 0;
		virtual ssize_t sendfile(int fd, off_t * offs, size_t size) = 0;
	protected:
		connection(socket & sockin) : sock(sockin) {}
		socket sock {};
	};
	
	struct basic_connection : public connection {
		basic_connection(socket &);
		virtual ~basic_connection();
		
		bool read_available();
		
		virtual ssize_t read(char * buf, size_t buf_len) override;
		virtual ssize_t write(char const * buf, size_t buf_len) override;
		virtual ssize_t sendfile(int fd, off_t * offs, size_t size) override;
	};
	
	struct ssl_connection : public connection {
		ssl_connection(socket &, SSL_CTX * ctx);
		virtual ~ssl_connection();
		
		bool read_available();
		
		virtual ssize_t read(char * buf, size_t buf_len) override;
		virtual ssize_t write(char const * buf, size_t buf_len) override;
		virtual ssize_t sendfile(int fd, off_t * offs, size_t size) override;
		
	protected:
		SSL * ssl = nullptr;
		int current_sendfile_fd;
	};
	
	struct listener {
		listener(uint16_t port);
		~listener();
		
		std::shared_ptr<basic_connection> accept_basic();
		std::shared_ptr<ssl_connection> accept_secure(SSL_CTX * ctx);
		
		inline int get_fd() { return sock.fd; }
		
	protected:
		socket sock {};
	};
}
