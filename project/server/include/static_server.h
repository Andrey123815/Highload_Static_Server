#pragma once

#include "main_server_settings.h"
#include "worker_process.h"
#include "static_server_log.h"
#include "define_log.h"

#include <vector>
#include <string>
#include <fstream>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/attributes.hpp>
#include <sys/types.h>
#include <sys/wait.h>

typedef boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger_t;

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace attrs = boost::log::attributes;

extern int process_stop;
const std::string DEFAULT_CONFIG_PATH = "./settings/static_server.conf";

class StaticServer {
public:
    StaticServer();

    ~StaticServer() = default;

    int server_start();
        bl::trivial::severity_level cast_types_logs_level(std::string lvl);
        void write_to_logs(std::string message, bl::trivial::severity_level lvl);

        static int daemonize();
        bool bind_listen_sock();
        int add_work_processes();
        int fill_pid_file();
        int delete_pid_file();

    static int process_setup_signals();  // set handlers to signals
        static void sigint_handler(int sig);  // handler for stop

    int server_stop();

private:
    int count_workflows;

    std::vector<pid_t> workers_pid;

    std::string access_log_level = "debug";
    std::string error_log_level = "error";
    StaticServerLog error_log = StaticServerLog ("error", true, cast_types_logs_level(error_log_level));
    StaticServerLog access_log = StaticServerLog ("access", true, cast_types_logs_level(access_log_level));

    std::vector<StaticServerLog*> vector_logs;

    int listen_sock;

    MainServerSettings server_settings;
};
