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

namespace Scan {

    // executes a command in another process
    string execute(const char *cmd) {
        array<char, 128> buffer{};
        string result;

        unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);

        if (!pipe) throw runtime_error("popen() failed!");

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    // check if file is safe option_online
    bool check_if_safe_online(string &fileHash) {
        string requestStr = "whois -h fileHash.cymru.com " + fileHash;
        string response = execute(&requestStr[0]);
        return response.find("NO_DATA") != string::npos;
    }

    // check if file is safe in local hash base
    bool check_if_safe_locally(string &fileHash, vector<string> &hashBase) {
        for (const string &hash : hashBase) {
            if (hash == fileHash) {
                return false;
            }
        }
        return true;
    }

    // load hash base file
    vector<string> load_hash_base(const vector<path> &paths) {
        vector<string> hashBase;

        for (const path &path : paths) {
            std::ifstream inStream(path.string());
            string hash;

            while (getline(inStream, hash)) {
                hashBase.push_back(hash);
            }
        }

        return hashBase;
    }

    // scans a single file
    file_scan_result scan_file(path &filePath, vector<string> &hashBase, bool check_online) {
        file_scan_result result;

        result.path = filePath;

        result.hash = Hash::md5(filePath.string());

        if (result.hash.empty()) {
            result.state = state_not_readable;
            return result;
        }

        bool safe = true;

        if (!check_if_safe_locally(result.hash, hashBase))
            safe = false;

        if (check_online && !check_if_safe_online(result.hash))
            safe = false;

        if (safe)
            result.state = state_safe;
        else
            result.state = state_not_safe;

        return result;
    }

    string get_scan_type_name(scan_type scanType) {
        switch (scanType) {
            case type_file:
                return "single file scan";
            case type_directory_linear:
                return "linear directory scan";
            case type_directory_recursive:
                return "recursive directory scan";
        }
        return "";
    }

    string get_scan_status_name(scan_status scanStatus, uint resultsFile, uint filesCount) {
        switch (scanStatus) {
            case status_just_started:
                return "just started";
            case status_in_progress: {
                return "in progress (" + to_string(resultsFile / filesCount * 100) + "%)";
            };
            case status_completed:
                return "completed";
        }
        return "";
    }

    void print_result_to_files(scan& scan) {
        stringstream resultStringStream;

        resultStringStream << "AVIR SCAN REPORT" << endl;
        resultStringStream << " --- " << endl;
        resultStringStream << "Start time: \t" << ctime(&scan.startTime);
        resultStringStream << "Elapsed: \t" << scan.elapsedSeconds.count() << " seconds" << endl;
        resultStringStream << "Status: \t" << get_scan_status_name(scan.status, scan.results.size(), scan.filePaths.size()) << endl;
        resultStringStream << " --- " << endl;
        resultStringStream << "Scan path: \t" << scan.scanPath.string() << endl;
        resultStringStream << "Scan type: \t" << get_scan_type_name(scan.type) << endl;
        resultStringStream << " --- " << endl;
        resultStringStream << "File count: \t" << scan.results.size() << endl;
        resultStringStream << "  Unsafe: \t" << scan.unsafeResults.size() << endl;
        resultStringStream << "  Unreadable: \t" << scan.unreadableResults.size() << endl;
        resultStringStream << "  Safe: \t" << scan.safeResults.size() << endl;

        if (!scan.unsafeResults.empty()) {
            resultStringStream << " --- " << endl;
            resultStringStream << "UNSAFE FILES: \t" << endl;
            for (auto &r : scan.unsafeResults) {
                resultStringStream << "  " << r.path.string() << endl;
            }
        }

        if (!scan.unreadableResults.empty()) {
            resultStringStream << " --- " << endl;
            resultStringStream << "Unreadable files: \t" << endl;
            for (auto &r : scan.unreadableResults) {
                resultStringStream << "  " << r.path.string() << endl;
            }
        }

        resultStringStream << endl;

        for (const path &outputPath : scan.outputPaths) {
            boost::filesystem::ofstream outStream(outputPath);
            outStream << resultStringStream.str();
        }
    }

    // main scan function
    void begin(scan scan) {
        // start clock

        auto start = std::chrono::system_clock::now();
        scan.startTime = chrono::system_clock::to_time_t(start);

        // load local hash base

        auto hashBase = load_hash_base(scan.hashBasePaths);

        // print result at beginning

        scan.status = status_just_started;
        print_result_to_files(scan);

        // begin scanning

        scan.status = status_in_progress;
        scan.results.reserve(scan.filePaths.size());

        // scan option_online
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

        for (auto &file : scan.filePaths) {

            file_scan_result result = scan_file(file, hashBase, false);

            scan.results.push_back(result);

            switch (result.state) {
                case state_not_readable:
                    scan.unreadableResults.push_back(result);
                    break;
                case state_not_safe:
                    scan.unsafeResults.push_back(result);
                    break;
                case state_safe:
                    scan.safeResults.push_back(result);
                    break;
            }
        }

        // print result at the end

        auto end = std::chrono::system_clock::now();
        scan.elapsedSeconds = end - start;

        scan.status = status_completed;

        print_result_to_files(scan);
    }
}