#include "pool.hh"
#include "protocol.hh"
#include "rwsl.hh"

#include <chrono>
#include <mutex>
#include <unordered_map>
#include <thread>

#include <cassert>

struct link {
	link * next = nullptr;
	link * prev = nullptr;
	blueshift::protocol * prot = nullptr;
	std::atomic_flag ctrl {false};
	~link() {
		if (prot) delete prot;
	}
};

struct server {
	blueshift::listener * lisock;
	blueshift::module::interface interface;
};

static link anchor;
static std::atomic_bool pool_run {true};
static rwslck pool_rwslck {};
static std::mutex lilock {};
static std::atomic<link *> cur {&anchor};
static std::unordered_map<uint16_t, server> lis;
static std::vector<std::thread> pool_workers;

void thread_run() {
	
	link * this_link = &anchor;
	blueshift::protocol::status stat;
	bool worked = false;
	
	while (pool_run) {
		pool_rwslck.read_access();
		this_link = cur.load();
		while (!cur.compare_exchange_strong(this_link, this_link->next)) {
			this_link = cur.load();
			std::this_thread::yield();
		}
		if (this_link->ctrl.test_and_set()) {
			pool_rwslck.read_done(); // yield the read lock, in case of a pending write lock
			continue;
		}
		pool_rwslck.read_done();
		
		if (!this_link->prot) { //anchor node
			
			bool accepted = false;
			
			lilock.lock();
			for (auto & li : lis) {
				
				if (li.second.interface.pulse) li.second.interface.pulse();
				
				auto c = li.second.lisock->accept();
				if (!c) continue;
				
				accepted = true;
				
				link * new_link = new link;
				new_link->prot = new blueshift::protocol {std::move(c), li.second.interface};
				pool_rwslck.write_lock();
				this_link->prev->next = new_link;
				new_link->prev = this_link->prev;
				this_link->prev = new_link;
				new_link->next = this_link;
				pool_rwslck.write_unlock();
				
			}
			lilock.unlock();
			this_link->ctrl.clear();
			if (!accepted && !worked) {
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
			} else {
				std::this_thread::yield();
			}
			worked = false;
			continue;
		}
		
		stat = this_link->prot->update();
		if (stat != blueshift::protocol::status::idle) {
			worked = true;
		}
		if (stat == blueshift::protocol::status::terminate) {
			pool_rwslck.write_lock();
			this_link->prev->next = this_link->next;
			this_link->next->prev = this_link->prev;
			if (cur == this_link) cur = this_link->next;
			pool_rwslck.write_unlock();
			delete this_link;
			continue;
		}
		this_link->ctrl.clear();
		continue;
	}
}

void blueshift::pool::init() {
	anchor.next = &anchor;
	anchor.prev = &anchor;
	
	for (unsigned int i = 0; i < std::thread::hardware_concurrency(); i++) { // FIXME
		pool_workers.emplace_back(thread_run);
	}
}

void blueshift::pool::term() noexcept {
	pool_run.store(false);
	for (std::thread & thr : pool_workers) {
		if (thr.joinable()) thr.join();
	}
	pool_workers.clear();
	
	for (auto const & li : lis) {
		delete li.second.lisock;
		if (li.second.interface.interface_term) li.second.interface.interface_term();
	}
	lis.clear();
	
	size_t cc = 0;
	while (anchor.next != &anchor) {
		cc++;
		link * l = anchor.next;
		anchor.next = l->next;
		delete l;
	}
	srcprintf_debug("%zu remaining connections closed", cc);
}

#include "module.hh"

void blueshift::pool::start_server(uint16_t port, module::interface h) {
	if (h.interface_init) h.interface_init();
	lilock.lock();
	lis[port] = { new blueshift::listener {port}, h };
	lilock.unlock();
}

void blueshift::pool::stop_server(uint16_t port) {
	lilock.lock();
	auto const & i = lis.find(port);
	if (i != lis.end()) {
		delete i->second.lisock;
		if (i->second.interface.interface_term) i->second.interface.interface_term();
		lis.erase(i);
	}
	lilock.unlock();
}
