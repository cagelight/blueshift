#include "protocol.hh"

#define unexpected_terminate { srcprintf_error("unexpected program execution caused a terminate to be hit"); return status::terminate; }

blueshift::protocol_http::protocol_http(module::interface * mi, std::shared_ptr<connection> conin) : mi{mi}, con {conin}, sr{conin}, sw{conin} { }

blueshift::protocol_http::~protocol_http() {
	if (processing_token) { 
		mi->cleanup(processing_token);
	}
}

blueshift::protocol_http::status blueshift::protocol_http::update() {
	
	try {
		if (do_informational) {
			switch (sw.write()) {
				case stream_writer::status::complete:
					do_informational = false;
					res.clear();
					break;
				case stream_writer::status::some_wrote:
					return status::progress;
				case stream_writer::status::none_wrote:
					return status::idle;
				case stream_writer::status::failure:
					return status::terminate;
			}
		}
		
		switch (mode) {
			case mode_e::req_recv:
				return do_req_recv();
			case mode_e::req_query:
				return do_req_query();
			case mode_e::res_multipart:
				return do_res_multipart();
			case mode_e::res_process:
				return do_res_process();
			case mode_e::res_finalize:
				return do_res_finalize();
			case mode_e::res_send:
				return do_res_send();
		}
	} catch (general_exception & ge) {
		printf("connection terminated: what(): \"%s\"", ge.what());
		return status::terminate;
	} catch (...) {
		printf("WARNING: connection threw something that was not a blueshift::general_exception");
		return status::terminate;
	}
	unexpected_terminate;
}

blueshift::protocol_http::status blueshift::protocol_http::do_req_recv() {
	
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
			unexpected_terminate;
	}
	
	delim:
	std::vector<char> header_data = sr.get_delimited_data();
	
	req.clear();
	res.clear();
	
	res.code = req.parse_from(header_data);
	res.version = req.version;
	
	for (auto const & i : req.fields) {
		if (!strncasecmp("multipart/", i.second.c_str(), 10)) {
			
			auto bi = i.second.begin() + 10;
			auto ei = bi;
			
			for (; ei != i.second.end() && *ei != ';'; ei++);
			if (ei == i.second.end()) break;
			req.multipart_type = {bi, ei};
			
			if (ei++ == i.second.end()) break;
			if (ei++ == i.second.end()) break;
			
			bi = ei;
			
			for (; ei != i.second.end() && *ei != '='; ei++);
			if (ei == i.second.end()) break;
			
			std::string boundary_text {bi, ei};
			if (strcasecmp(boundary_text.c_str(), "boundary")) break;
			if (ei++ == i.second.end()) break;
			
			bi = ei;
			
			req.multipart_delimiter = {bi, i.second.end()};
			
			req.is_multipart = true;
			break;
		}
	}
	
	if (res.code != http::status_code::ok) {
		setup_send_error();
		return status::progress;
	}
	
	setup_req_query();
	return status::progress;
}

blueshift::protocol_http::status blueshift::protocol_http::do_req_query() {
	auto e = mi->query(&processing_token, req, reqq);
	if (e == module::mss::waiting) return status::idle;
	
	switch (reqq.q) {
		case module::request_query::qe::ok: {
			if (req.is_multipart) {
				std::string delim = "--";
				delim += req.multipart_delimiter;
				sr.set_delimiter(delim);
				setup_res_multipart();
				if (req.field("Expect") == "100-continue") {
					do_informational = true;
					res.clear();
					res.code = http::status_code::continue_s;
					sw.set_header(res.serialize());
					return status::progress;
				}
			} else {
				size_t cl = req.content_length();
				if (cl) {
					sr.set_counter(cl);
					setup_res_process();
				} else {
					setup_res_finalize();
				}
			}
			return status::progress;
		} 
		case module::request_query::qe::refuse_payload:
			setup_res_finalize();
			connection_close = true;
			return status::progress;
	}
	
	unexpected_terminate;
}

blueshift::protocol_http::status blueshift::protocol_http::do_res_process() {
	
	if (general_buffer.size()) {
		if (mi->process(processing_token, req, general_buffer) == module::mss::waiting) return status::idle;
		general_buffer.clear();
		switch (pwm) {
			case process_wait_mode::partial:
				break;
			case process_wait_mode::end:
				mi->process_end(processing_token, req);
				setup_res_finalize();
				return status::progress;
		}
	}
	
	if (sr.counter_is_set()) {
		switch(sr.read()) {
			case stream_reader::status::counter_hit: {
				general_buffer = sr.get_counted_data();
				pwm = process_wait_mode::end;
				if (mi->process(processing_token, req, general_buffer) == module::mss::waiting) return status::idle;
				mi->process_end(processing_token, req);
				setup_res_finalize();
				return status::progress;
			}
			case stream_reader::status::some_read: {
				general_buffer = sr.get_all_data();
				pwm = process_wait_mode::partial;
				if (mi->process(processing_token, req, general_buffer) == module::mss::waiting) return status::idle;
				return status::progress;
			}
			case stream_reader::status::none_read:
				return status::idle;
			case stream_reader::status::delimiter_detected:
				unexpected_terminate;
			case stream_reader::status::failure:
				return status::terminate;

		}
	}
	
	unexpected_terminate;
}

blueshift::protocol_http::status blueshift::protocol_http::do_res_multipart() {
	
	switch (mmode) {
		case multipart_mode::begin: {
			switch (sr.read()) {
				case stream_reader::status::delimiter_detected: {
					sr.get_delimited_data();
					sr.set_delimiter("\r\n\r\n");
					mmode = multipart_mode::peak_check;
					return status::progress;
				}
				case stream_reader::status::some_read: 
					return status::progress;
				case stream_reader::status::none_read:
					return status::idle;
				case stream_reader::status::counter_hit:
					unexpected_terminate;
				case stream_reader::status::failure:
					return status::terminate;
			}
		}
		case multipart_mode::peak_check: {
			switch (sr.read()) {
				case stream_reader::status::delimiter_detected: {
					auto const & p = sr.peak();
					if (p.size() < 2) return status::progress;
					if (p[0] != '\r' || p[1] != '\n') {
						setup_send_error();
						return status::progress;
					}
					sr.get_data(2);
					mmode = multipart_mode::header;
					return status::progress;
				}
				case stream_reader::status::some_read: 
					return status::progress;
				case stream_reader::status::none_read:
					return status::idle;
				case stream_reader::status::counter_hit:
					unexpected_terminate;
				case stream_reader::status::failure:
					return status::terminate;
			}
		}
		case multipart_mode::header: {
			switch(sr.read()) {
				case stream_reader::status::delimiter_detected: {
					general_buffer = sr.get_delimited_data();
					mul.clear();
					if (mul.parse_from(general_buffer)) {
						general_buffer.clear();
						std::string delim = "\r\n--";
						delim += req.multipart_delimiter;
						sr.set_delimiter(delim);
						mmode = multipart_mode::body;
						mi->process_multipart_begin(processing_token, req, mul);
						return status::progress;
					} else {
						res.code = http::status_code::bad_request;
						general_buffer.clear();
						setup_send_error();
						return status::progress;
					}
				}
				case stream_reader::status::some_read: 
					return status::progress;
				case stream_reader::status::none_read:
					return status::idle;
				case stream_reader::status::counter_hit:
					unexpected_terminate;
				case stream_reader::status::failure:
					return status::terminate;
			}
			unexpected_terminate;
		}
		case multipart_mode::body: {
			if (general_buffer.size()) {
				if (mi->process_multipart(processing_token, req, mul, general_buffer ) == module::mss::waiting) return status::idle;
				general_buffer.clear();
				switch (mwm) {
					case multipart_wait_mode::partial:
						break;
					case multipart_wait_mode::next:
						goto configure_next;
					case multipart_wait_mode::end:
						goto configure_done;
					default:
						unexpected_terminate;
				}
			}
			
			switch (sr.read()) {
				case stream_reader::status::delimiter_detected: {
					general_buffer = sr.get_delimited_data(false);
					auto const & p = sr.peak();
					if (p.size() < 2) return status::progress;
					else if (p[0] == '\r' && p[1] == '\n') {
						sr.get_data(2); // remove post-delimiters
						mwm = multipart_wait_mode::next;
						if (mi->process_multipart(processing_token, req, mul, general_buffer ) == module::mss::waiting) return status::idle;
						general_buffer.clear();
						goto configure_next;
					}
					else if (p.size() < 4) return status::progress;
					else if (!memcmp(p.data(), "--\r\n", 4)) {
						sr.get_data(4); // remove post-delimiters
						mwm = multipart_wait_mode::end;
						if (mi->process_multipart(processing_token, req, mul, general_buffer ) == module::mss::waiting) return status::idle;
						general_buffer.clear();
						goto configure_done;
					} else {
						srcprintf_error("unexpected multipart behavior!");
						return status::terminate;
					}
				}
				case stream_reader::status::some_read: 
					mwm = multipart_wait_mode::partial;
					general_buffer = sr.get_all_data(true);
					return status::progress;
				case stream_reader::status::none_read:
					return status::idle;
				case stream_reader::status::counter_hit:
					unexpected_terminate;
				case stream_reader::status::failure:
					return status::terminate;
			}
		}
	}
	unexpected_terminate;
	
	configure_next:
	mi->process_multipart_end(processing_token, req, mul);
	sr.set_delimiter("\r\n\r\n");
	mmode = multipart_mode::header;
	return status::progress;
						
	configure_done:
	mi->process_multipart_end(processing_token, req, mul);
	setup_res_finalize();
	return status::progress;
}

blueshift::protocol_http::status blueshift::protocol_http::do_res_finalize() {
	
	if (mi->finalize_response(processing_token, req, res, resq) == module::mss::waiting) return status::idle;
	if (processing_token) { 
		mi->cleanup(processing_token);
		processing_token = nullptr;
	}
	
	switch (resq.q) {
		case module::response_query::qe::no_body:
			res.fields["Content-Length"] = std::to_string(0);
			break;
		case module::response_query::qe::body_buffer:
			res.fields["Content-Length"] = std::to_string(resq.body_buffer.size());
			res.fields["Content-Type"] = resq.MIME;
			sw.set(std::move(resq.body_buffer));
			break;
		case module::response_query::qe::body_file: {
			
			std::string file_etag = resq.body_file->get_last_modified().generate_etag();
			
			std::string const & etag = req.field("If-Match");
			if (etag != empty_str && etag != file_etag) {
				res.code = http::status_code::precondition_failed;
				setup_send_error();
			}
			
			if (req.field("Range") != empty_str) {
				
				std::string const & etag = req.field("If-Range");
				if (etag != empty_str && etag != file_etag) goto cant_do_range;
				
				std::string const & range = req.field("Range");
				std::string::const_iterator bi, ei;
				bi = ei = range.begin();
				for (; ei != range.end() && *ei != '='; ei++);
				if (ei++ == range.end()) goto cant_do_range;
				if (ei == range.end()) goto cant_do_range;
				if (std::string{bi, ei} != "bytes=") goto cant_do_range;
				bi = ei;
				for (; ei != range.end() && *ei != '-'; ei++);
				if (ei == range.end()) goto cant_do_range;
				std::string from_vs {bi, ei};
				size_t from_v = strtoul(from_vs.c_str(), nullptr, 10);
				size_t to_v = resq.body_file->get_size() - 1;
				if (++ei != range.end()) {
					std::string to_vs {ei, range.end()};
					size_t to_vt = strtoul(to_vs.c_str(), nullptr, 10);
					if (to_vt <= to_v) to_v = to_vt;
				}
				to_v += 1;
				if (from_v > to_v) goto cant_do_range;
				size_t range_v = to_v - from_v;
				if (range_v == resq.body_file->get_size()) goto cant_do_range;
				res.code = http::status_code::partial_content;
				res.fields["Accept-Ranges"] = "bytes";
				res.fields["Content-Length"] = std::to_string(range_v);
				res.fields["Content-Type"] = resq.MIME;
				res.fields["Content-Range"] = "bytes " + std::to_string(from_v)  + "-" + std::to_string(to_v - 1) + "/" + std::to_string(resq.body_file->get_size());
				sw.set(resq.body_file, from_v, to_v);
				break;
			}
			cant_do_range:
			res.fields["Accept-Ranges"] = "bytes";
			res.fields["Content-Length"] = std::to_string(resq.body_file->get_size());
			res.fields["Content-Type"] = resq.MIME;
			sw.set(resq.body_file);
			break;
		}
	}
	
	res.fields["Server"] = "Blueshift";
	if (connection_close) {
		res.fields["Connection"] = "close";
	}
	
	sw.set_header(res.serialize());
	
	setup_res_send();
	return status::progress;
}

blueshift::protocol_http::status blueshift::protocol_http::do_res_send() {
	
	switch (sw.write()) {
		case stream_writer::status::complete:
			if (connection_close) return status::terminate;
			setup_req_recv();
			return status::progress;
		case stream_writer::status::some_wrote:
			return status::progress;
		case stream_writer::status::none_wrote:
			return status::idle;
		case stream_writer::status::failure:
			return status::terminate;
	}
	
	unexpected_terminate;
}

// ================================================================================================================================

void blueshift::protocol_http::setup_req_recv() {
	mode = mode_e::req_recv;
	general_buffer.clear();
	general_buffer.shrink_to_fit();
	req.clear();
	sr.set_delimiter("\r\n\r\n");
}

void blueshift::protocol_http::setup_req_query() {
	mode = mode_e::req_query;
	general_buffer.clear();
	reqq.reset();
	if (processing_token) { 
		mi->cleanup(processing_token);
		processing_token = nullptr;
	}
}

void blueshift::protocol_http::setup_res_process() {
	mode = mode_e::res_process;
	general_buffer.clear();
	pwm = process_wait_mode::partial;
	mi->process_begin(processing_token, req);
}

void blueshift::protocol_http::setup_res_multipart() {
	mode = mode_e::res_multipart;
	general_buffer.clear();
	mmode = multipart_mode::begin;
	mwm = multipart_wait_mode::partial;
}

void blueshift::protocol_http::setup_res_finalize() {
	mode = mode_e::res_finalize;
	general_buffer.clear();
	res.clear();
	resq.reset();
}

void blueshift::protocol_http::setup_res_send() {
	mode = mode_e::res_send;
	general_buffer.clear();
}

// ================================================================================================================================

void blueshift::protocol_http::setup_send_error() {
	res.fields.clear();
	std::string generror {};
	generror += "<h1>";
	generror += std::to_string(static_cast<unsigned int>(res.code));
	generror += " ";
	generror += http::text_for_status(res.code);
	generror += "</h1>";
	res.fields["Content-Type"] = "text/html";
	res.fields["Content-Length"] = std::to_string(generror.size());
	sw.set_header(res.serialize());
	sw.set( {generror.begin(), generror.end()} );
	setup_res_send();
	connection_close = true;
}

