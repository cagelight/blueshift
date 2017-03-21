#pragma once

#include "socket.hh"
#include "file.hh"

#include <vector>

namespace blueshift {
	
	struct stream_reader {
		
		enum struct status {
			some_read,
			none_read,
			delimiter_detected,
			failure,
			counter_hit,
		};
		
		stream_reader(std::shared_ptr<connection> con);
		
		status read();
		size_t size() const {return data.size();}
		std::vector<char> get_all_data(bool avoid_potential_delimiters = true);
		std::vector<char> get_delimited_data(bool include_delimiter = true);
		std::vector<char> get_counted_data();
		std::vector<char> get_data(size_t num);
		void set_counter(size_t limit);
		void set_delimiter(std::string const &);
		
		std::vector<char> const & peak() const { return data; }
		
		inline bool counter_is_set() const {
			return counter_lim != 0;
		}
		
	private:
		
		void recalculate_delimiters();
		
		std::shared_ptr<connection> con;
		std::vector<char> data {};
		std::vector<char> delimiter = {'\r', '\n', '\r', '\n'};
		size_t delimiter_counter = 0;
		size_t delimited_position = 0;
		
		ssize_t counter_lim = 0;
		ssize_t counter_cur = 0;
	};
	
	struct stream_writer {
		
		enum struct status {
			complete,
			some_wrote,
			none_wrote,
			failure
		};
		
		stream_writer(std::shared_ptr<connection> con);
		
		status write();
		void set_header(std::vector<char> &&);
		void set(std::vector<char> &&);
		void add(std::vector<char> const &);
		
		void set(shared_file f);
		void set(shared_file f, off_t starting_offs, size_t ending_offs);
		
	private:
		enum struct mode_e {
			none,
			data,
			file,
		} mode = mode_e::none;
		std::shared_ptr<connection> con;
		std::vector<char> header {};
		std::vector<char> data {};
		shared_file file = nullptr;
		off_t file_offs = 0;
		size_t file_size = 0;
	};
	
}
