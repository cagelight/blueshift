#include "protocol.hh"

blueshift::protocol::protocol(std::unique_ptr<connection> && conin) : con(std::move(conin)) {
	
}

constexpr size_t blueshift::protocol::work_buf_len;
thread_local char blueshift::protocol::work_buf[blueshift::protocol::work_buf_len];

blueshift::protocol::status blueshift::protocol::update_req_recv() {
	
	ssize_t r = con->read(work_buf, work_buf_len);
	if (r == 0) return status::idle;
	if (r < 0) return status::terminate;
	
	header.insert(header.end(), work_buf, work_buf + r);
	size_t hs = header.size();
	
	if (hs > 4 && !memcmp("\r\n\r\n", header.data() + hs - 4, 4)) {
		req.set_buffer(header.data(), header.size());
		req.parse();
		u_state = state::res_calc;
	}
	
	return status::progress;
}

blueshift::protocol::status blueshift::protocol::update_res_calc() {
	return status::terminate;
}

blueshift::protocol::status blueshift::protocol::update_res_send() {
	return status::terminate;
}

blueshift::protocol::status blueshift::protocol::update() {
	switch(u_state) {
		case state::req_recv:
			return update_req_recv();
		case state::res_calc:
			return update_res_calc();
		case state::res_send:
			return update_res_send();
	}
	return status::terminate;
}
