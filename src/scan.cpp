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

    //vector<string> hashBase;

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

    // check if file is safe online
    bool check_if_safe_online(string & fileHash)
    {
        string requestStr = "whois -h fileHash.cymru.com " + fileHash;
        string response = execute(&requestStr[0]);
        return response.find("NO_DATA") != string::npos;
    }

    // check if file is safe in local hash base
    bool check_if_safe_locally(string & fileHash, vector<string>& hashBase)
    {
        for(const string& hash : hashBase){
            if(hash == fileHash){
                return false;
            }
        }
        return true;
    }

    // load hash base file
    vector<string> load_hash_base(const vector<path>& paths)
    {
        vector<string> hashBase;

        for(const path& path : paths){
            std::ifstream inStream(path.string());
            string hash;

            while(getline(inStream, hash)){
                hashBase.push_back(hash);
            }
        }

        return hashBase;
    }

    // scans a single file
    file_scan_result scan_file(path& filePath, vector<string>& hashBase, bool check_online)
    {
        file_scan_result result;

        result.path = filePath;

        result.hash = Hash::md5(filePath.string());

        if(result.hash.empty()){
            result.state = is_not_readable;
            return result;
        }

        bool safe = true;

        if(!check_if_safe_locally(result.hash, hashBase))
            safe = false;

        if(check_online && !check_if_safe_online(result.hash))
            safe = false;

        if (safe)
            result.state = is_safe;
        else
            result.state = is_not_safe;

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

    void scan::print_result_to_files()
    {
        stringstream resultStringStream;

        resultStringStream << "AVIR SCAN REPORT" << endl;
        resultStringStream << " --- " << endl;
        resultStringStream << "Start time: \t" << ctime(&start_time);
        resultStringStream << "Elapsed: \t" << elapsed_seconds.count() << " seconds" << endl;
        resultStringStream << "Status: \t" << get_scan_status_name() << endl;
        resultStringStream << " --- " << endl;
        resultStringStream << "Scan path: \t" << scanPath.string() << endl;
        resultStringStream << "Scan type: \t" << get_scan_type_name() << endl;
        resultStringStream << " --- " << endl;
        resultStringStream << "File count: \t" << results.size() << endl;
        resultStringStream << "  Unsafe: \t" << unsafeResults.size() << endl;
        resultStringStream << "  Unreadable: \t" << unreadableResults.size() << endl;
        resultStringStream << "  Safe: \t" << safeResults.size() << endl;

        if(!unsafeResults.empty()){
            resultStringStream << " --- " << endl;
            resultStringStream << "UNSAFE FILES: \t" << endl;
            for(auto & r : unsafeResults){
                resultStringStream << "  " << r.path.string() << endl;
            }
        }

        if(!unreadableResults.empty()){
            resultStringStream << " --- " << endl;
            resultStringStream << "Unreadable files: \t" << endl;
            for(auto & r : unreadableResults){
                resultStringStream << "  " << r.path.string() << endl;
            }
        }

        resultStringStream << endl;

        for(const path& outputPath : outputPaths){
            boost::filesystem::ofstream outStream(outputPath);
            outStream << resultStringStream.str();
        }
    }

    // empty constructor
    scan::scan()= default;

    // main scan function
    void scan::begin()
    {
        // start clock

        auto start = std::chrono::system_clock::now();
        start_time = chrono::system_clock::to_time_t(start);

        // load local hash base

        auto hashBase = load_hash_base(hashBasePaths);

        // print result at beginning

        scanStatus = just_started;
        print_result_to_files();

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

            file_scan_result result = scan_file(file, hashBase, false);

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

        print_result_to_files();
    }
}