#pragma once

#include "socket.hh"
#include "http.hh"
#include "module.hh"
#include "util.hh"

#include <vector>

namespace blueshift {
	
	struct protocol {
		
		virtual ~protocol() = default;

		enum struct status : uint_fast8_t {
			idle,
			progress,
			terminate,
		};
		
		virtual status update() = 0;
		virtual int get_fd() = 0;
		
	};
	
	struct protocol_http : public protocol {
		
		protocol_http(module::interface * mi, std::shared_ptr<connection> conin);
		~protocol_http();
		
		virtual status update() override;
		virtual int get_fd() override { return con->get_fd(); }
		
	private:
		
		module::interface * mi;
		
		bool connection_close = false;
		
		http::request_header req {};
		http::response_header res {};
		http::multipart_header mul {};
		
		void * processing_token = nullptr;
		
		module::request_query reqq {};
		module::response_query resq {};
		
		bool do_informational = false;
		
		enum struct mode_e {
			req_recv,
			req_query,
			res_process,
			res_multipart,
			res_finalize,
			res_send,
		} mode = mode_e::req_recv;
		
		enum struct process_wait_mode {
			partial,
			end
		} pwm;
		
		enum struct multipart_mode {
			begin,
			peak_check,
			header,
			body
		} mmode = multipart_mode::header;
		
		enum struct multipart_wait_mode {
			partial,
			next,
			end
		} mwm;
		
		status do_req_recv();
		status do_req_query();
		status do_res_process();
		status do_res_multipart();
		status do_res_finalize();
		status do_res_send();
		
		void setup_req_recv();
		void setup_req_query();
		void setup_res_process();
		void setup_res_multipart();
		void setup_res_finalize();
		void setup_res_send();
		
		void setup_send_error();
		
		std::shared_ptr<connection> con;
		stream_reader sr;
		stream_writer sw;
		
		std::vector<char> general_buffer;
		
	};
}
