#pragma once

#include <unistd.h>
#include <vector>
#include <string>
#include <map>

#include "client_connection.h"
#include "static_server_log.h"
#include "define_log.h"

class WorkerProcess {
public:
    explicit WorkerProcess(int listen_sock, std::string staticRoot/*, std::vector<StaticServerLog*>& vector_logs*/);

    void run();

    ~WorkerProcess() = default;

    void setup_signal_handlers();

    static void sigint_handler(int sig);

//    void write_to_logs(std::string message, bl::trivial::severity_level lvl);

private:
//    typedef enum {
//        INFO_STOP_DONE
//    } log_messages_t;

    std::map<int, class ClientConnection> client_connections;

    int listen_sock;
    std::string staticRoot;
//    std::vector<StaticServerLog*> vector_logs;

//    void write_to_log(log_messages_t log_type);
};
