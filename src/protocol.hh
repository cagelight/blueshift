#pragma once

#include "socket.hh"

#include <vector>
#include <unordered_map>

#include <experimental/string_view> // TODO -- string_view was accepted into C++17, but was not available at the time of writing, switch when available

namespace blueshift {
	
	namespace http {
		
		enum struct status : uint_fast16_t {
			// 1XX
			continue_s = 100,
			switching_protocols = 101,
			processing = 102,
			
			// 2XX
			ok = 200,
			created = 201,
			accepted = 202,
			non_authorative_information = 203,
			no_content = 204,
			reset_content = 205,
			partial_content = 206,
			multi_status = 207,
			already_reported = 208,
			im_used = 226,
			
			// 3XX
			multiple_choices = 300,
			moved_permanently = 301,
			found = 302,
			see_other = 303,
			not_modified = 304,
			use_proxy = 305,
			switch_proxy = 306,
			temporary_redirect = 307,
			permanent_redirect = 308,
			
			// 4XX
			bad_request = 400,
			unauthorized = 401,
			payment_required = 402,
			forbidden = 403,
			not_found = 404,
			method_not_allowed = 405,
			not_acceptable = 406,
			proxy_authentication_required = 407,
			request_timeout = 408,
			conflict = 409,
			gone = 410,
			length_required = 411,
			precondition_failed = 412,
			payload_too_large = 413,
			uri_too_long = 414,
			unsupported_media_type = 415,
			range_not_satisfiable = 416,
			expectation_failed = 417,
			im_a_teapot = 418,
			misdirected_request = 421,
			unprocessable_entity = 422,
			locked = 423,
			failed_dependency = 424,
			upgrade_required = 426,
			precondition_required = 428,
			too_many_requests = 429,
			request_header_fields_too_large = 431,
			unavailable_for_legal_reasons = 451,
			
			// 5XX
			internal_server_error = 500,
			not_implemented = 501,
			bad_gateway = 502,
			service_unavailable = 503,
			gateway_timeout = 504,
			http_version_not_supported = 505,
			variant_also_negotiates = 506,
			insufficient_storage = 507,
			loop_detected = 508,
			not_extended = 510,
			network_authentication_required = 511,
		};
		
		char const * text_for_status(status stat);
		
		class request {
		public:
			
			request() = default;
			~request();
			
			void set_buffer(char const * buf, size_t buf_len);
			void parse();
			
			status get_status () { return status_; }
			
		private:
			
			status status_;
			
			char const * method;
			char const * http_version;
			char const * path;
			
			std::unordered_map<std::experimental::string_view, char const *> fields;
			
			char * hbuf = nullptr;
			size_t hbuf_len = 0;
			
		};
		
	}
	
	class protocol {
	public:
		
		enum struct status : uint_fast8_t {
			idle,
			progress,
			terminate,
		};
		
		protocol(std::unique_ptr<connection> && conin);
		~protocol() = default;
		
		status update();
		
	private:
		
		static constexpr size_t work_buf_len = 512;
		static thread_local char work_buf [work_buf_len];
		
		std::vector<char> header;
		http::request req;
		
		status update_req_recv();
		
		status update_res_calc();
		
		status update_res_send();
		
		enum struct state : uint_fast8_t {
			req_recv,
			res_calc,
			res_send
		};
		
		state u_state = state::req_recv;
		
		std::unique_ptr<connection> con;
	};
	
}
