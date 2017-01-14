#pragma once

#include "socket.hh"
#include "http.hh"
#include "module.hh"

#include <vector>

namespace blueshift {
	
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
		
		union {
			size_t write_ind;
			off_t writefile_offs;
		};
		ssize_t proxy_bufi;
		char proxy_buf [64];
		
		http::request req;
		std::vector<char> header;
		
		http::response res;
		std::string res_head;
		
		status update_req_recv();
		status update_res_calc();
		status update_res_send_head();
		status update_res_send_body();
		status update_proxy();
		
		void setup_update_req_recv();
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
	
}
