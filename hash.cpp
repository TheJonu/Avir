#include <iostream>
#include <iomanip>

#include <openssl/md5.h>
#include <boost/iostreams/device/mapped_file.hpp>

#include "hash.h"

std::string md5_of_file(const std::string& path)
{
    unsigned char result[MD5_DIGEST_LENGTH];

    try{
        boost::iostreams::mapped_file_source src(path);
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