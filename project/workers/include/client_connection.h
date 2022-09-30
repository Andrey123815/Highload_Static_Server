#pragma once

#include <string>
#include <ctime>
#include <filesystem>

#include "http_request.h"
#include "static_server_log.h"
#include "define_log.h"

typedef enum {
    CONNECTION_PROCESSING,
    CONNECTION_TIMEOUT_ERROR,
    CONNECTION_FINISHED,
    ERROR_WHILE_CONNECTION_PROCESSING
} connection_status_t;

class ClientConnection {
public:
    ClientConnection(int sock, std::string &staticRoot, std::vector<StaticServerLog*>& vector_logs);

    ClientConnection &operator=(const ClientConnection &other) = default;

    ClientConnection(const ClientConnection &other) = default;

    ClientConnection() = default;

    connection_status_t connection_processing();

    clock_t get_timeout();

    std::string url_decode(std::string url);

    std::string erase_query_params(std::string url);

    std::string get_location(std::string path, bool &is_forbidden);

    void write_to_logs(std::string message, bl::trivial::severity_level lvl);

//    bool set_searching_index_file = false;
//    bool is_forbidden = false;
private:
    int sock;
    clock_t timeout;
    std::string staticRoot;
    std::vector<StaticServerLog*> vector_logs;

    typedef enum {
        GET_REQUEST,
        ROOT_FOUND,
        ROOT_NOT_FOUND,
        SEND_HTTP_HEADER_RESPONSE,
        SEND_FILE,
        BAD_REQUEST,
        ERROR_STAGE
    } connection_stages_t;

    typedef enum {
        NONE,
        GET,
        HEAD,
        POST
    } methods_t;

    typedef enum {
        INFO_NEW_CONNECTION,
        INFO_CONNECTION_FINISHED,
        ERROR_404_NOT_FOUND,
        ERROR_TIMEOUT,
        ERROR_READING_REQUEST,
        ERROR_SEND_RESPONSE,
        ERROR_SEND_FILE,
        ERROR_BAD_REQUEST
    } log_messages_t;

    connection_stages_t stage = GET_REQUEST;

    methods_t method = NONE;

    char last_char_;
    std::string line_;
    HttpRequest request_;
    std::string location_;
    std::string response;

    bool search_index_file = false;
    bool is_forbidden_status = false;
    bool is_url_dir = false;

    int response_pos = 0;
    int file_fd;

    // return true if their connection processing stage is finished
    bool get_request();
    connection_stages_t process_location(/*bool &is_forbidden*/);
    int make_response_header(bool root_found);

    bool send_http_header_response();

    bool send_file();

    bool is_end_request();  // check if the line_ has ended

    void set_method();

    void message_to_log(log_messages_t log_type, std::string url = "", std::string method = "");
};
