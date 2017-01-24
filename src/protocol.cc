#include "protocol.hh"

blueshift::protocol::protocol(std::unique_ptr<connection> && conin, module::interface & mi) : mi{mi}, con {std::move(conin)}, sr {*con}, sw {*con} {
	
}

blueshift::protocol::~protocol() {

}

blueshift::protocol::status blueshift::protocol::update() {
	
	switch (mode) {
		case mode_e::req_recv:
			return do_req_recv();
		case mode_e::req_query:
			return do_req_query();
		case mode_e::proxy:
			return do_proxy();
		case mode_e::res_process:
			return do_res_process();
		case mode_e::res_finalize:
			return do_res_finalize();
		case mode_e::res_send:
			return do_res_send();
	}
	return status::terminate;
}

blueshift::protocol::status blueshift::protocol::do_req_recv() {
	
	switch (sr.read()) {
		case stream_reader::status::some_read:
			return status::progress;
		case stream_reader::status::none_read:
			return status::idle;
		case stream_reader::status::delimiter_detected:
			goto delim;
		case stream_reader::status::failure:
			return status::terminate;
		case stream_reader::status::counter_hit:
			return status::terminate; // shouldn't happen
		default:
			return status::terminate;
	}
	
	delim:
	std::vector<char> header_data = sr.get_delimited_data();
	req.clear();
	res.clear();
	res.code = req.parse_from(header_data);
	res.version = req.version;
	reqq.reset();
	mode = mode_e::req_query;
	return status::progress;
}

blueshift::protocol::status blueshift::protocol::do_req_query() {
	auto e = mi.query(req, reqq);
	if (e == module::mss::waiting) return status::idle;
	
	switch (reqq.q) {
		case module::request_query::qe::ok: {
			size_t cl = req.content_length();
			if (cl) {
				sr.set_counter(cl);
				mode = mode_e::res_process;
			} else {
				mode = mode_e::res_finalize;
			}
			return status::progress;
		} 
		case module::request_query::qe::refuse_payload:
			mode = mode_e::res_finalize;
			return status::progress; // FIXME actually refuse payload
		case module::request_query::qe::proxy_direct:
			mode = mode_e::proxy;
			break;
		case module::request_query::qe::proxy_resolve:
			mode = mode_e::proxy;
			break;
	}
	
	return status::terminate;
}

blueshift::protocol::status blueshift::protocol::do_proxy() {
	// TODO
	return status::terminate;
}

blueshift::protocol::status blueshift::protocol::do_res_process() {
	
	if (general_buffer.size()) {
		if (mi.process(reqq.processing_token, general_buffer) == module::mss::proceed) {
			general_buffer.clear();
		} else {
			return status::idle;
		}
	}
	
	if (sr.counter_is_set()) {
		switch(sr.read()) {
			case stream_reader::status::counter_hit: {
				general_buffer = sr.get_counted_data();
				if (mi.process(reqq.processing_token, general_buffer) == module::mss::proceed) {
					general_buffer.clear();
					mode = mode_e::res_finalize;
				}
				return status::progress;
			}
			case stream_reader::status::some_read: {
				general_buffer = sr.get_all_data();
				if (mi.process(reqq.processing_token, general_buffer) == module::mss::proceed) {
					general_buffer.clear();
				}
				return status::progress;
			}
			case stream_reader::status::none_read:
				return status::idle;
			case stream_reader::status::delimiter_detected:
			case stream_reader::status::failure:
				return status::terminate;

		}
	}
	
	return status::terminate;
}

blueshift::protocol::status blueshift::protocol::do_res_finalize() {
	
	if (mi.finalize_response(reqq.processing_token, res, resq) == module::mss::waiting) return status::idle;
	
	switch (resq.q) {
		case module::response_query::qe::no_body:
			res.fields["Content-Length"] = std::to_string(0);
			break;
		case module::response_query::qe::body_buffer:
			res.fields["Content-Length"] = std::to_string(resq.body_buffer.size());
			res.fields["Content-Type"] = resq.MIME;
			sw.set(std::move(resq.body_buffer));
			break;
		case module::response_query::qe::body_file:
			res.fields["Content-Length"] = std::to_string(resq.body_file->get_size());
			res.fields["Content-Type"] = resq.MIME;
			sw.set(resq.body_file);
			break;
	}
	
	res.fields["Server"] = "Blueshift";
	
	sw.set_header(res.serialize());
	
	mode = mode_e::res_send;
	return status::progress;
}

blueshift::protocol::status blueshift::protocol::do_res_send() {
	
	switch (sw.write()) {
		case stream_writer::status::complete:
			mode = mode_e::req_recv;
			return status::progress;
		case stream_writer::status::some_wrote:
			return status::progress;
		case stream_writer::status::none_wrote:
			return status::idle;
		case stream_writer::status::failure:
			return status::terminate;
	}
	
	return status::terminate;
}
