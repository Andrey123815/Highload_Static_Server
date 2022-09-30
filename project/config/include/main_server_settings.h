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

private:
    int port;
    std::string root;
    std::string config_path;
};