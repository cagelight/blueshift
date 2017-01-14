#include "protocol.hh"

blueshift::protocol::protocol(std::unique_ptr<connection> && conin, server_request_handler h) : con(std::move(conin)), srh(h) {
	
}

blueshift::protocol::~protocol() {
	if (proxyfuture) delete proxyfuture;
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
		header.shrink_to_fit();
		req.parse(header.data(), header.size());
		u_state = state::res_calc;
		token = nullptr;
		res_calc_continue = false;
	}
	
	return status::progress;
}

blueshift::protocol::status blueshift::protocol::update_res_calc() {
	
	if (!res_calc_continue) {
		res.reset();
		if (req.get_status() != http::status_code::ok) {
			res.status = req.get_status();
			res.set_body_generic();
			goto skipperino;
		}
		res.status = http::status_code::ok;
	}
	
	module_status st;
	
	try {
		st = srh (req, res, token);
	} catch (blueshift::general_exception & e) {
		res.reset();
		res.status = http::status_code::internal_server_error;
		res.set_body(blueshift::strf("<h3>Blueshift General Exception thrown during module call!</h3><br><span style=\"font-weight: bold;\">what(): </span><span>%s</span>", e.what()).c_str(), "text/html");
		st = module_status::complete;
	} catch (std::exception & e) {
		res.reset();
		res.status = http::status_code::internal_server_error;
		res.set_body_generic();
		st = module_status::complete;
	} catch (...) {
		st = module_status::terminate;
	}
	
	switch (st) {
		case module_status::complete:
			break;
		case module_status::incomplete:
			res_calc_continue = true;
			return status::progress;
		case module_status::waiting:
			res_calc_continue = true;
			return status::idle;
		case module_status::terminate:
			return status::terminate;
	}
	
	if (res.btype == http::response::body_type::proxy) {
		setup_update_proxy();
		return status::progress;
	}
	
	skipperino:
	setup_update_res_send_head();
	
	return status::progress;
}

blueshift::protocol::status blueshift::protocol::update_res_send_head() {
	
	ssize_t write_cur = con->write(res_head.data() + write_ind, res_head.size() - write_ind);
	if (write_cur == 0) return status::idle;
	if (write_cur < 0) return status::terminate;
	write_ind += write_cur;
	if (write_ind == res_head.size()) {
		setup_update_res_send_body();
	}
	return status::progress;
}

blueshift::protocol::status blueshift::protocol::update_res_send_body() {
	
	switch (res.btype) {
		case http::response::body_type::buffer: {
			ssize_t write_cur = con->write(res.body.data() + write_ind, res.body.size() - write_ind);
			if (write_cur == 0) return status::idle;
			if (write_cur < 0) return status::terminate;
			write_ind += write_cur;
			if (write_ind == res.body.size()) {
				header.clear();
				header.shrink_to_fit();
				u_state = state::req_recv;
			}
		} break;
		case http::response::body_type::file: {
			ssize_t write_cur = con->sendfile(res.body_file.get_FD(), &writefile_offs, res.body_file.get_size());
			if (write_cur == 0) return status::idle;
			if (write_cur < 0) {
				return status::terminate;
			}
			if (static_cast<size_t>(writefile_offs) == res.body_file.get_size()) {
				header.clear();
				header.shrink_to_fit();
				u_state = state::req_recv;
			}
		} break;
		case http::response::body_type::proxy:
			throw blueshift::general_exception("update_res_send_body should never have been reached with a response set for proxy mode!");
	}
	
	return status::progress;
}

blueshift::protocol::status blueshift::protocol::update_proxy() {
	
	if (!proxycon) {
		future_connection::status stat = proxyfuture->get_status();
		switch (stat) {
			case future_connection::status::ready:
				proxycon = proxyfuture->get_connection();
				write_ind = 0;
				proxy_bufi = 0;
				break;
			case future_connection::status::connecting:
				return status::idle;
			case future_connection::status::failed:
				res.reset();
				res.status = http::status_code::bad_gateway;
				res.set_body_generic();
				setup_update_res_send_head();
				return status::progress;
			case future_connection::status::completed:
				srcthrow("proxycon is null and proxyfuture is reporting status::completed, this should not be possible.");
		}
	}
	
	if (write_ind < header.size()) {
		ssize_t write_cur = proxycon->write(header.data() + write_ind, header.size() - write_ind);
		if (write_cur == 0) return status::idle;
		if (write_cur < 0) return status::terminate;
		write_ind += write_cur;
		return status::progress;
	}
	
	if (!proxy_bufi) {
		proxy_bufi = proxycon->read(proxy_buf, 64);
		if (proxy_bufi == 0) return status::idle;
		if (proxy_bufi < 0) return status::terminate;
	} else {
		ssize_t e = con->write(proxy_buf, proxy_bufi);
		if (e == 0) return status::idle;
		if (e < 0) return status::terminate;
		proxy_bufi -= e;
	}
	
	if (con->read_available()) {
		if (proxyfuture) {
			delete proxyfuture;
			proxyfuture = nullptr;
		}
		proxycon.reset();
		setup_update_req_recv();
	}
	
	return status::progress;
}

blueshift::protocol::status blueshift::protocol::update() {
	switch(u_state) {
		case state::req_recv:
			return update_req_recv();
		case state::res_calc:
			return update_res_calc();
		case state::res_send_head:
			return update_res_send_head();
		case state::res_send_body:
			return update_res_send_body();
		case state::proxy:
			return update_proxy();
	}
	return status::terminate;
}

void blueshift::protocol::setup_update_req_recv() {
	header.clear();
	header.shrink_to_fit();
	u_state = state::req_recv;
}

void blueshift::protocol::setup_update_res_send_head() {
	res_head = res.create_header();
	u_state = state::res_send_head;
	write_ind = 0;
}

void blueshift::protocol::setup_update_res_send_body() {
	write_ind = 0;
	writefile_offs = 0;
	u_state = state::res_send_body;
}

void blueshift::protocol::setup_update_proxy() {
	
	if (proxycon) proxycon.reset();
	if (proxyfuture) delete proxyfuture;
	proxyfuture = new future_connection(res.proxy_host.c_str(), res.proxy_service.c_str());
		
	u_state = state::proxy;
}
