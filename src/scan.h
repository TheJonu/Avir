#ifndef AVIR_SCAN_H
#define AVIR_SCAN_H

#include <iostream>
#include <ctime>
#include <boost/filesystem.hpp>

namespace Scan {
    enum scan_type {
        type_file, type_directory_linear, type_directory_recursive
    };

    enum scan_status {
        status_just_started, status_in_progress, status_completed, status_terminated
    };

    enum file_state {
        state_not_readable, state_safe, state_not_safe
    };

    struct file_scan_result {
        boost::filesystem::path path;
        std::string hash;
        file_state state;
    };

    struct scan {
        scan_type type;
        bool online;
        bool unreadable;
        std::vector<boost::filesystem::path> filePaths;
        boost::filesystem::path scanPath;
        std::vector<boost::filesystem::path> hashBasePaths;
        std::vector<boost::filesystem::path> outputPaths;
        scan_status status;
        std::vector<file_scan_result> results;
        std::vector<file_scan_result> unsafeResults;
        std::vector<file_scan_result> unreadableResults;
        std::vector<file_scan_result> safeResults;
        std::vector<std::string> hashBase;
        std::time_t startTime;
        std::chrono::duration<double> elapsedSeconds;
    };

    /*
    class scan {
    public:
        scan_type type;
        bool online;
        std::vector<boost::filesystem::path> filePaths;
        boost::filesystem::path scanPath;
        std::vector<boost::filesystem::path> hashBasePaths;
        std::vector<boost::filesystem::path> outputPaths;
    public:
        scan_status status;
        std::vector<file_scan_result> results;
        std::vector<file_scan_result> unsafeResults;
        std::vector<file_scan_result> unreadableResults;
        std::vector<file_scan_result> safeResults;
        std::vector<std::string> hashBase;
        std::time_t startTime;
        std::chrono::duration<double> elapsedSeconds;
    public:
        scan();
    };
     */

    void begin(scan scan);
}

#endif
