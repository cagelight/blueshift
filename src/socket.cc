#include "com.hh"
#include "socket.hh"

#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>

static constexpr int enable = 1;

blueshift::connection::connection(socket & sockin) : sock(sockin) {}

blueshift::listener::listener(uint16_t port) {
	
	sock.fd = ::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	setsockopt(sock.fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	setsockopt(sock.fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
	if (sock.fd == -1) srcthrow("failed to acquire socket file descriptor");
	
	sock.addr.sin_family = AF_INET;
	sock.addr.sin_addr.s_addr = INADDR_ANY;
	sock.addr.sin_port = htons(port);
	
	if (bind(sock.fd, reinterpret_cast<struct sockaddr *>(&sock.addr), sizeof(sock.addr)) < 0) srcthrow("failed to bind socket");
	if (listen(sock.fd, SOMAXCONN)) srcthrow("failed to begin listening");
}

blueshift::listener::~listener() {
	if (sock.fd != -1) {
		shutdown(sock.fd, SHUT_RDWR);
		close(sock.fd);
	}
}

std::unique_ptr<blueshift::connection> blueshift::listener::accept() {
	socket tsock;
	socklen_t slen = sizeof(tsock.addr);
	tsock.fd = accept4(sock.fd, reinterpret_cast<struct sockaddr *>(&tsock.addr), &slen, SOCK_NONBLOCK);
	if (tsock.fd == -1) return {nullptr};
	return std::unique_ptr<connection> {new connection {tsock}};
}


blueshift::connection::~connection() {
	if (sock.fd != -1) {
		shutdown(sock.fd, SHUT_RDWR);
		close(sock.fd);
	}
}

ssize_t blueshift::connection::read(char * buf, size_t buf_len) {
	ssize_t e = recv(sock.fd, buf, buf_len, 0);
	if (e == 0) return - 1;
	else if (e < 0) {
		if (errno == EAGAIN
			#if EAGAIN != EWOULDBLOCK
			|| errno == EWOULDBLOCK
			#endif
		) return 0;
		else return -1;
	} else return e;
}

ssize_t blueshift::connection::write(char const * buf, size_t buf_len) {
	ssize_t e = send(sock.fd, buf, buf_len, 0);
	if (e == 0) return - 1;
	else if (e < 0) {
		if (errno == EAGAIN
			#if EAGAIN != EWOULDBLOCK
			|| errno == EWOULDBLOCK
			#endif
		) return 0;
		else return -1;
	} else return e;
}

