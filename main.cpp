#include <iostream>
#include <istream>
#include <string>
#include <iomanip>

#include <openssl/md5.h>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem.hpp>


using namespace std;
using namespace boost::iostreams;
using namespace boost::filesystem;


string md5_from_file(const string& path)
{
    unsigned char result[MD5_DIGEST_LENGTH];
    mapped_file_source src(path);
    MD5((unsigned char*)src.data(), src.size(), result);

    ostringstream sout;
    sout << hex << setfill('0');
    for(auto c: result) sout << setw(2) << (int)c;

    return sout.str();
}

int main(int argc, char *argv[])
{
    path dir_path = "../in";
    directory_iterator end_dir_i;

    for(directory_iterator dir_i(dir_path); dir_i != end_dir_i; ++dir_i)
    {
        if (is_regular_file(dir_i -> path()))
        {
            string file = dir_i -> path().string();
            string hash = md5_from_file(file);

            cout << endl;
            cout << "File: " << file << endl;
            cout << "Hash: " << hash << endl;
        }
    }
}