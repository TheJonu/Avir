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

    enum scan_status{
        just_started, in_progress, completed
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
    private:
        void print_result();
        std::string get_scan_type_name() const;
        std::string get_scan_status_name() const;
    public:
        scan_type scanType;
        std::vector<boost::filesystem::path> filePaths;
        boost::filesystem::path scanPath;
        boost::filesystem::path hashbasePath;
        boost::filesystem::path outputPath;
        scan();
        void begin();
    };
}

#endif
