#include "com.hh"
#include "socket.hh"
#include "pool.hh"
#include "protocol.hh"

#include <cstdarg>
#include <csignal>

std::atomic_bool com::run_sem {true};

static void catch_interrupt (int sig) {
	switch (sig) {
	case SIGINT:
		if (!com::run_sem) std::terminate();
		com::run_sem.store(false);
		break;
	case SIGPIPE:
		srcprintf_debug("a SIGPIPE was ignored");
		break;
	}
}

int main() {
	
	signal(SIGINT, catch_interrupt);
	signal(SIGPIPE, catch_interrupt);
	
	
	try {
		
		blueshift::listener l {2080};
		
		while (com::run_sem) {
			std::unique_ptr<blueshift::connection> con = l.accept();
			if (con) {
				blueshift::protocol p {std::move(con)};
				while (com::run_sem) {
					if (p.update() == blueshift::protocol::status::terminate) break;
				}
				break;
			}
		}
	} catch ( com::general_exception const & ex ) {
		com::print(ex.what());
	}
	
	return 0;
}

static constexpr size_t strf_startlen = 256;
std::string com::strf(char const * fmt, ...) noexcept {
	va_list va;
	va_start(va, fmt);
	char * tmp_buf = reinterpret_cast<char *>(malloc(strf_startlen));
	tmp_buf[strf_startlen - 1] = 0;
	size_t req_size = vsnprintf(tmp_buf, strf_startlen, fmt, va);
	va_end(va);
	if (req_size >= strf_startlen) {
		tmp_buf = reinterpret_cast<char *>(realloc(tmp_buf, req_size+1));
		va_start(va, fmt);
		vsprintf(tmp_buf, fmt, va);
		va_end(va);
		tmp_buf[req_size] = 0;
	}
	std::string r = {tmp_buf};
	free(tmp_buf);
	return {r};
}

void com::print(char const * str) noexcept {
	std::printf("%s\n", str);
}
