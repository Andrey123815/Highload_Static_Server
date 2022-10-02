#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <exception>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <sys/stat.h>

#include "client_connection.h"
#include "http_request.h"
#include "http_response.h"
#include "http_handle.h"

#define MAX_METHOD_LENGTH 4
#define CLIENT_SEC_TIMEOUT 5 // maximum request idle time
#define PAGE_404 "public/404.html" // потом изменить
#define FOR_404_RESPONSE "/sadfsadf/sadfsaf/asdfsaddf"

ClientConnection::ClientConnection(int sock, std::string &staticRoot, std::vector<StaticServerLog*>& vector_logs) :
        sock(sock), staticRoot(staticRoot), vector_logs(vector_logs) {}

connection_status_t ClientConnection::connection_processing() {
    if (this->stage == ERROR_STAGE) {
        return ERROR_WHILE_CONNECTION_PROCESSING;
    }

    if (this->stage == GET_REQUEST) {
        if (!get_request() && clock() / CLOCKS_PER_SEC - this->timeout > CLIENT_SEC_TIMEOUT) {
            this->message_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
        try {
            if (last_char_ == '\n') {
                request_.add_line(line_);
                write_to_logs(request_.get_url(), INFO);
                line_.clear();
            }
        } catch (std::exception& e) {
            this->message_to_log(ERROR_BAD_REQUEST);
            stage = BAD_REQUEST;
        }
        if (request_.first_line_added()) {
            stage = process_location();
        }
    }

    write_to_logs("AFTER process location, stage: " + std::string(stage == ROOT_FOUND ? "ROOT_FOUND" : "other"), ERROR);

    int response_status = OK_STATUS;

//    if (stage == FORBIDDEN_REQUEST) {
//        write_to_logs("stage FORBIDDEN_REQUEST", INFO);
//        make_response_header(true, true);
//        response_status = FORBIDDEN_REQUEST_STATUS;
//        stage = SEND_HTTP_HEADER_RESPONSE;
//    }

    if (stage == ROOT_FOUND) {
        response_status = make_response_header(true);
        stage = SEND_HTTP_HEADER_RESPONSE;
    }
    if (stage == ROOT_NOT_FOUND) {
        write_to_logs("ROOT_NOT_FOUND stage", ERROR);
        response_status = make_response_header(false);
        stage = SEND_HTTP_HEADER_RESPONSE;
    }

    if (this->stage == SEND_HTTP_HEADER_RESPONSE) {
        write_to_logs("in stage SEND_HTTP_HEADER_RESPONSE", INFO);
        if (this->send_http_header_response()) {
            this->stage = SEND_FILE;
        } else if (clock() / CLOCKS_PER_SEC - this->timeout > CLIENT_SEC_TIMEOUT) {
            this->message_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    if (response_status == NOT_FOUND_STATUS /*|| response_status == FORBIDDEN_REQUEST_STATUS*/) {
        return CONNECTION_FINISHED;
    }

    if (request_.get_method() == HEAD_METHOD && this->stage == SEND_FILE) {
        return CONNECTION_FINISHED;
    }

    if (this->stage == SEND_FILE) {
        if (this->send_file()) {
            this->message_to_log(INFO_CONNECTION_FINISHED);
            return CONNECTION_FINISHED;
        } else if (clock() / CLOCKS_PER_SEC - this->timeout > CLIENT_SEC_TIMEOUT) {
            this->message_to_log(ERROR_TIMEOUT);
            return CONNECTION_TIMEOUT_ERROR;
        }
    }

    return CONNECTION_PROCESSING;
}

bool ClientConnection::get_request() {
    bool is_read_data = false;
    int result_read;
    while ((result_read = read(this->sock, &last_char_, sizeof(last_char_))) == sizeof(last_char_)) {
        this->line_.push_back(last_char_);
        if (this->line_.size() >= MAX_METHOD_LENGTH) {
            this->set_method();
        }
        is_read_data = true;
    }

    if (request_.requst_ended()) {
        return true;
    }

    if (result_read == -1) {
        this->stage = ERROR_STAGE;
        return false;
    }

    if (is_read_data) {
        this->timeout = clock() / CLOCKS_PER_SEC;
    }

    return false;
}

int ClientConnection::make_response_header(bool root_found) {
    if (root_found) {
        std::string res = erase_query_params(this->staticRoot + location_);
        this->file_fd = open(res.c_str(), O_RDONLY);
        struct stat file_stat;
        if (file_fd == NOT_OK || fstat(file_fd, &file_stat) == NOT_OK) {
//            std::ifstream iff("/http-test-suite111" + location_);
//            if (iff.bad()) {
//                write_to_logs("NOT EXISTS", ERROR);
//            } else {
//                write_to_logs("EXISTS", INFO);
//            }
            write_to_logs(std::string(this->search_index_file ? "true" : "false") + std::string(this->is_url_dir ? "true" : "false"), WARNING);
        }
//        write_to_logs("BEFORE HHTP HANDLER" + std::string(this->is_forbidden_status ? "true" : "false"), WARNING);
        this->response = http_handler(request_, res, this->is_forbidden_status).get_string();
        this->file_fd = open(res.c_str(), O_RDONLY);
        this->line_.clear();
        return this->file_fd == NOT_OK ? NOT_FOUND_STATUS : OK_STATUS;
    } else {
        this->file_fd = open(PAGE_404, O_RDONLY);
        this->response = http_handler(request_).get_string();
        this->message_to_log(ERROR_404_NOT_FOUND);
    }
    this->line_.clear();
//    close(this->file_fd);

    return NOT_FOUND_STATUS;
}

bool ClientConnection::send_http_header_response() {
    bool is_write_data = false;
    int write_result;
    while ((write_result = write(this->sock, this->response.c_str() + response_pos, 1)) == 1) {
        response_pos++;
        if (response_pos == this->response.size() - 1) {
            this->response.clear();
            write_result = write(this->sock, "\r\n", 2);
            if (write_result == -1) {
                this->stage = ERROR_STAGE;
                this->message_to_log(ERROR_SEND_RESPONSE);
                return false;
            }
            return true;
        }
        is_write_data = true;
    }

    if (write_result == -1) {
        this->stage = ERROR_STAGE;
        this->message_to_log(ERROR_SEND_RESPONSE);
        return false;
    }

    if (is_write_data) {
        this->timeout = clock() / CLOCKS_PER_SEC;
    }

    return false;
}

bool ClientConnection::send_file() {
    bool is_write_data = false;
    char c;
    int read_code;
    int write_result;

    read_code = read(this->file_fd, &c, sizeof(c));
    while ((write_result = write(this->sock, &c, sizeof(c)) == sizeof(c)) && read_code > 0) {
        read_code = read(this->file_fd, &c, sizeof(c));
        is_write_data = true;
    }

    if (write_result == -1) {
        this->stage = ERROR_STAGE;
        this->message_to_log(ERROR_SEND_FILE);
        return false;
    }

    if (read_code == 0) {
        return true;
    }

    if (is_write_data) {
        this->timeout = clock() / CLOCKS_PER_SEC;
    }

    return false;
}

void ClientConnection::set_method() {
    if (this->line_.substr(0, 3) == "GET") {
        this->method = GET;
    } else if (this->line_.substr(0, 4) == "HEAD") {
        this->method = HEAD;
    }
}

bool ClientConnection::is_end_request() {
    size_t pos = this->line_.find("\r\n\r\n");
    return pos != std::string::npos;
}

void ClientConnection::message_to_log(log_messages_t log_type, std::string url, std::string method) {
    switch (log_type) {
        case INFO_NEW_CONNECTION:
            this->write_to_logs("New connection [METHOD " + method + "] [URL "
                                    + url
                                    + "] [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET " + std::to_string(this->sock)
                                    + "]", INFO);
            break;
        case INFO_CONNECTION_FINISHED:
            this->write_to_logs("Connection finished successfully [WORKER PID " + std::to_string(getpid()) + "]" + " [CLIENT SOCKET "
                                    + std::to_string(this->sock) + "]", INFO);
            break;
        case ERROR_404_NOT_FOUND:
            this->write_to_logs("404 NOT FOUND [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
                                     + std::to_string(this->sock) + "]", ERROR);
            break;
        case ERROR_TIMEOUT:
            this->write_to_logs("TIMEOUT ERROR [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
                                    + std::to_string(this->sock) + "]", ERROR);
            break;
        case ERROR_READING_REQUEST:
            this->write_to_logs("Reading request error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
                                    + std::to_string(this->sock) + "]", ERROR);
            break;
        case ERROR_SEND_RESPONSE:
            this->write_to_logs("Send response error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
                                    + std::to_string(this->sock) + "]", ERROR);
            break;
        case ERROR_SEND_FILE:
            this->write_to_logs("Send file error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
                                    + std::to_string(this->sock) + "]", ERROR);
            break;
        case ERROR_BAD_REQUEST:
            this->write_to_logs("Bad request error [WORKER PID " + std::to_string(getpid()) + "] [CLIENT SOCKET "
                                    + std::to_string(this->sock) + "]", ERROR);
            break;
    }
}

clock_t ClientConnection::get_timeout() {
    return this->timeout;
}

std::string ClientConnection::url_decode(std::string url) {
    std::string new_url;
    char ch;
    int j;
    for (auto i = 0; i < url.size(); i++) {
        if (url[i] == '%') {
            sscanf(url.substr(i + 1, 2).c_str(), "%x", &j);
            ch = static_cast<char>(j);
            new_url += ch;
            i = i + 2;
        } else {
            new_url += url[i];
        }
    }

    return new_url;
}

std::string ClientConnection::erase_query_params(std::string url) {
    return url.substr(0, url.find('?'));
}

ClientConnection::connection_stages_t ClientConnection::process_location() {
    std::string url = request_.get_url();
//    write_to_logs("simple url: " + request_.get_url() + " new url: " + request_.get_url_without_query_params(), INFO);
    this->message_to_log(INFO_NEW_CONNECTION, url, request_.get_method());
    HttpResponse http_response;

    bool is_forbidden = false;

    location_ = get_location(url_decode(url), is_forbidden);

//    std::ifstream iff;
//    iff.open(location_);
//    write_to_logs("I AM TUTA " + location_ + " set_searching_index_file "+ std::string(this->set_searching_index_file ? "true" : "false"), DEBUG);
//    if (this->set_searching_index_file && !iff.is_open() && !this->search_index_file) {
//        write_to_logs("TUUUT FORBIDDEN", WARNING);
//        is_forbidden = true;
//    }

    this->is_forbidden_status = is_forbidden;
//    this->search_index_file = is_forbidden;

//    write_to_logs(std::string(is_forbidden ? "true" : "false") + std::string(this->is_url_dir ? "true" : "false"), WARNING);

    if (location_[location_.size() - 1] == '/') {
        return ROOT_NOT_FOUND;
    }

    return ROOT_FOUND;
}

void ClientConnection::write_to_logs(std::string message, bl::trivial::severity_level lvl) {
    for (auto i : vector_logs) {
        i->log(message, lvl);
    }
}

static bool containsRootEscaping(std::string path) {
    const std::string rootEscaping = "../";

    size_t pos = path.find(rootEscaping);
    if (pos != std::string::npos) return true;

    return false;
}

std::string ClientConnection::get_location(std::string path, bool &is_forbidden) {
    if (containsRootEscaping(path)) {
        is_forbidden = true;
    }

    if (path[path.size() - 1] == '/') {
        write_to_logs(path, WARNING);
        std::filesystem::path file(this->staticRoot + path);
        std::filesystem::path file1(this->staticRoot + path.substr(0, path.length() - 1));
//        if (std::filesystem::is_regular_file(file1)) {
//            write_to_logs("file1 is regular", INFO);
//            std::ifstream iff;
//            iff.open(path.substr(0, path.length() - 1));
////            if (!iff.is_open()) {
//            write_to_logs("TUUUT FORBIDDEN", WARNING);
//            is_forbidden = true;
//            return path;
////            }
////            return path;
//        }
        if (std::filesystem::is_directory(file)) {
            this->is_url_dir = true;
            bool no_files = true;

            for (auto const &dir_entry: std::filesystem::directory_iterator{file}) {
                std::string pth = dir_entry.path();
                std::string file_path = std::string(pth.begin() + pth.find_last_of('/') + 1, pth.end());
                if (!dir_entry.is_directory() && file_path == "index.html") {
                    no_files = false;
                }
            }

            if (no_files) {
                is_forbidden = true;
            }

//            std::ifstream iff(path + "index.html");
//            if (iff.bad()) {
//                is_forbidden = true;
//            }

//            std::ifstream iff;
//            iff.open(path + "index.html");
//            if(!iff) is_forbidden = true;

            this->search_index_file = true;
            return path + "index.html";
        }
//        std::ifstream iff;
//        iff.open(path + "index.html");
//        write_to_logs("try " + path + "index.html", INFO);
//        is_forbidden = true;
        this->search_index_file = true;
        return path  + "index.html";
    }

    return path;
}
