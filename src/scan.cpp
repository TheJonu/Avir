#include <iostream>

#include <boost/filesystem.hpp>

#include "scan.h"
#include "hash.h"

using namespace std;
using namespace boost::filesystem;

namespace Scan
{
    scan::scan(){

    }

    void scan::begin(){

        switch(scanType){
            case file_scan:
                begin_scan_file();
                break;
            case dir_linear_scan:
                begin_scan_dir_linear();
                break;
            case dir_recursive_scan:
                begin_scan_dir_recursive();
                break;
        }
    }

    bool scan::ask_to_continue()
    {
        string yn;
        cout << "Do you want to continue? [Y/n] ";
        cin >> yn;
        if(yn == "y" || yn == "Y"){
            return true;
        }
        return false;
    }

    string scan::execute(const char* cmd)
    {
        array<char, 128> buffer;
        string result;
        unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);

        if(!pipe) throw runtime_error("popen() failed!");

        while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr){
            result += buffer.data();
        }
        return result;
    }

    void scan::begin_scan_dir_linear()
    {
        int file_count = 0;
        for(directory_iterator iterator(scanPath); iterator != directory_iterator(); ++iterator)
        {
            if (is_regular_file(iterator -> path()))
            {
                file_count++;
            }
        }
        cout << "Found " << file_count << " files at " << scanPath.string() << endl;

        if(!ask_to_continue())
            return;

        int number = 0;
        for(directory_iterator iterator(scanPath); iterator != directory_iterator(); ++iterator)
        {
            if (is_regular_file(iterator -> path()))
            {
                cout << "--- " << ++number << "/" << file_count << " --------------------------------" << endl;
                scan_file(iterator -> path());
            }
        }
    }

    void scan::begin_scan_dir_recursive()
    {
        int file_count = 0;
        for(recursive_directory_iterator iterator(scanPath); iterator != recursive_directory_iterator(); ++iterator)
        {
            if (is_regular_file(iterator -> path()))
            {
                file_count++;
            }
        }
        cout << "Found " << file_count << " files at " << scanPath.string() << endl;

        if(!ask_to_continue())
            return;

        int number = 0;
        for(recursive_directory_iterator iterator(scanPath); iterator != recursive_directory_iterator(); ++iterator)
        {
            if (is_regular_file(iterator -> path()))
            {
                cout << "--- " << ++number << "/" << file_count << " --------------------------------" << endl;
                scan_file(iterator -> path());
            }
        }
    }

    void scan::begin_scan_file()
    {
        scan_file(scanPath);
    }

    bool scan::scan_file(const path& path)
    {
        cout << "File: \t" << path.filename().string() << endl;

        cout << "Path: \t" << path.parent_path().string() << endl;

        string hash = Hash::md5(path.string());
        cout << "Hash: \t" << hash << endl;

        cout << "Safe: \t";
        string requestStr = "whois -h hash.cymru.com " + hash;
        const char* request = &requestStr[0];
        string response = execute(request);
        int pos = response.find("NO_DATA");
        if(pos != string::npos){
            cout << "yes" << endl;
            return false;
        }
        else{
            cout << "NOT SAFE - MALWARE DETECTED" << endl;
            return true;
        }
    }
}