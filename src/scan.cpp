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
    path hashBasePath;
    path resultsPath;

    vector<string> hashBase;

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

    // check file safety online // true if safe
    bool check_online(string & hash)
    {
        string requestStr = "whois -h hash.cymru.com " + hash;
        string response = execute(&requestStr[0]);
        return response.find("NO_DATA") != string::npos;
    }

    // check file safety locally // true if safe
    bool check_local(string & fileHash)
    {
        for(const string& hash : hashBase){
            if(hash == fileHash){
                return false;
            }
        }
        return true;
    }

    // load hash base file
    void load_hash_base()
    {
        std::ifstream inStream(hashBasePath.string());
        string hash;

        while(getline(inStream, hash)){
            hashBase.push_back(hash);
        }
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
        // start clock

        auto start = std::chrono::system_clock::now();

        // determine directories

        char const *home = getenv("HOME");

        string homeString(home);
        string avirString = homeString + "/Avir";

        string hashBaseString = avirString + "/hashbase.txt";
        string resultsString = avirString + "/results";

        create_directories(avirString);
        create_directories(resultsString);

        hashBasePath = hashBaseString;
        resultsPath = resultsString;

        // load local hash base

        load_hash_base();

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
        //int safe_count = 0;
        vector<file_scan_result> unsafeResults;
        vector<file_scan_result> unreadableResults;
        vector<file_scan_result> safeResults;

        for(auto & result : results){
            switch(result.state){
                case is_not_readable:
                    unreadableResults.push_back(result);
                    break;
                case is_not_safe:
                    unsafeResults.push_back(result);
                    break;
                case is_safe:
                    //safe_count++;
                    safeResults.push_back(result);
                    break;
            }
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

        // if no output specified, create default

        if(outputPath.empty()){
            outputPath = resultsPath.string() + "/scan_" + to_string(start_time) + ".txt";
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
        outStream << "  Unsafe: \t" << unsafeResults.size() << endl;
        outStream << "  Unreadable: \t" << unreadableResults.size() << endl;
        outStream << "  Safe: \t" << safeResults.size() << endl;

        if(!unsafeResults.empty()){
            outStream << " --- " << endl;
            outStream << "UNSAFE FILES: \t" << endl;
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

        if(!unreadableResults.empty()){
            outStream << " --- " << endl;
            outStream << "Safe files: \t" << endl;
            for(auto & r : safeResults){
                outStream << "  " << r.path.string() << endl;
            }
        }

        outStream << endl;
        outStream.close();
    }
}