#include "com.hh"
#include "socket.hh"
#include "pool.hh"
#include "protocol.hh"

#include <cstdarg>
#include <csignal>

#include <thread>

#include <dlfcn.h>

std::atomic_bool blueshift::run_sem {true};

static void catch_interrupt (int sig) {
	switch (sig) {
	case SIGINT:
		if (!blueshift::run_sem) std::terminate();
		blueshift::run_sem.store(false);
		break;
	}
}

int main(int argc, char * * argv) {
	
	signal(SIGINT, catch_interrupt);
	signal(SIGPIPE, SIG_IGN);
	
	if (argc == 1) return -1; // TODO -- usage print
	
	char const * module_name = argv[1];
	
	void * handle = dlopen(module_name, RTLD_NOW|RTLD_DEEPBIND);
	if (!handle) {
		srcprintf_error("could not open module:");
		blueshift::print(dlerror());
		return -1;
	}
	
	blueshift::module::import imp {};
	
	void * mod_init = dlsym(handle, "_blueshift_module_init");
	if (!mod_init) {
		srcprintf_error("could not find _blueshift_module_init in module");
		return -1;
	}
	
	blueshift::pool::init();
	
	imp.start_server = blueshift::pool::start_server;
	imp.stop_server = blueshift::pool::stop_server;
	
	try {
		reinterpret_cast<void (*) (blueshift::module::import const *)>(mod_init)(&imp);
	} catch (blueshift::general_exception & e) {
		srcprintf_error("exception thrown during module initialization:\n%s", e.what());
		return -1;
	}
	
	blueshift::pool::enter_epoll_loop();
	blueshift::pool::term();
	
	void * mod_term = dlsym(handle, "_blueshift_module_term");
	if (mod_term) {
		reinterpret_cast<void (*)()>(mod_term)();
	}
	
	return 0;
}
