#include "com.hh"
#include "socket.hh"

#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <netdb.h>
#include <fcntl.h>

/*
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
*/

static constexpr int enable = 1;
static constexpr int disable = 0;

blueshift::listener::listener(uint16_t port) {
	
	sock.fd = ::socket(PF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0);
	setsockopt(sock.fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	setsockopt(sock.fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
	setsockopt(sock.fd, IPPROTO_IPV6, IPV6_V6ONLY, &disable, sizeof(disable));
	if (sock.fd == -1) srcthrow("failed to acquire socket file descriptor");
	
	sock.addr.sin6_family = AF_INET6;
	sock.addr.sin6_addr = in6addr_any;
	sock.addr.sin6_port = htons(port);
	
	if (bind(sock.fd, reinterpret_cast<struct sockaddr *>(&sock.addr), sizeof(sock.addr)) < 0) srcthrow("failed to bind socket");
	if (listen(sock.fd, SOMAXCONN)) srcthrow("failed to begin listening");
}

blueshift::listener::~listener() {
	if (sock.fd != -1) {
		shutdown(sock.fd, SHUT_RDWR);
		close(sock.fd);
	}
}

std::shared_ptr<blueshift::connection_http> blueshift::listener::accept_http() {
	socket tsock;
	socklen_t slen = sizeof(tsock.addr);
	tsock.fd = accept4(sock.fd, reinterpret_cast<struct sockaddr *>(&tsock.addr), &slen, SOCK_NONBLOCK);
	if (tsock.fd == -1) return {nullptr};
	return std::shared_ptr<connection_http> {new connection_http {tsock}};
}

/*
std::shared_ptr<blueshift::ssl_connection> blueshift::listener::accept_secure(SSL_CTX * ctx) {
	socket tsock;
	socklen_t slen = sizeof(tsock.addr);
	tsock.fd = accept4(sock.fd, reinterpret_cast<struct sockaddr *>(&tsock.addr), &slen, SOCK_NONBLOCK);
	if (tsock.fd == -1) return {nullptr};
	return std::shared_ptr<ssl_connection> {new ssl_connection {tsock, ctx}};
}
*/

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================

std::string blueshift::connection::get_IP() {
	socklen_t len = sizeof(sockaddr_storage);
	struct sockaddr_storage addr;
	getpeername(sock.fd, (struct sockaddr*)&addr, &len);
	char ipstr[24] {};
	struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
	inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
	if (!strncmp("::ffff:", ipstr, 7)) {
		return {ipstr + 7};
	} else return {ipstr};
}
	
// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================


blueshift::connection_http::connection_http(socket & sockin) : connection(sockin) {}

blueshift::connection_http::~connection_http() {
	if (sock.fd != -1) {
		shutdown(sock.fd, SHUT_RDWR);
		close(sock.fd);
	}
}

bool blueshift::connection_http::read_available() {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sock.fd, &fds);
	struct timeval tv = {
		.tv_sec = 0,
		.tv_usec = 1,
	};
	int chk = select(sock.fd + 1, &fds, nullptr, nullptr, &tv);
	return chk;
}

ssize_t blueshift::connection_http::read(char * buf, size_t buf_len) {
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

ssize_t blueshift::connection_http::write(char const * buf, size_t buf_len) {
	ssize_t e = send(sock.fd, buf, buf_len, 0);
	if (e < 0) {
		if (errno == EAGAIN
			#if EAGAIN != EWOULDBLOCK
			|| errno == EWOULDBLOCK
			#endif
		) return 0;
		else return -1;
	} else return e;
}

ssize_t blueshift::connection_http::sendfile(int fd, off_t * offs, size_t size) {
	ssize_t e = ::sendfile(sock.fd, fd, offs, size);
	if (e < 0) {
		if (errno == EAGAIN
			#if EAGAIN != EWOULDBLOCK
			|| errno == EWOULDBLOCK
			#endif
		) return 0;
		else return -1;
	} else return e;
}

// ================================================================================================================================
// --------------------------------------------------------------------------------------------------------------------------------
// ================================================================================================================================
/*
blueshift::ssl_connection::ssl_connection(socket & sockin, SSL_CTX * ctx) : connection(sockin) {
	ssl = SSL_new(ctx);
	if (!SSL_set_fd(ssl, sock.fd)) {
		ERR_print_errors_fp(stderr);
		srcprintf_error("SSL connection could not be established on this socket: %s", get_IP().c_str());
	}
	if (SSL_accept(ssl) != 1) {
		ERR_print_errors_fp(stderr);
		srcprintf_error("SSL handshake failed: %s", get_IP().c_str());
	}
}

blueshift::ssl_connection::~ssl_connection() {
	if (ssl) SSL_free(ssl);
	if (sock.fd != -1) {
		shutdown(sock.fd, SHUT_RDWR);
		close(sock.fd);
	}
}

bool blueshift::ssl_connection::read_available() {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sock.fd, &fds);
	struct timeval tv = {
		.tv_sec = 0,
		.tv_usec = 1,
	};
	int chk = select(sock.fd + 1, &fds, nullptr, nullptr, &tv);
	return chk;
}

ssize_t blueshift::ssl_connection::read(char * buf, size_t buf_len) {
	ssize_t e = SSL_read(ssl, buf, buf_len);
	if (e == 0) return - 1;
	else if (e < 0) {
		if (SSL_get_error(ssl, e) == SSL_ERROR_WANT_READ) return 0;
		else return -1;
	} else return e;
}

ssize_t blueshift::ssl_connection::write(char const * buf, size_t buf_len) {
	ssize_t e = SSL_write(ssl, buf, buf_len);
	if (e < 0) {
		if (SSL_get_error(ssl, e) == SSL_ERROR_WANT_WRITE) return 0;
		else return -1;
	} else return e;
}

ssize_t blueshift::ssl_connection::sendfile(int fd, off_t * offs, size_t size) {
	ssize_t to_read = size - *offs;
	if (to_read > 4096) to_read = 4096;
	char dat [4096];
	if (lseek(fd, *offs, SEEK_SET) < 0) return -1;
	ssize_t alen = ::read(fd, dat, to_read);
	if (alen <= 0) return -1; 
	ssize_t e = write(dat, alen);
	if (e > 0) *offs += e;
	return e;
}
*/
