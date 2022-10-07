#pragma once

#include <string>
#include <iostream>
#include <fstream>

class MainServerSettings {
public:
    explicit MainServerSettings(std::string config_path);

    MainServerSettings() = default;

    void read_config();

    int get_port();
    std::string get_root();
    int get_count_workers();

private:
    int port;
    std::string root;
    int count_worker_process;
    std::string config_path;
};