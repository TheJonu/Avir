#ifndef AVIR_SCAN_H
#define AVIR_SCAN_H

#include <iostream>
#include <ctime>
#include <chrono>

#include <boost/filesystem.hpp>

namespace Scan {
    enum scan_scope {
        scope_file, scope_directory_linear, scope_directory_recursive
    };
    enum scan_method {
        method_local, method_online
    };
    enum scan_status {
        status_just_started, status_in_progress, status_completed, status_terminated
    };
    enum file_state {
        state_not_readable, state_safe, state_not_safe
    };

    // results of scanning a single file
    struct file_scan_result {
        boost::filesystem::path path;
        std::string hash;
        file_state state;
    };

    // scan data
    class scan {
    public:
        // types
        scan_scope scope;
        scan_method method;
        bool printUnreadable;
        // paths
        boost::filesystem::path scanPath;
        boost::filesystem::path quarantinePath;
        std::vector<boost::filesystem::path> filePaths;
        std::vector<boost::filesystem::path> hashListPaths;
        std::vector<boost::filesystem::path> reportPaths;
        // status
        scan_status status;
        std::vector<file_scan_result> results;
        std::vector<file_scan_result> unsafeResults;
        std::vector<file_scan_result> unreadableResults;
        std::vector<file_scan_result> safeResults;
        // time
        std::time_t startTime;
        std::chrono::duration<double> elapsedSeconds;
    };

    void begin(scan scan);
}

#endif
