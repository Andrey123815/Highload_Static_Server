#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <csignal>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "static_server.h"

#define BACKLOG 128


int process_stop = 0;

StaticServer::StaticServer() {
    this->server_settings = MainServerSettings(DEFAULT_CONFIG_PATH);
    this->count_workflows = this->server_settings.get_count_workers();
    vector_logs.push_back(&error_log);
    vector_logs.push_back(&access_log);
    write_to_logs("SERVER STARTING...", INFO);
}

bl::trivial::severity_level StaticServer::cast_types_logs_level(std::string lvl) {
    if (lvl == "info") {
        return INFO;
    }
    if (lvl == "debug") {
        return DEBUG;
    }
    if (lvl == "trace") {
        return TRACE;
    }
    return ERROR;
}

void StaticServer::write_to_logs(std::string message, bl::trivial::severity_level lvl) {
    for (auto i : vector_logs) {
        i->log(message, lvl);
    }
}

int StaticServer::daemonize() {
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    pid_t pid = fork();

    if (pid == -1) {
        return -1;
    }

    if (pid != 0) {  // Возвращается нуль процессу-потомку и пид чилда мастеру
        exit(0);
    }

    setsid();

    return 0;
}

int StaticServer::fill_pid_file() {
    std::ofstream stream_to_pid_file;
    stream_to_pid_file.open("pid_file.txt", std::ios::out);

    if (!stream_to_pid_file.is_open()) {
        return -1;
    }

    stream_to_pid_file << getpid() << std::endl;

    for (auto i : workers_pid) {
        stream_to_pid_file << i << std::endl;
    }

    stream_to_pid_file.close();

    return 0;
}

int StaticServer::delete_pid_file() {
    return remove("pid_file.txt");
}

int StaticServer::add_work_processes() {
    for (int i = 0; i < this->count_workflows; ++i) {
       pid_t pid = fork();
        if (pid == -1) {
           write_to_logs("ERROR FORK", ERROR);
           return -1;
        }
        if (pid != 0) {
           workers_pid.push_back(pid);
        } else {
           WorkerProcess worker(this->listen_sock, this->server_settings.get_root()/*, vector_logs*/);
           worker.run();
           break;
        }
    }

    return 0;
}

int StaticServer::server_start() {
    if (daemonize() != 0) {
        write_to_logs("ERROR IN SERVER DAEMONIZE", ERROR);
        return -1;
    }

    if (!this->bind_listen_sock()) {
        write_to_logs("ERROR IN BIND SOCKET", ERROR);
        return -1;
    }

    if (add_work_processes()!= 0) {
        write_to_logs("ERROR IN ADDING WORK PROCESSES", ERROR);
        return -1;
    }

    if (fill_pid_file() == -1) {
        write_to_logs("ERROR IN FILL PID FILE", ERROR);
        return -1;
    }

    write_to_logs("Worker processes (" + std::to_string(this->workers_pid.size()) + ") successfully started", INFO);

    process_setup_signals();  // установка нужных обработчиков сигналов

    while (true) {
        if (process_stop == 1) {
            server_stop();
            return 0;
        }
    }
}

void StaticServer::sigint_handler(int sig) {
    process_stop = 1;
}

int StaticServer::process_setup_signals() {
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = StaticServer::sigint_handler;
    sigaction(SIGINT, &act, nullptr);
    return 0;
}

int StaticServer::server_stop() {
    write_to_logs("SERVER STOP...", WARNING);
    close(this->listen_sock);

    int status;
    for (auto &i : this->workers_pid) {
        kill(i, SIGINT);
    }
    for (auto &i : this->workers_pid) {
        waitpid(i, &status, 0);
    }

    write_to_logs("SERVER STOPPED", INFO);
    delete_pid_file();
    exit(0);
}

bool StaticServer::bind_listen_sock() {
    this->listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (this->listen_sock == -1)
        return false;

    int enable = 1;
    if (setsockopt(this->listen_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
        return false;

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(this->server_settings.get_port());
    if (inet_aton("127.0.0.1", &addr.sin_addr) == -1)
        return false;

    if (bind(this->listen_sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
        return false;

    if (listen(this->listen_sock, BACKLOG) == -1) {
        return false;
    }

    return true;
}
