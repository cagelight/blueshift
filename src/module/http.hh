#pragma once

#include "file.hh"
#include "istring.hh"

#include <unordered_map>
#include <vector>

namespace blueshift {
	
	struct protocol;
	
	namespace http {
		
		typedef std::unordered_map<istring, std::string, ihash, std::equal_to<istring>> field_map;
		typedef std::unordered_map<std::string, std::string> arg_map;
		
		static std::string const default_mime {"application/octet-stream"};
		
		std::string urldecode(std::string const &);
		
		typedef uint_fast16_t status_code_ut;
		enum struct status_code : status_code_ut {
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
		
		char const * text_for_status(status_code stat);
		
		struct request_header {
			
			std::string method;
			std::string version;
			std::string path;
			
			arg_map arguments;
			field_map fields;
			
			bool is_multipart = false;
			std::string multipart_type;
			std::string multipart_delimiter;
			
			void clear();
			status_code parse_from(std::vector<char> const &);
			std::vector<char> serialize();
			
			inline std::string const & field(char const * f) const {
				auto i = fields.find(f);
				if (i != fields.end()) return i->second;
				return empty_str;
			}
			
			inline std::string const & argument(char const * f) const {
				auto i = arguments.find(f);
				if (i != arguments.end()) return i->second;
				return empty_str;
			}
			
			inline size_t content_length() const {
				auto cli = fields.find("Content-Length");
				if (cli != fields.end()) return strtoul(cli->second.c_str(), nullptr, 10);
				return 0;
			}
			
			inline std::string const & content_type() const {
				auto cti = fields.find("Content-Type");
				if (cti != fields.end()) return cti->second;
				return default_mime;
			}
		};
		
		struct response_header {
			
			std::string version;
			status_code code;
			field_map fields;
			
			void clear();
			bool parse_from(std::vector<char> const &);
			std::vector<char> serialize();
			
			inline std::string const & field(char const * f) const {
				auto i = fields.find(f);
				if (i != fields.end()) return i->second;
				return empty_str;
			}
			
			inline size_t content_length() const {
				auto cli = fields.find("Content-Length");
				if (cli != fields.end()) return strtoul(cli->second.c_str(), nullptr, 10);
				return 0;
			}
			
			inline std::string const & content_type() const {
				auto cti = fields.find("Content-Type");
				if (cti != fields.end()) return cti->second;
				return default_mime;
			}
			
		};
		
		struct multipart_header {

			std::string disposition;
			std::string name;
			std::string filename;
			field_map fields;
			
			void clear();
			bool parse_from(std::vector<char> const &);
			
			inline char const * content_type() const {
				auto cti = fields.find("Content-Type");
				if (cti != fields.end()) return cti->second.c_str();
				return nullptr;
			}
			
		};
	}
}
