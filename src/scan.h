#ifndef AVIR_SCAN_H
#define AVIR_SCAN_H

#include <iostream>

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

    struct scan_results{

    };

    class scan{
    public:

        boost::filesystem::path scanPath;
        scan_type scanType;
        scan();
        void begin();

    private:

        std::vector<boost::filesystem::path> files;
        std::vector<file_scan_result> fileScanResults;

        void find_file();
        void find_files_linear();
        void find_files_recursive();

        void begin_scan_file();
        void begin_scan_dir_linear();
        void begin_scan_dir_recursive();

        bool ask_to_continue();
        std::string execute(const char* cmd);
        file_scan_result scan_file(const boost::filesystem::path& path);
    };
}

#endif
