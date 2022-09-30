#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <csignal>
#include <boost/log/trivial.hpp>

#include "worker_process.h"
#include "client_connection.h"

#define EPOLL_SIZE 1024
#define EPOLL_RUN_TIMEOUT -1
#define MAX_METHOD_LENGTH 4
#define CLIENT_SEC_TIMEOUT 5 // maximum request idle time

extern bool is_stop = false;

WorkerProcess::WorkerProcess(int listen_sock, std::string staticRoot, std::vector<StaticServerLog*>& vector_logs) :
        listen_sock(listen_sock), staticRoot(staticRoot), vector_logs(vector_logs) {
    signal(SIGPIPE, SIG_IGN);
    this->setup_signal_handlers();
}

void WorkerProcess::run() {
    static struct epoll_event ev, events[EPOLL_SIZE];
    ev.events = EPOLLIN | EPOLLET;

    int epoll_fd = epoll_create(EPOLL_SIZE);
    ev.data.fd = this->listen_sock;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->listen_sock, &ev);

    int client, epoll_events_count;

    while (!is_stop) {
        epoll_events_count = epoll_wait(epoll_fd, events, EPOLL_SIZE, EPOLL_RUN_TIMEOUT);
        for (int i = 0; i < epoll_events_count; ++i) {
            if (events[i].data.fd == this->listen_sock) {
                client = accept(this->listen_sock, NULL, NULL);
                fcntl(client, F_SETFL, fcntl(client, F_GETFL, 0) | O_NONBLOCK);
                ev.data.fd = client;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client, &ev);
                this->client_connections[client] = ClientConnection(client, this->staticRoot, vector_logs);
            } else {  // if the event happened on a client socket
                connection_status_t connection_status = this->client_connections[events[i].data.fd].connection_processing();
                if (connection_status == CONNECTION_FINISHED || connection_status == CONNECTION_TIMEOUT_ERROR ||
                    connection_status == ERROR_WHILE_CONNECTION_PROCESSING) {
                    this->client_connections.erase(events[i].data.fd);
                    close(events[i].data.fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
                }
            }
        }
    }

    exit(0);
}

void WorkerProcess::sigint_handler(int sig) {
    is_stop = true;
}

void WorkerProcess::setup_signal_handlers() {
    struct sigaction act{};
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = sigint_handler;
    sigaction(SIGINT, &act, nullptr);
}

void WorkerProcess::write_to_logs(const std::string message, bl::trivial::severity_level lvl) {
    for (auto i : vector_logs) {
        i->log(message, lvl);
    }
}
