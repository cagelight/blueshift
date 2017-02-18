#include "pool.hh"
#include "protocol.hh"
#include "rwsl.hh"

#include <chrono>
#include <mutex>
#include <unordered_map>
#include <thread>
#include <condition_variable>

#include <cassert>

#include <unistd.h>
#include <sys/epoll.h>

struct conlink {
	conlink * next = nullptr;
	conlink * prev = nullptr;
	blueshift::protocol * prot = nullptr;
	std::atomic_flag ctrl {false};
	~conlink() {
		if (prot) delete prot;
	}
};

struct server {
	blueshift::listener * lisock;
	blueshift::module::interface * interface;
};

static conlink anchor;
static std::atomic_bool pool_run {true};
static rwslck pool_rwslck {};
static std::mutex lilock {};
static std::atomic<conlink *> cur {&anchor};
static std::unordered_map<uint16_t, server> lis;
static std::vector<std::thread> pool_workers;
static std::mutex cvm {};
static std::condition_variable cv {};

static int epoll_obj = 0;
static thread_local struct epoll_event epoll_evt {};
inline static void clear_epoll_evt() { memset(&epoll_evt, 0, sizeof(struct epoll_event)); }

void thread_run() {
	
	conlink * this_conlink = &anchor;
	blueshift::protocol::status stat;
	bool worked = false;
	
	while (pool_run) {
		pool_rwslck.read_access();
		this_conlink = cur.load();
		while (!cur.compare_exchange_strong(this_conlink, this_conlink->next)) {
			this_conlink = cur.load();
			std::this_thread::yield();
		}
		if (this_conlink->ctrl.test_and_set()) {
			pool_rwslck.read_done(); // yield the read lock, in case of a pending write lock
			continue;
		}
		pool_rwslck.read_done();
		
		if (!this_conlink->prot) { //anchor node
			
			bool accepted = false;
			
			lilock.lock();
			for (auto & li : lis) {
				
				li.second.interface->pulse();
				
				auto c = li.second.lisock->accept();
				if (!c) continue;
				
				accepted = true;
				
				conlink * new_conlink = new conlink;
				new_conlink->prot = new blueshift::protocol {c, li.second.interface};
				
				clear_epoll_evt();
				epoll_evt.events = EPOLLIN | EPOLLOUT | EPOLLET;
				epoll_ctl(epoll_obj, EPOLL_CTL_ADD, new_conlink->prot->get_fd(), &epoll_evt);
				
				pool_rwslck.write_lock();
				this_conlink->prev->next = new_conlink;
				new_conlink->prev = this_conlink->prev;
				this_conlink->prev = new_conlink;
				new_conlink->next = this_conlink;
				pool_rwslck.write_unlock();
				
			}
			lilock.unlock();
			this_conlink->ctrl.clear();
			if (!accepted && !worked) {
				std::unique_lock<std::mutex> lk(cvm);
				cv.wait_for(lk, std::chrono::milliseconds(1000));
			} else {
				std::this_thread::yield();
			}
			worked = false;
			continue;
		}
		
		stat = this_conlink->prot->update();
		if (stat != blueshift::protocol::status::idle) {
			worked = true;
		}
		if (stat == blueshift::protocol::status::terminate) {
			pool_rwslck.write_lock();
			this_conlink->prev->next = this_conlink->next;
			this_conlink->next->prev = this_conlink->prev;
			if (cur == this_conlink) cur = this_conlink->next;
			pool_rwslck.write_unlock();
			delete this_conlink;
			continue;
		}
		this_conlink->ctrl.clear();
		continue;
	}
}

void blueshift::pool::init() {
	anchor.next = &anchor;
	anchor.prev = &anchor;
	
	for (unsigned int i = 0; i < std::thread::hardware_concurrency(); i++) { // FIXME
		pool_workers.emplace_back(thread_run);
	}
	
	epoll_obj = epoll_create(1);
}

void blueshift::pool::term() noexcept {
	pool_run.store(false);
	for (std::thread & thr : pool_workers) {
		if (thr.joinable()) thr.join();
	}
	pool_workers.clear();
	
	for (auto const & li : lis) {
		delete li.second.lisock;
		li.second.interface->interface_term();
	}
	lis.clear();
	
	size_t cc = 0;
	while (anchor.next != &anchor) {
		cc++;
		conlink * l = anchor.next;
		anchor.next = l->next;
		delete l;
	}
	close(epoll_obj);
	srcprintf_debug("%zu remaining connections closed", cc);
}

void blueshift::pool::enter_epoll_loop() {
	
	while (blueshift::run_sem) {
		clear_epoll_evt();
		epoll_wait(epoll_obj, &epoll_evt, 1, 1000);
		cv.notify_one();
	}
}

#include "module.hh"

void blueshift::pool::start_server(uint16_t port, module::interface * h) {
	h->interface_init();
	lilock.lock();
	auto * li = new blueshift::listener {port};
	lis[port] = { li, h };
	
	clear_epoll_evt();
	epoll_evt.events = EPOLLIN;
	epoll_ctl(epoll_obj, EPOLL_CTL_ADD, li->get_fd(), &epoll_evt);
	
	lilock.unlock();
}

void blueshift::pool::stop_server(uint16_t port) {
	lilock.lock();
	auto const & i = lis.find(port);
	if (i != lis.end()) {
		delete i->second.lisock;
		i->second.interface->interface_term();
		lis.erase(i);
	}
	lilock.unlock();
}
