#include "module.hh"
#include "module_helpers.hh"
#include <algorithm>

#include "json.hh"

void blueshift::module::request_query::set_token(void * token) {
	processing_token = token;
}

void blueshift::module::request_query::refuse_payload() {
	q = qe::refuse_payload;
}

void blueshift::module::request_query::reset() {
	q = qe::ok;
	processing_token = nullptr;
}

void blueshift::module::response_query::set_body(std::vector<char> const & buf, char const * MIMEi) {
	body_buffer = buf;
	MIME = MIMEi ? MIMEi : http::default_mime;
	q = qe::body_buffer;
}

void blueshift::module::response_query::set_body(std::vector<char> && buf, char const * MIMEi) {
	body_buffer = std::move(buf);
	MIME = MIMEi ? MIMEi : http::default_mime;
	q = qe::body_buffer;
}

void blueshift::module::response_query::set_body(shared_file const & fil, char const * MIMEi) {
	body_file = fil;
	MIME = MIMEi ? MIMEi : body_file->get_MIME();
	q = qe::body_file;
}

void blueshift::module::response_query::set_body(std::string const & str, char const * MIMEi) {
	body_buffer = {str.begin(), str.end()};
	MIME = MIMEi ? MIMEi : "text/plain";
	q = qe::body_buffer;
}

void blueshift::module::response_query::set_body(char const * str, char const * MIMEi) {
	body_buffer = {str, str + strlen(str)};
	MIME = MIMEi ? MIMEi : "text/plain";
	q = qe::body_buffer;
}

void blueshift::module::response_query::reset() {
	body_buffer.clear();
	body_buffer.shrink_to_fit();
	body_file.reset();
	MIME.clear();
	q = qe::no_body;
}

bool blueshift::module::serve_static_file(http::request_header const & req, http::response_header & res, module::response_query & resq, std::string const & root, bool directory_listing) {
	
	json j;
	
	j["test"] = json::num(42);
	j["test2"] = json::str("TEEEST");
	j["test3"] = json::ary({ json::num(100200), json::str("LOLWUT") });
	j["test4"] = json::map();
	j["test4"]->map["subtest1"] = json::str("this is not a drill");
	
	print(j.serialize());
	
	std::string file_path = root + req.path;
	
	shared_file f = file::open(file_path.c_str());
	if (f->get_type() == file::type::file) {
	
		time::point req_date {req.field("If-None-Match")};
		time::point file_date = f->get_last_modified();
		
		res.fields["ETag"] = file_date.generate_etag();
		res.fields["Cache-Control"] = "max-age=1800, public, must-revalidate";
		
		if (req_date == file_date) {
			res.code = http::status_code::not_modified;
		} else {
			resq.set_body(f);
		}
		
		return true;
	
	} else if (f->get_type() == file::type::directory && directory_listing) {
		
		std::string body { "<head><meta charset=\"UTF-8\"><link rel=\"stylesheet\" type=\"text/css\" href=\"/__bluedlcss\"></head><body>" };
		
		std::string dlstr = "/" + req.path + "/";
		strops::remove_duplicates(dlstr, '/');
		body += "<h1>Directory Listing for " + dlstr + "</h1>";
		
		body += "<div class=\"mc\"><table><th>Type</th><th>Filename</th>";
		std::vector<blueshift::directory_listing> dls = f->get_files();
		std::sort(dls.begin(), dls.end(), directory_listing::natsortcmp_s {});
		for (blueshift::directory_listing const & dl : dls) {
			body += "<tr><td>";
			switch (dl.type_) {
				case file::type::file:
					body += "F</td><td>";
					break;
				case file::type::directory:
					body += "D</td><td>";
					break;
				default:
					body += "U</td><td>";
					break;
			}
			std::string p = "/" + req.path + "/" + dl.name;
			strops::remove_duplicates(p, '/');
			body += "<a href=\"" + p + "\">" + dl.name + "</a>";
			body += "</td></tr>";
		}
		body += "</table></div>";
		
		body += "</body>";
		
		resq.set_body(std::move(body), "text/html");
		return true;
		
	} else if (directory_listing) {
		
		std::string csst = req.path;
		strops::trim(csst, '/');
		if (csst == "__bluedlcss") {
			res.fields["Cache-Control"] = "max-age=2592000, public, immutable";
			std::string bdlcss {
R"BLUEDLCSS(

body {
	text-align:center;
	background-color: #224;
	color: #FFF;
}

a {
	text-decoration: none;
}

a:visited {
	color: #FFF;
}

a:link {
	color: #FFF;
}

a:hover {
	color: #FFF;
	text-shadow:0px 0px 5px #FFF;
}

a:active {
	color: #FFF;
	text-shadow:0px 0px 5px #FFF;
}

.mc {
	display: inline-block; 
	background-color: #222;
	padding: 20px;
	border-radius: 40px;
}

)BLUEDLCSS"
			};
			resq.set_body(bdlcss, "text/css");
			return true;
		}
	}
	
	return false;
}
