#ifndef LOGGER_H
#define LOGGER_H
#include <string>
#include <filesystem>
#include <ctime>
#include <fstream>
class Logger
{
public:
    Logger(std::string logfile, std::string dbname, std::string benchmark, std::string dataset)
    {
        time_t now = time(0);
        struct tm *timestamp;
        timestamp = localtime(&now);
        char timestamp_buf[25];
        strftime(timestamp_buf, 25, "%F_%H%M%S", timestamp);

        std::filesystem::path logpath = "./results/" + dbname + "/" + dataset + "/" + benchmark;
        _logpath = logpath.c_str();
        std::filesystem::create_directories(logpath);

        _logfile = logpath.string() + "/" + logfile + std::string(timestamp_buf);
        _log.open(_logfile, std::fstream::out | std::fstream::app);
    }
    ~Logger()
    {
        _log.close();
        _logfile = nullptr;
        _logpath = nullptr;
    }
    void out(std::string msg)
    {
        _log << msg << "\n"; //TODO: override this funciton with formatting?
    }

private:
    std::string _logfile;
    std::string _logpath;
    std::fstream _log;
};

#endif