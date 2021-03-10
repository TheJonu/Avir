#include <iostream>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem.hpp>

#include "hash.h"

using namespace std;
using namespace boost::iostreams;
using namespace boost::filesystem;

int main(int argc, char *argv[])
{
    path dir_path = "../in";
    directory_iterator end_dir_i;

    for(directory_iterator dir_i(dir_path); dir_i != end_dir_i; ++dir_i)
    {
        if (is_regular_file(dir_i -> path()))
        {
            string file = dir_i -> path().string();
            string hash = md5_of_file(file);

            cout << endl;
            cout << "File: " << file << endl;
            cout << "Hash: " << hash << endl;
        }
    }

    cout << "Safe travels! Press anything to exit...";
    getchar();
}