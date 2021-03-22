#include <iostream>
#include <chrono>
#include <ctime>

#include <boost/filesystem.hpp>
#include <future>

#include "scan.h"
#include "hash.h"

using namespace std;
using namespace boost::filesystem;

namespace Scan
{
    //const path HASH_BASE = "~/Avir/hashbase.txt";

    // executes a command in another process
    string execute(const char* cmd)
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

    // check file safety online
    bool check_online(string & hash)
    {
        string requestStr = "whois -h hash.cymru.com " + hash;
        string response = execute(&requestStr[0]);
        return response.find("NO_DATA") != string::npos;
    }

    // check file safety locally
    bool check_local(string & hash)
    {
        return true;
    }

    // scans a single file
    file_scan_result scan_file(const path& path)
    {
        file_scan_result result;

        result.path = path;

        result.hash = Hash::md5(path.string());

        if(result.hash.empty()){
            result.state = is_not_readable;
            return result;
        }

        if(check_online(result.hash) && check_local(result.hash)){
            result.state = is_safe;
        }
        else{
            result.state = is_not_safe;
        }

        return result;
    }

    // empty constructor
    scan::scan(){}

    // main scan function
    void scan::begin()
    {
        // start

        auto start = std::chrono::system_clock::now();

        // begin asynchronous scanning

        vector<file_scan_result> results;
        results.reserve(filePaths.size());

        vector<future<file_scan_result>> futures;
        futures.reserve(filePaths.size());

        for(auto & file : filePaths){
            futures.push_back(async(launch::async, scan_file, file));
        }

        for(auto & e : futures){
            results.push_back(e.get());
        }

        // scanning ended

        auto end = std::chrono::system_clock::now();

        // filter results

        int resultCount = results.size();
        int safe_count = 0;
        vector<file_scan_result> unreadableResults;
        vector<file_scan_result> unsafeResults;

        for(auto & result : results){
            switch(result.state){
                case is_not_readable:
                    unreadableResults.push_back(result);
                    break;
                case is_not_safe:
                    unsafeResults.push_back(result);
                    break;
                case is_safe:
                    safe_count++;
                    break;
            }
        }

        // return if no output specified

        if(outputPath.empty()){
            return;
        }

        // calculate stuff

        time_t start_time = chrono::system_clock::to_time_t(start);
        time_t end_time = chrono::system_clock::to_time_t(end);
        chrono::duration<double> elapsed_seconds = end-start;

        string scanTypeName;
        switch(scanType){
            case file_scan: scanTypeName = "single file scan"; break;
            case dir_linear_scan: scanTypeName = "linear directory scan"; break;
            case dir_recursive_scan: scanTypeName = "recursive directory scan"; break;
        }

        // generate output file

        boost::filesystem::ofstream outStream(outputPath);

        outStream << "AVIR SCAN REPORT" << endl;
        outStream << " --- " << endl;
        outStream << "Start time: \t" << ctime(&start_time);
        outStream << "End time: \t" << ctime(&end_time);
        outStream << "Elapsed: \t" << elapsed_seconds.count() << " seconds" << endl;
        outStream << " --- " << endl;
        outStream << "Scan path: \t" << scanPath.string() << endl;
        outStream << "Scan type: \t" << scanTypeName << endl;
        outStream << " --- " << endl;
        outStream << "File count: \t" << resultCount << endl;
        outStream << "  Safe: \t" << safe_count << endl;
        outStream << "  Unsafe: \t" << unsafeResults.size() << endl;
        outStream << "  Unreadable: \t" << unreadableResults.size() << endl;

        if(!unsafeResults.empty()){
            outStream << " --- " << endl;
            outStream << "Unsafe files: \t" << endl;
            for(auto & r : unsafeResults){
                outStream << "  " << r.path.string() << endl;
            }
        }

        if(!unreadableResults.empty()){
            outStream << " --- " << endl;
            outStream << "Unreadable files: \t" << endl;
            for(auto & r : unreadableResults){
                outStream << "  " << r.path.string() << endl;
            }
        }

        outStream << endl;
        outStream.close();
    }
}