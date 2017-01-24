#pragma once

#include "socket.hh"
#include "http.hh"
#include "module.hh"
#include "util.hh"

#include <vector>

namespace blueshift {
	
	struct protocol {
		
		enum struct status : uint_fast8_t {
			idle,
			progress,
			terminate,
		};
		
		protocol(std::unique_ptr<connection> && conin, module::interface & mi);
		~protocol();
		
		status update();
		
	private:
		
		module::interface & mi;
		
		http::request_header req {};
		http::response_header res {};
		
		module::request_query reqq {};
		module::response_query resq {};
		
		enum struct mode_e {
			req_recv,
			req_query,
			proxy,
			res_process,
			res_finalize,
			res_send,
		} mode = mode_e::req_recv;
		
		status do_req_recv();
		status do_req_query();
		status do_proxy();
		status do_res_process();
		status do_res_finalize();
		status do_res_send();
		
		std::unique_ptr<connection> con;
		stream_reader sr;
		stream_writer sw;
		
		std::vector<char> general_buffer;
		
	};
	
	/*
	class protocol {
	public:
		
		enum struct status : uint_fast8_t {
			idle,
			progress,
			terminate,
		};
		
		protocol(std::unique_ptr<connection> && conin, server_request_handler h);
		~protocol();
		
		status update();
		
	private:
		
		static constexpr size_t work_buf_len = 512;
		static thread_local char work_buf [work_buf_len];
		
		char * overflow = nullptr;
		size_t overflow_size;
		size_t overflow_ind = 0;
		
		char * proxy_buf = nullptr;
		ssize_t proxy_bufi = 0, proxy_ind = 0;
		
		int8_t head_delimiter_counter = 0;
		
		union {
			size_t write_ind;
			off_t writefile_offs;
		};
		
		http::request req;
		std::vector<char> header;
		
		http::response res;
		std::string res_head;
		
		ssize_t read(char * buf, size_t buf_len);
		
		status update_req_recv();
		status update_res_calc();
		status update_res_send_head();
		status update_res_send_body();
		status update_proxy();
		
		void setup_update_req_recv();
		void setup_update_res_calc();
		void setup_update_res_send_head();
		void setup_update_res_send_body();
		void setup_update_proxy();
		
		enum struct state : uint_fast8_t {
			req_recv,
			res_calc,
			res_send_head,
			res_send_body,
			proxy,
		};
		
		state u_state = state::req_recv;
		bool res_calc_continue;
		
		std::unique_ptr<connection> con;
		
		blueshift::future_connection * proxyfuture = nullptr;
		std::unique_ptr<connection> proxycon;
		
		server_request_handler srh;
		void * token;
	};
	*/
}
