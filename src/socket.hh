#pragma once

#include "com.hh"
#include <netinet/in.h>



namespace blueshift {
	
	struct socket {
		socket() { memset(&addr, 0, sizeof(addr)); }
		int fd = -1;
		struct sockaddr_in addr;
	};
	
	struct connection {
		connection(socket &);
		~connection();
		
		bool read_available();
		
		ssize_t read(char * buf, size_t buf_len);
		ssize_t write(char const * buf, size_t buf_len);
		ssize_t sendfile(int fd, off_t * offs, size_t size);

		inline int get_fd() { return sock.fd; }
		
	protected:
		socket sock {};
	};
	
	struct listener {
		listener(uint16_t port);
		~listener();
		
		std::shared_ptr<connection> accept();
		
		inline int get_fd() { return sock.fd; }
		
	protected:
		socket sock {};
	};
}
