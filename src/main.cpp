#include <iostream>
#include <string>
#include <unistd.h>

#include <boost/filesystem.hpp>

#include "scan.h"

using namespace std;
using namespace boost::filesystem;

void print_usage()
{
    cout << "Usage: avir [type] [option]" << endl;
    cout << " Types:" << endl;
    cout << "  -f  <path>:  scan a single file" << endl;
    cout << "  -dl <path>:  scan a directory linearly" << endl;
    cout << "  -dr <path>:  scan a directory recursively" << endl;
    cout << " Options:" << endl;
    cout << "  -o  <file>:  create output file" << endl;
}

int main(int argc, char *argv[])
{
    if(argc == 1){
        print_usage();
        return 0;
    }

    if(argc % 2 == 0){
        cout << "Wrong usage." << endl;
        print_usage();
        return 0;
    }

    string arg1 = argv[1];
    string arg2 = argv[2];

    Scan::scan scan;

    if(arg1 == "-f"){
        if(!is_regular_file(arg2)){
            cout << "That's not a file." << endl;
            return 0;
        }
        scan.scanType = Scan::file_scan;
    }
    else if(arg1 == "-dl"){
        if(!is_directory(arg2)){
            cout << "That's not a directory." << endl;
            return 0;
        }
        scan.scanType = Scan::dir_linear_scan;
    }
    else if(arg1 == "-dr"){
        if(!is_directory(arg2)){
            cout << "That's not a directory." << endl;
            return 0;
        }
        scan.scanType = Scan::dir_recursive_scan;
    }
    else{
        cout << "Wrong type." << endl;
        print_usage();
        return 0;
    }

    if(argc >= 5){
        string arg3 = argv[3];
        string arg4 = argv[4];

        if(arg3 == "-o"){
            std::ofstream file {arg4};
            if(access(arg4.c_str(), F_OK) != -1){
                scan.outputPath = canonical(arg4);
            }
        }
        else{
            cout << "Wrong option." << endl;
            print_usage();
            return 0;
        }
    }

    scan.scanPath = canonical(arg2);
    scan.begin();
}