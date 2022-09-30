#include <cstring>
#include <sstream>

#include "http_exceptions.h"
#include "http_request.h"
#include "string_to_lower.h"

std::string& HttpRequest::get_method() {
    return method_;
}

std::string HttpRequest::get_method() const {
    return std::string(method_);
}

std::string& HttpRequest::get_url() {
    return url_;
}

std::string HttpRequest::get_url() const {
    return std::string(url_);
}

HttpRequest::HttpRequest(const std::string& str) {
    size_t lf_pos = str.find('\n');
    if (lf_pos == std::string::npos) {
        throw DelimException("Line feed not found");
    }
    size_t start_pos = 0;
    size_t end_pos = str.find(' ');
    if (end_pos == std::string::npos) {
        throw DelimException("Space not found");
    }
    method_ = std::string(str, start_pos, end_pos - start_pos);

    start_pos = end_pos + 1;
    while (str[start_pos] == ' ') {
        start_pos++;
    }
    end_pos = str.find(' ', start_pos);
    if (end_pos == std::string::npos) {
        throw DelimException("Space not found");
    }
    url_ = std::string(str, start_pos, end_pos - start_pos);
//    this->url_without_query_params_ = erase_query_params(this->url_);

    start_pos = end_pos + 1;
    while (str[start_pos] == ' ') {
        start_pos++;
    }

    std::string protocol(str, start_pos, lf_pos - 1 - start_pos);
    if (sscanf(protocol.c_str(), "HTTP/%d.%d", &version_major_, &version_minor_) != 2) {
        throw ReadException("Error while reading from file descriptor");
    }

    start_pos = lf_pos + 1;
    while (true) {
        lf_pos = str.find('\n', start_pos);
        if (lf_pos == std::string::npos || lf_pos == start_pos || str[start_pos] == '\r') {
            break;
        }
        size_t colon_pos = str.find(':', start_pos);
        std::string header_name(str, start_pos, colon_pos - start_pos);
        if (colon_pos == std::string::npos) {
            throw DelimException("Colon not found");
        }
        string_to_lower(header_name);

        start_pos = colon_pos + 1;
        while (str[start_pos] == ' ') {
            start_pos++;
        }
        std::string header_value(str, start_pos, lf_pos - 1 - start_pos);
        string_to_lower(header_value);
        headers_[header_name] = header_value;
        start_pos = lf_pos + 1;
    }
}

void HttpRequest::add_line(const std::string& line) {
    if (!first_line_added_) {
        add_first_line(line);
        return;
    }
    if (!headers_read_) {
        add_header(line);
        return;
    }
    request_ended_ = true;
}

void HttpRequest::add_first_line(const std::string& line) {
    size_t lf_pos = line.find('\n');
    if (lf_pos == std::string::npos) {
        throw DelimException("Line feed not found");
    }
    size_t start_pos = 0;
    size_t end_pos = line.find(' ');
    if (end_pos == std::string::npos) {
        throw DelimException("Space not found");
    }
    method_ = std::string(line, start_pos, end_pos - start_pos);

    start_pos = end_pos + 1;
    while (line[start_pos] == ' ') {
        start_pos++;
    }
    end_pos = line.find(' ', start_pos);
    if (end_pos == std::string::npos) {
        throw DelimException("Space not found");
    }
    url_ = std::string(line, start_pos, end_pos - start_pos);

    start_pos = end_pos + 1;
    while (line[start_pos] == ' ') {
        start_pos++;
    }

    std::string protocol(line, start_pos, lf_pos - 1 - start_pos);
    if (sscanf(protocol.c_str(), "HTTP/%d.%d", &version_major_, &version_minor_) != 2) {
        throw ReadException("Error while reading from file descriptor");
    }
    first_line_added_ = true;
}

void HttpRequest::add_header(const std::string& line) {
    size_t start_pos = 0;
    size_t lf_pos;
    lf_pos = line.find('\n', start_pos);
    if (lf_pos == std::string::npos) {
        throw DelimException("Line feed not found");
    }
    if (lf_pos == start_pos || line[start_pos] == '\r') {
        headers_read_ = true;
        return;
    }
    size_t colon_pos = line.find(':', start_pos);
    std::string header_name(line, start_pos, colon_pos - start_pos);
    if (colon_pos == std::string::npos) {
        throw DelimException("Colon not found");
    }
    string_to_lower(header_name);

    start_pos = colon_pos + 1;
    while (line[start_pos] == ' ') {
        start_pos++;
    }
    std::string header_value(line, start_pos, lf_pos - 1 - start_pos);
    string_to_lower(header_value);
    headers_[header_name] = header_value;
}

bool HttpRequest::first_line_added() const {
    return first_line_added_;
}

bool HttpRequest::requst_ended() const {
    return request_ended_;
}
