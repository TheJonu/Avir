#include <iostream>
#include <string>

#include <boost/filesystem.hpp>

#include "scan.h"

using namespace std;
using namespace boost::filesystem;

void print_usage()
{
    cout << "Usage: avir [type] [path]" << endl;
    cout << "  -f <filepath>: scan a single file" << endl;
    cout << "  -dl <dirpath>: scan a directory linearly" << endl;
    cout << "  -dr <dirpath>: scan a directory recursively" << endl;
}

int main(int argc, char *argv[])
{
    if(argc <= 2){
        print_usage();
        return 0;
    }

    string arg1 = argv[1];
    string arg2 = argv[2];

    Scan::scan scan;

    if(arg1 == "-f"){
        if(!is_regular_file(arg2)){
            cout << "That's not a file.";
            return 1;
        }
        scan.scanType = Scan::file_scan;
    }
    else if(arg1 == "-dl"){
        if(!is_directory(arg2)){
            cout << "That's not a directory." << endl;
            return 1;
        }
        scan.scanType = Scan::dir_linear_scan;
    }
    else if(arg1 == "-dr"){
        if(!is_directory(arg2)){
            cout << "That's not a directory." << endl;
            return 1;
        }
        scan.scanType = Scan::dir_recursive_scan;
    }
    else{
        cout << "Wrong usage." << endl;
        print_usage();
    }

    scan.scanPath = canonical(arg2);
    scan.begin();
}