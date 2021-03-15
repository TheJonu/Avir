#include <iostream>
#include <chrono>
#include <ctime>

#include <boost/filesystem.hpp>

#include "scan.h"
#include "hash.h"

using namespace std;
using namespace boost::filesystem;

namespace Scan
{
    scan::scan(){

    }

    void scan::begin()
    {
        switch(scanType){
            case file_scan:
                find_file();
                break;
            case dir_linear_scan:
                cout << "Searching for files linearly..." << endl;
                find_files_linear();
                break;
            case dir_recursive_scan:
                cout << "Searching for files recursively..." << endl;
                find_files_recursive();
                break;
        }

        if(scanType == dir_linear_scan || scanType == dir_recursive_scan){
            if(files.size() == 0){
                cout << "Found no files at " << scanPath << endl;
            }

            cout << "Found " << files.size() << " files at " << scanPath << endl;

            string yn;
            cout << "Do you want to continue? [Y/n] ";
            cin >> yn;
            if(yn != "y" && yn != "Y"){
                return;
            }
        }
        cout << endl;

        // hide the window here

        auto start = std::chrono::system_clock::now();

        int fileCount = files.size();
        int i = 1;
        for(auto & file : files){
            cout << i << "/" << fileCount << "\t" << file.string() << endl;
            fileScanResults.push_back(scan_file(file.string()));
            i++;
        }

        int resultCount = fileScanResults.size();
        int unreadable_count = 0;
        int safe_count = 0;
        vector<file_scan_result> unsafeResults;

        for(auto & result : fileScanResults){
            switch(result.state){
                case is_not_readable:
                    unreadable_count++;
                    break;
                case is_safe:
                    safe_count++;
                    break;
                case is_not_safe:
                    unsafeResults.push_back(result);
                    break;
            }
        }

        auto end = std::chrono::system_clock::now();

        time_t start_time = chrono::system_clock::to_time_t(start);
        time_t end_time = chrono::system_clock::to_time_t(end);
        chrono::duration<double> elapsed_seconds = end-start;

        cout << endl;
        cout << "File count: \t" << resultCount << endl;
        cout << "   Safe: \t" << safe_count << endl;
        cout << "   Unsafe: \t" << unsafeResults.size() << endl;
        cout << "   Unreadable: \t" << unreadable_count << endl;

        if(outputPath.empty()){
            return;
        }

        cout << "Output saved to " << outputPath << endl;

        boost::filesystem::ofstream outStream(outputPath);

        outStream << "AVIR SCAN REPORT" << endl;
        outStream << " --- " << endl;
        outStream << "Start time: \t" << ctime(&start_time);
        outStream << "End time: \t" << ctime(&end_time);
        outStream << "Elapsed: \t" << elapsed_seconds.count() << " seconds" << endl;
        outStream << " --- " << endl;
        outStream << "File count: \t" << resultCount << endl;
        outStream << "  Safe: \t" << safe_count << endl;
        outStream << "  Unsafe: \t" << unsafeResults.size() << endl;
        outStream << "  Unreadable: \t" << unreadable_count << endl;
    }

    void scan::find_file()
    {
        if(is_regular_file(scanPath)){
            files.push_back(scanPath);
        }
    }

    void scan::find_files_linear()
    {
        for(directory_iterator iterator(scanPath); iterator != directory_iterator(); ++iterator)
        {
            path path = iterator -> path();
            if (is_regular_file(path))
            {
                files.push_back(path);
            }
        }
    }

    void scan::find_files_recursive()
    {
        for(recursive_directory_iterator iterator(scanPath); iterator != recursive_directory_iterator(); ++iterator)
        {
            path path = iterator -> path();
            if (is_regular_file(path))
            {
                files.push_back(path);
            }
        }
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

    file_scan_result scan::scan_file(const path& path)
    {
        file_scan_result result;

        result.path = path;

        result.hash = Hash::md5(path.string());

        if(result.hash.empty()){
            result.state = is_not_readable;
            return result;
        }

        string requestStr = "whois -h hash.cymru.com " + result.hash;
        const char* request = &requestStr[0];

        string response = execute(request);

        if(response.find("NO_DATA") != string::npos){
            result.state = is_safe;
        }
        else{
            result.state = is_not_safe;
        }

        return result;
    }
}