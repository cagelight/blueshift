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

bool blueshift::connection::read_available() {
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
	if (e < 0) {
		if (errno == EAGAIN
			#if EAGAIN != EWOULDBLOCK
			|| errno == EWOULDBLOCK
			#endif
		) return 0;
		else return -1;
	} else return e;
}

ssize_t blueshift::connection::sendfile(int fd, off_t * offs, size_t size) {
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

blueshift::future_connection::future_connection(const char * host, const char * service) {
	
	sock.fd = ::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	setsockopt(sock.fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	setsockopt(sock.fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
	if (sock.fd == -1) srcthrow("failed to acquire socket file descriptor");
	
	addrinfo * addri, * addrii;
	addrinfo hint {};
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	int r = getaddrinfo(host, service, &hint, &addri);
	if (r != 0) {
		status_ = status::failed;
		return;
	}
	
	sockaddr_in * saddr = nullptr;
	addrii = addri;
	
	while (addrii != nullptr) {
		if (addrii->ai_family == AF_INET) {
			saddr = reinterpret_cast<sockaddr_in *>(addri->ai_addr);
			break;
		}
		addrii = addrii->ai_next;
	}
	
	if (!saddr) {
		status_ = status::failed;
		freeaddrinfo(addri);
		return;
	}
	
	sock.addr = * saddr;
	freeaddrinfo(addri);
	
	srcprintf_debug("opening proxy to: %s", inet_ntoa(saddr->sin_addr));

	r = connect(sock.fd, reinterpret_cast<struct sockaddr *>(&sock.addr), sizeof(sock.addr));
	if (r != 0 && errno != EINPROGRESS) {
		status_ = status::failed;
		return;
	}
	status_ = status::connecting;
}

blueshift::future_connection::future_connection(const char * ip, uint16_t port) {
	
	sock.fd = ::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	setsockopt(sock.fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	setsockopt(sock.fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
	if (sock.fd == -1) srcthrow("failed to acquire socket file descriptor");
	
	int r = inet_aton(ip, &sock.addr.sin_addr);
	if (!r) {
		status_ = status::failed;
		return;
	}
	
	sock.addr.sin_port = port;
	sock.addr.sin_family = AF_INET;
	
	srcprintf_debug("opening proxy to: %s", inet_ntoa(sock.addr.sin_addr));
	r = connect(sock.fd, reinterpret_cast<struct sockaddr *>(&sock.addr), sizeof(sock.addr));
	if (r != 0 && errno != EINPROGRESS) {
		status_ = status::failed;
		return;
	}
	
	status_ = status::connecting;
}

blueshift::future_connection::~future_connection() {
	if (sock.fd != -1) {
		shutdown(sock.fd, SHUT_RDWR);
		close(sock.fd);
	}
}

blueshift::future_connection::status blueshift::future_connection::get_status() {
	if (status_ == status::connecting) {
		int err;
		socklen_t err_len = sizeof(err);
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(sock.fd, &fds);
		struct timeval sw = {
			0,
			100,
		};
		int ret = select(sock.fd + 1, nullptr, &fds, nullptr, &sw);
		if (!ret) return status::connecting;
		int r = getsockopt(sock.fd, SOL_SOCKET, SO_ERROR, &err, &err_len);
		if (r != 0 || (err != 0 && err != EINPROGRESS)) {
			status_ = status::failed;
		}
		else if (err == 0) status_ = status::ready;
	}
	return status_;
}

std::unique_ptr<blueshift::connection> blueshift::future_connection::get_connection() {
	if (status_ != status::ready) return nullptr;
	std::unique_ptr<connection> con {new connection {sock}};
	sock.fd = -1;
	status_ = status::completed;
	return con;
}




