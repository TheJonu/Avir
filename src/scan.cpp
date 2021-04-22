#include <iostream>
#include <chrono>
#include <ctime>
#include <future>
#include <csignal>
#include <iomanip>
#include <openssl/md5.h>
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include "scan.h"

using namespace std;
using namespace boost::filesystem;

namespace Scan {

    scan *globalScan; // used by termination signal handler

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

    std::string md5(const std::string& file_path)
    {
        unsigned char result[MD5_DIGEST_LENGTH];

        try{
            boost::iostreams::mapped_file_source src(file_path);
            MD5((unsigned char*)src.data(), src.size(), result);

            std::ostringstream sout;
            sout << std::hex << std::setfill('0');
            for(auto c: result) sout << std::setw(2) << (int)c;

            return sout.str();
        }
        catch(const std::exception&) {
            return "";
        }
    }

    // check if file is safe option_online
    bool check_if_safe_online(string &fileHash) {
        string requestStr = "whois -h hash.cymru.com " + fileHash;
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

    file_scan_result scan_file_locally(path &filePath, vector<string> &hashBase) {
        file_scan_result result;
        result.path = filePath;
        result.hash = md5(filePath.string());

        if (result.hash.empty()) {
            result.state = state_not_readable;
            return result;
        }

        if (check_if_safe_locally(result.hash, hashBase))
            result.state = state_safe;
        else
            result.state = state_not_safe;

        return result;
    }

    file_scan_result scan_file_online(const path& filePath){
        file_scan_result result;
        result.path = filePath;
        result.hash = md5(filePath.string());

        if (result.hash.empty()) {
            result.state = state_not_readable;
            return result;
        }

        if (check_if_safe_online(result.hash))
            result.state = state_safe;
        else
            result.state = state_not_safe;

        return result;
    }

    void move_to_quarantine(const path& filePath, const path& quarantinePath){
        string newPath = quarantinePath.string() + "/" + filePath.filename().string();
        rename(filePath.string(), newPath);
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

    string get_scan_status_name(const scan& scan) {
        switch (scan.status) {
            case status_just_started:
                return "just started";
            case status_in_progress:
                return "in progress (" + to_string(scan.results.size() * 100 / scan.filePaths.size()) + "%)";
            case status_completed:
                return "completed";
            case status_terminated:
                return "terminated (" + to_string(scan.results.size() * 100 / scan.filePaths.size()) + "%)";
        }
        return "";
    }

    string get_hash_source_name(bool isOnline){
        if(isOnline) return "online";
        else return "local";
    }

    void print_result_to_files(scan& scan) {
        stringstream resultStringStream;

        resultStringStream << "AVIR SCAN REPORT" << endl;
        resultStringStream << " --- " << endl;
        resultStringStream << "Start time: \t" << ctime(&scan.startTime);
        resultStringStream << "Elapsed: \t" << scan.elapsedSeconds.count() << " seconds" << endl;
        resultStringStream << "Status: \t" << get_scan_status_name(scan) << endl;
        resultStringStream << " --- " << endl;
        resultStringStream << "Scan path: \t" << scan.scanPath.string() << endl;
        resultStringStream << "Scan type: \t" << get_scan_type_name(scan.type) << endl;
        resultStringStream << "Hash source: \t" << get_hash_source_name(scan.online) << endl;
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

        if (scan.unreadable && !scan.unreadableResults.empty()) {
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

    void terminate_signal_handler(int signum){
        (*globalScan).status = status_terminated;
        print_result_to_files(*globalScan);
        exit(signum);
    }

    // main scan function
    void begin(scan scan) {
        // register signal handler

        globalScan = &scan;
        signal(SIGTERM, terminate_signal_handler);

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
        auto lastNow = std::chrono::system_clock::now();
        chrono::duration<double> timeSinceSave{};
        double saveInterval = 1;

        // scan online

        if(scan.online){
            vector<future<file_scan_result>> futures;
            futures.reserve(scan.filePaths.size());

            for(auto & file : scan.filePaths){
                futures.push_back(async(launch::async, scan_file_online, file));
            }

            for(auto & e : futures){
                file_scan_result result = e.get();
                scan.results.push_back(result);

                switch (result.state) {
                    case state_not_readable:
                        scan.unreadableResults.push_back(result);
                        break;
                    case state_not_safe:
                        scan.unsafeResults.push_back(result);
                        move_to_quarantine(result.path, scan.quarantineDirPath);
                        break;
                    case state_safe:
                        scan.safeResults.push_back(result);
                        break;
                }

                auto newNow = std::chrono::system_clock::now();
                scan.elapsedSeconds = newNow - start;
                timeSinceSave += newNow - lastNow;
                if(timeSinceSave.count() > saveInterval){
                    cout << "Print at " << to_string(scan.elapsedSeconds.count()) << endl;
                    print_result_to_files(scan);
                    timeSinceSave = {};
                }
                lastNow = newNow;
            }
        }

        // scan locally

        if(!scan.online){
            for (auto &file : scan.filePaths) {
                file_scan_result result = scan_file_locally(file, hashBase);
                scan.results.push_back(result);

                switch (result.state) {
                    case state_not_readable:
                        scan.unreadableResults.push_back(result);
                        break;
                    case state_not_safe:
                        scan.unsafeResults.push_back(result);
                        move_to_quarantine(result.path, scan.quarantineDirPath);
                        break;
                    case state_safe:
                        scan.safeResults.push_back(result);
                        break;
                }

                auto newNow = std::chrono::system_clock::now();
                scan.elapsedSeconds = newNow - start;
                timeSinceSave += newNow - lastNow;
                if(timeSinceSave.count() > saveInterval){
                    cout << "Print at " << to_string(scan.elapsedSeconds.count()) << endl;
                    print_result_to_files(scan);
                    timeSinceSave = {};
                }
                lastNow = newNow;

                //usleep(100);
            }
        }

        // print result at the end

        scan.status = status_completed;

        print_result_to_files(scan);
    }
}