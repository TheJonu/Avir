#include <iostream>
#include <chrono>
#include <ctime>
#include <future>
#include <tgmath.h>

#include <boost/filesystem.hpp>

#include "scan.h"
#include "hash.h"

using namespace std;
using namespace boost::filesystem;

namespace Scan
{
    scan_status scanStatus;

    vector<file_scan_result> results;
    vector<file_scan_result> unsafeResults;
    vector<file_scan_result> unreadableResults;
    vector<file_scan_result> safeResults;

    vector<string> hashBase;

    time_t start_time;
    chrono::duration<double> elapsed_seconds;


    // executes a command in another process
    string execute(const char* cmd)
    {
        array<char, 128> buffer{};
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
    void load_hash_base(const path& path)
    {
        std::ifstream inStream(path.string());
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

        //bool onlineSafe = check_online(result.hash);
        //bool localSafe = check_local(result.hash);

        bool safe = check_local(result.hash);

        if(safe){
            result.state = is_safe;
        }
        else{
            result.state = is_not_safe;
        }

        return result;
    }

    string scan::get_scan_type_name() const
    {
        switch(scanType){
            case file_scan: return "single file scan";
            case dir_linear_scan: return "linear directory scan";
            case dir_recursive_scan: return "recursive directory scan";
        }
        return "";
    }

    string scan::get_scan_status_name() const
    {
        switch(scanStatus){
            case just_started: return "just started";
            case in_progress: {
                return "in progress (" + to_string(results.size() / filePaths.size() * 100) + "%)";
            };
            case completed: return "completed";
        }
        return "";
    }

    void scan::print_result()
    {
        boost::filesystem::ofstream outStream(outputPath);

        outStream << "AVIR SCAN REPORT" << endl;
        outStream << " --- " << endl;
        outStream << "Start time: \t" << ctime(&start_time);
        outStream << "Elapsed: \t" << elapsed_seconds.count() << " seconds" << endl;
        outStream << "Status: \t" << get_scan_status_name() << endl;
        outStream << " --- " << endl;
        outStream << "Scan path: \t" << scanPath.string() << endl;
        outStream << "Scan type: \t" << get_scan_type_name() << endl;
        outStream << " --- " << endl;
        outStream << "File count: \t" << results.size() << endl;
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

        /*
        if(!safeResults.empty()){
            outStream << " --- " << endl;
            outStream << "Safe files: \t" << endl;
            for(auto & r : safeResults){
                outStream << "  " << r.path.string() << endl;
            }
        }
         */

        outStream << endl;
        outStream.close();
    }

    // empty constructor
    scan::scan(){}

    // main scan function
    void scan::begin()
    {
        // start clock

        auto start = std::chrono::system_clock::now();
        start_time = chrono::system_clock::to_time_t(start);

        // load local hash base

        load_hash_base(hashbasePath);

        // print result at beginning

        scanStatus = just_started;
        print_result();

        // begin scanning

        scanStatus = in_progress;
        results.reserve(filePaths.size());

        // scan online
        /*
        vector<future<file_scan_result>> futures;
        futures.reserve(filePaths.size());

        for(auto & file : filePaths){
            futures.push_back(async(launch::async, scan_file, file));
        }

        double last_print_time = 0;

        for(auto & e : futures){
            file_scan_result result = e.get();
            results.push_back(result);

            auto now = std::chrono::system_clock::now();
            elapsed_seconds = now - start;

            if(elapsed_seconds.count() - last_print_time > 0.5f){
                cout << "Print result at " << elapsed_seconds.count() << " seconds";

                last_print_time = elapsed_seconds.count();

                switch(result.state){
                    case is_not_readable: unreadableResults.push_back(result); break;
                    case is_not_safe: unsafeResults.push_back(result); break;
                    case is_safe: safeResults.push_back(result); break;
                }

                print_result();
            }
        }
        */

        // scan locally

        for(auto & file : filePaths){

            file_scan_result result = scan_file(file);

            results.push_back(result);

            switch(result.state){
                case is_not_readable: unreadableResults.push_back(result); break;
                case is_not_safe: unsafeResults.push_back(result); break;
                case is_safe: safeResults.push_back(result); break;
            }
        }

        // print result at the end

        auto end = std::chrono::system_clock::now();
        elapsed_seconds = end - start;

        scanStatus = completed;

        print_result();
    }
}