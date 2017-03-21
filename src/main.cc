#include "com.hh"
#include "socket.hh"
#include "pool.hh"
#include "protocol.hh"

#include <cstdarg>
#include <csignal>

#include <thread>

#include <dlfcn.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

std::atomic_bool blueshift::run_sem {true};

static void catch_interrupt (int sig) {
	switch (sig) {
	case SIGINT:
		if (!blueshift::run_sem) std::terminate();
		blueshift::run_sem.store(false);
		break;
	}
}

static void blexit() {
	EVP_cleanup();
	exit(-1);
}

int main(int argc, char * * argv) {
	
	signal(SIGINT, catch_interrupt);
	signal(SIGPIPE, SIG_IGN);
	
	if (argc == 1) blexit(); // TODO -- usage print
	
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
	
	char const * module_name = argv[1];
	
	void * handle = dlopen(module_name, RTLD_NOW|RTLD_DEEPBIND);
	if (!handle) {
		srcprintf_error("could not open module:");
		blueshift::print(dlerror());
		blexit();
	}
	
	blueshift::module::import imp {};
	
	void * mod_init = dlsym(handle, "_blueshift_module_init");
	if (!mod_init) {
		srcprintf_error("could not find _blueshift_module_init in module");
		blexit();
	}
	
	blueshift::pool::init();
	
	imp.start_server = blueshift::pool::start_server;
	imp.start_server_ssl = blueshift::pool::start_server_ssl;
	imp.stop_server = blueshift::pool::stop_server;
	
	try {
		reinterpret_cast<void (*) (blueshift::module::import const *)>(mod_init)(&imp);
	} catch (blueshift::general_exception & e) {
		srcprintf_error("exception thrown during module initialization:\n%s", e.what());
		blexit();
	}
	
	blueshift::pool::enter_epoll_loop();
	blueshift::pool::term();
	
	void * mod_term = dlsym(handle, "_blueshift_module_term");
	if (mod_term) {
		reinterpret_cast<void (*)()>(mod_term)();
	}
	
	return 0;
}
