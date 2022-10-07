#include "main_server_settings.h"

MainServerSettings::MainServerSettings(std::string config_path)
    : config_path(config_path) {
    read_config();
}

void MainServerSettings::read_config() {
    std::fstream file(this->config_path,
    std::ios::binary | std::ios::in);

    if (file.is_open()) {
        std::string line;
        getline(file, line);
        this->port = std::atoi(std::string(line.begin() + line.find(' ') + 1, line.end()).c_str());
        line.clear();
        getline(file, line);
        this->root = std::string(line.begin() + line.find(' ') + 1, line.end());
        line.clear();
        getline(file, line);
        this->count_worker_process = std::atoi(std::string(line.begin() + line.find(' ') + 1, line.end()).c_str());
        line.clear();
    }
}

int MainServerSettings::get_port() {
    return this->port;
}

std::string MainServerSettings::get_root() {
    return this->root;
}

int MainServerSettings::get_count_workers() {
    return this->count_worker_process;
}
