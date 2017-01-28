#include "util.hh"

#define STREAM_BUFFER_SIZE 4096
static thread_local char stream_buffer [STREAM_BUFFER_SIZE];

blueshift::stream_reader::stream_reader(blueshift::connection& con) : con(con) { }

blueshift::stream_reader::status blueshift::stream_reader::read() {
	ssize_t c = con.read(stream_buffer, STREAM_BUFFER_SIZE);
	if (c < 0) return status::failure;
	if (c) {
		data.insert(data.end(), stream_buffer, stream_buffer + c);
		recalculate_delimiters(); // TODO -- optimize
	}
	if (counter_lim) {
		counter_cur -= c;
		if (counter_cur <= 0) {
			return status::counter_hit;
		}
	}
	if (delimited_position) return status::delimiter_detected;
	if (c == 0) return status::none_read;
	return status::some_read;
}

void blueshift::stream_reader::set_counter(size_t limit) {
	counter_lim = limit;
	counter_cur = limit;
	size_t s = data.size();
	if (s) {
		counter_cur -= s;
	}
}

void blueshift::stream_reader::set_delimiter(std::string const & dstr) {
	delimiter = {dstr.begin(), dstr.end()};
	recalculate_delimiters();
}

std::vector<char> blueshift::stream_reader::get_data(size_t num) {
	std::vector<char> r {data.begin(), data.begin() + num};
	data.erase(data.begin(), data.begin() + num);
	recalculate_delimiters();
	if (counter_lim) counter_lim -= num;
	return r;
}

std::vector<char> blueshift::stream_reader::get_all_data(bool avoid_potential_delimiters) {
	std::vector<char> r {};
 	if (!avoid_potential_delimiters || !delimiter_counter) {
		r = data;
		data.clear();
		data.shrink_to_fit();
		recalculate_delimiters();
 	} else {
 		r = {data.begin(), data.end() - delimiter_counter};
		data.erase(data.begin(), data.end() - delimiter_counter);
		recalculate_delimiters();
 	}
 	if (counter_lim) counter_lim -= r.size();
	return r;
}

std::vector<char> blueshift::stream_reader::get_delimited_data(bool include_delimiter) {
	if (counter_lim) { srcprintf_debug("attempted to get delimited data during a data counting session, this is undefined behavior"); }
	if (!delimited_position) srcthrow("attempted to get delimited data before delimiter found");
	std::vector<char> r {data.begin(), data.begin() + (include_delimiter ? delimited_position : delimited_position - delimiter.size())};
	data.erase(data.begin(), data.begin() + delimited_position);
	recalculate_delimiters();
	return r;
}

std::vector<char> blueshift::stream_reader::get_counted_data() {
	if (counter_cur > 0) srcthrow("attempted to get counted data before data was acquired");
	std::vector<char> r {data.begin(), data.begin() + counter_lim};
	data.erase(data.begin(), data.begin() + counter_lim);
	recalculate_delimiters();
	counter_lim = 0;
	counter_cur = 0;
	return r;
}


void blueshift::stream_reader::recalculate_delimiters() {
	delimiter_counter = 0;
	delimited_position = 0;
	
	if (!delimiter.size()) return;
	
	for (size_t i = 0; i < data.size(); i++) {
		if (data[i] == delimiter[delimiter_counter]) delimiter_counter++; else {
			if (data[i] == delimiter[0]) delimiter_counter = 1;
			else delimiter_counter = 0;
		}
		if (delimiter_counter == delimiter.size()) {delimited_position = i + 1; break;};
	}
}

// ================================================================

blueshift::stream_writer::stream_writer(blueshift::connection& con) : con(con) { }

blueshift::stream_writer::status blueshift::stream_writer::write() {
	ssize_t c;
	
	if (header.size()) {
		c = con.write(header.data(), header.size());
		if (c < 0) return status::failure;
		if (c == 0) return status::none_wrote;
		header.erase(header.begin(), header.begin() + c);
		if (!header.size() && mode == mode_e::none) {
			return status::complete;
		}
		return status::some_wrote;
	}
	
	switch (mode) {
		case mode_e::none:
			return status::complete; 
		case mode_e::data:
			c = con.write(data.data(), data.size());
			if (c < 0) return status::failure;
			if (c == 0) return status::none_wrote;
			data.erase(data.begin(), data.begin() + c);
			if (!data.size()) {
				mode = mode_e::none;
				return status::complete;
			}
			return status::some_wrote;
		case mode_e::file:
			c = con.sendfile(file->get_FD(), &file_offs, file_size);
			if (c < 0) return status::failure;
			if (c == 0) return status::none_wrote;
			if (file_offs == static_cast<off_t>(file_size)) {
				file_offs = 0;
				file_size = 0;
				mode = mode_e::none;
				file.reset();
				return status::complete;
			}
			return status::some_wrote;
	}
	
	throw general_exception {"stream_writer somehow got invalid mode"};
}

void blueshift::stream_writer::set_header(std::vector<char> && buf) {
	header = std::move(buf);
}

void blueshift::stream_writer::set(shared_file f) {
	if (mode == mode_e::data) srcthrow("attempted to set file in buffer mode");
	mode = mode_e::file;
	file = f;
	file_offs = 0;
	file_size = file->get_size();
}

void blueshift::stream_writer::set(shared_file f, off_t starting_offs, size_t ending_offs) {
	if (mode == mode_e::data) srcthrow("attempted to set file in buffer mode");
	mode = mode_e::file;
	file = f;
	file_offs = starting_offs;
	file_size = ending_offs;
}

void blueshift::stream_writer::set(std::vector<char> && buf) {
	if (!buf.size()) return;
	if (mode == mode_e::file) srcthrow("attempted to set buffer data in file mode");
	if (data.size()) srcthrow("attempted to set buffer when buffer was not empty");
	mode = mode_e::data;
	data = std::move(buf);
}

void blueshift::stream_writer::add(std::vector<char> const & buf) {
	if (!buf.size()) return;
	if (mode == mode_e::file) srcthrow("attempted to set buffer data in file mode");
	mode = mode_e::data;
	data.insert(data.end(), buf.begin(), buf.end());
}

