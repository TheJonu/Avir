#ifndef AVIR_SCAN_H
#define AVIR_SCAN_H

#include <iostream>

#include <boost/filesystem.hpp>

namespace Scan
{
    enum scan_type{
        file_scan, dir_linear_scan, dir_recursive_scan
    };

    struct file_scan_results{

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
        void begin_scan_file();
        void begin_scan_dir_linear();
        void begin_scan_dir_recursive();
        bool ask_to_continue();
        std::string execute(const char* cmd);
        bool scan_file(const boost::filesystem::path& path);
    };
}

#endif
