#include "scan.h"

#include <iostream>
#include <chrono>
#include <ctime>
#include <future>
#include <csignal>
#include <iomanip>

#include <openssl/md5.h>
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

namespace fs = boost::filesystem;
namespace io = boost::iostreams;

namespace Scan {

    scan *globalScan;   // global pointer used only by the termination signal handler, which doesn't accept any arguments

    // converts a scan scope enum to string
    std::string get_scan_scope_string(scan_scope scanScope) {
        switch (scanScope) {
            case scope_file:
                return "single file scan";
            case scope_directory_linear:
                return "linear directory scan";
            case scope_directory_recursive:
                return "recursive directory scan";
        }
        return "";
    }

    // converts a scan method enum to string
    std::string get_scan_method_string(scan_method scanMethod){
        switch (scanMethod) {
            case method_local:
                return "local";
            case method_online:
                return "online";
        }
        return "";
    }

    //converts scan status to string
    std::string get_scan_status_string(const scan& scan) {
        switch (scan.status) {
            case status_just_started:
                return "just started";
            case status_in_progress:
                return "in progress (" + std::to_string(scan.results.size() * 100 / scan.filePaths.size()) + "%)";
            case status_completed:
                return "completed";
            case status_terminated:
                return "terminated (" + std::to_string(scan.results.size() * 100 / scan.filePaths.size()) + "%)";
        }
        return "";
    }

    // executes any console command in another process
    // borrowed code
    std::string execute(const char *cmd) {
        std::array<char, 128> buffer{};
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe) throw std::runtime_error("popen() failed!");
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    // generates an MD5 hash of a file
    // borrowed code
    std::string md5(const std::string& file_path)
    {
        unsigned char result[MD5_DIGEST_LENGTH];
        try{
            io::mapped_file_source src(file_path);
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

    // checks online if the file is safe
    bool check_hash_safety_online(std::string &fileHash) {
        std::string requestStr = "whois -h hash.cymru.com " + fileHash;
        std::string response = execute(&requestStr[0]);
        return response.find("NO_DATA") != std::string::npos;
    }

    // checks in the local hash list if the file is safe
    bool check_hash_safety_locally(std::string &fileHash, std::vector<std::string> &hashList) {
        for (const std::string &hash : hashList) {
            if (hash == fileHash) {
                return false;
            }
        }
        return true;
    }

    // loads hashes from hash list files to a vector
    std::vector<std::string> load_hashes(const std::vector<fs::path> &paths) {
        std::vector<std::string> hashBase;
        for (const fs::path &path : paths) {
            std::ifstream inStream(path.string());
            std::string hash;
            while (getline(inStream, hash)) {
                hashBase.push_back(hash);
            }
        }
        return hashBase;
    }

    // scans a single file based on hash lists
    file_scan_result scan_file_locally(fs::path &filePath, std::vector<std::string> &hashBase) {
        file_scan_result result;
        result.path = filePath;
        result.hash = md5(filePath.string());
        if (result.hash.empty()) {
            result.state = state_not_readable;
            return result;
        }
        if (check_hash_safety_locally(result.hash, hashBase))
            result.state = state_safe;
        else
            result.state = state_not_safe;
        return result;
    }

    // scans a single file online
    file_scan_result scan_file_online(const fs::path& filePath){
        file_scan_result result;
        result.path = filePath;
        result.hash = md5(filePath.string());
        if (result.hash.empty()) {
            result.state = state_not_readable;
            return result;
        }
        if (check_hash_safety_online(result.hash))
            result.state = state_safe;
        else
            result.state = state_not_safe;
        return result;
    }

    // moves a file to quarantine (requires sudo)
    void move_to_quarantine(const fs::path& filePath, const fs::path& quarantinePath){
        std::string newPath = quarantinePath.string() + "/" + filePath.filename().string();
        fs::rename(filePath.string(), newPath);
    }

    // prints current scan data to all report files
    void print_result_to_files(scan& scan) {
        // prepare report string
        std::stringstream resultStringStream;
        resultStringStream << "AVIR SCAN REPORT" << std::endl;
        resultStringStream << " --- " << std::endl;
        resultStringStream << "Start time: \t" << ctime(&scan.startTime);
        resultStringStream << "Elapsed: \t" << scan.elapsedSeconds.count() << " seconds" << std::endl;
        resultStringStream << "Status: \t" << get_scan_status_string(scan) << std::endl;
        resultStringStream << " --- " << std::endl;
        resultStringStream << "Scan path: \t" << scan.scanPath.string() << std::endl;
        resultStringStream << "Scan scope: \t" << get_scan_scope_string(scan.scope) << std::endl;
        resultStringStream << "Scan method: \t" << get_scan_method_string(scan.method) << std::endl;
        resultStringStream << " --- " << std::endl;
        resultStringStream << "File count: \t" << scan.results.size() << std::endl;
        resultStringStream << "  Unsafe: \t" << scan.unsafeResults.size() << std::endl;
        resultStringStream << "  Unreadable: \t" << scan.unreadableResults.size() << std::endl;
        resultStringStream << "  Safe: \t" << scan.safeResults.size() << std::endl;
        // add a list of unsafe files
        if (!scan.unsafeResults.empty()) {
            resultStringStream << " --- " << std::endl;
            resultStringStream << "UNSAFE FILES: \t" << std::endl;
            for (auto &r : scan.unsafeResults) {
                resultStringStream << "  " << r.path.string() << std::endl;
            }
        }
        // add a list of unreadable files if specified
        if (scan.printUnreadable && !scan.unreadableResults.empty()) {
            resultStringStream << " --- " << std::endl;
            resultStringStream << "Unreadable files: \t" << std::endl;
            for (auto &r : scan.unreadableResults) {
                resultStringStream << "  " << r.path.string() << std::endl;
            }
        }
        // print string to files
        resultStringStream << std::endl;
        for (const fs::path &outputPath : scan.reportPaths) {
            boost::filesystem::ofstream outStream(outputPath);
            outStream << resultStringStream.str();
        }
    }

    // does stuff depending on file scan result
    void react_to_result(scan& scan, const file_scan_result& result){
        switch (result.state) {
            case state_not_readable:
                scan.unreadableResults.push_back(result);
                break;
            case state_not_safe:
                scan.unsafeResults.push_back(result);
                move_to_quarantine(result.path, scan.quarantinePath);
                break;
            case state_safe:
                scan.safeResults.push_back(result);
                break;
        }
    }

    // handles the termination signal by printing a report
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
        scan.startTime = std::chrono::system_clock::to_time_t(start);

        // load hash list files
        auto hashList = load_hashes(scan.hashListPaths);

        // print result at beginning
        scan.status = status_just_started;
        print_result_to_files(scan);

        // begin scanning
        scan.status = status_in_progress;
        scan.results.reserve(scan.filePaths.size());
        auto lastNow = std::chrono::system_clock::now();
        std::chrono::duration<double> timeSinceSave{};
        double saveInterval = 2;

        // scan online if this method was chosen
        if(scan.method == method_online){
            std::vector<std::future<file_scan_result>> futures;
            futures.reserve(scan.filePaths.size());
            // create futures for each file
            for(auto & file : scan.filePaths){
                futures.push_back(async(std::launch::async, scan_file_online, file));
            }
            // retrieve scan results
            for(auto & e : futures){
                file_scan_result result = e.get();
                scan.results.push_back(result);
                react_to_result(scan, result);
                // check timer and print results periodically
                auto newNow = std::chrono::system_clock::now();
                scan.elapsedSeconds = newNow - start;
                timeSinceSave += newNow - lastNow;
                if(timeSinceSave.count() > saveInterval){
                    print_result_to_files(scan);
                    timeSinceSave = {};
                }
                lastNow = newNow;
            }
        }

        // scan locally if this method was chosen
        if(scan.method == method_local){
            // scan each file synchronously
            for (auto &file : scan.filePaths) {
                file_scan_result result = scan_file_locally(file, hashList);
                scan.results.push_back(result);
                react_to_result(scan, result);
                // check timer and print results periodically
                auto newNow = std::chrono::system_clock::now();
                scan.elapsedSeconds = newNow - start;
                timeSinceSave += newNow - lastNow;
                if(timeSinceSave.count() > saveInterval){
                    print_result_to_files(scan);
                    timeSinceSave = {};
                }
                lastNow = newNow;
            }
        }

        // print report at the end
        scan.status = status_completed;
        print_result_to_files(scan);
    }
}