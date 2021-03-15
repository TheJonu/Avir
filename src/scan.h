#ifndef AVIR_SCAN_H
#define AVIR_SCAN_H

#include <iostream>
#include <ctime>

#include <boost/filesystem.hpp>

namespace Scan
{
    enum scan_type{
        file_scan, dir_linear_scan, dir_recursive_scan
    };

    enum file_state{
        is_not_readable, is_safe, is_not_safe
    };

    struct file_scan_result{
        boost::filesystem::path path;
        std::string hash;
        file_state state;
    };

    class scan{
    public:

        scan_type scanType;
        boost::filesystem::path scanPath;
        boost::filesystem::path outputPath;

        scan();
        void begin();

    private:

        //std::tm startTime;
        //std::tm endTime;

        std::vector<boost::filesystem::path> files;
        std::vector<file_scan_result> fileScanResults;

        void find_file();
        void find_files_linear();
        void find_files_recursive();

        std::string execute(const char* cmd);
        file_scan_result scan_file(const boost::filesystem::path& path);
    };
}

#endif
