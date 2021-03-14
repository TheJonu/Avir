#include <iostream>
#include <iomanip>

#include <openssl/md5.h>
#include <boost/iostreams/device/mapped_file.hpp>

namespace Hash
{
    std::string md5(const std::string& file_path)
    {
        unsigned char result[MD5_DIGEST_LENGTH];

        try{
            boost::iostreams::mapped_file_source src(file_path);
            MD5((unsigned char*)src.data(), src.size(), result);

            std::ostringstream sout;
            sout << std::hex << std::setfill('0');
            for(auto c: result) sout << std::setw(2) << (int)c;

            return sout.str();
        }
        catch(const std::exception&) {
            return "";
        }
    }
}