#include "http.hh"

blueshift::http::request::~request() {
	if (hbuf) free(hbuf);
}

static char * next_token(char * tok, char * max_tok) { // max_tok is always assumed to point to a null terminator, the last one of a char buffer
	do { tok++; } while (*tok != '\0');
	do { tok++; } while (tok < max_tok && *tok == '\0');
	return tok;
}

static void tokenize_line(char * tok, char * max_tok, char c) {
	for (; tok < max_tok && *tok != '\0'; tok++) if (*tok == c) *tok = '\0';
}

void blueshift::http::request::parse(char const * buf, size_t buf_len) {
	
	header_copy = {buf, buf_len};
	
	hbuf_len = buf_len;
	hbuf = (char *) realloc(hbuf, hbuf_len);
	memcpy(hbuf, buf, hbuf_len);
	
	method = nullptr;
	http_version = nullptr;
	path = nullptr;
	
	char * cur = hbuf, * cur2 = nullptr;
	char * const max_cur = cur + hbuf_len - 1;
	
	status = status_code::internal_server_error;
	
	for (size_t i = 0; i < hbuf_len; i++) {
		if (hbuf[i] == '\r') hbuf[i] = '\0';
		else if (hbuf[i] == '\n') hbuf[i] = '\0';
	}
	
	if (*max_cur != '\0') { // the protocol does not send a header unless it ends with \\r\\n\\r\\n, and those are replaced with \\0 above, so this should never happen
		status = status_code::internal_server_error;
		return;
	}
	
	tokenize_line(cur, max_cur, ' ');
	
	method = cur;
	if (strcmp(cur, "GET")) {
		status = status_code::method_not_allowed;
		return;
	}
	
	cur = next_token(cur, max_cur);
	if (cur == max_cur) {
		status = status_code::bad_request;
		return;
	}
	path = cur;
	
	cur = next_token(cur, max_cur);
	if (cur == max_cur) {
		status = status_code::bad_request;
		return;
	}
	http_version = cur;
	if (strcmp(cur, "HTTP/1.1")) {
		status = status_code::http_version_not_supported;
		return;
	}
	
	status = status_code::ok;
	
	cur2 = cur;
	while (true) {
		cur = next_token(cur2, max_cur); 
		if (cur == max_cur) break;
		char * sct = strchr(cur, ':');
		if (sct == nullptr || *(sct + 1) != ' ') {
			status = status_code::bad_request;
			return;
		}
		*sct = '\0';
		*(sct + 1) = '\0';
		cur2 = next_token(cur, max_cur);
		if (cur == max_cur) {
			status = status_code::bad_request;
			return;
		}
		fields[cur] = cur2;
	}
}
