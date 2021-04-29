#include "scan.h"

#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <csignal>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <chrono>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

enum action {
    action_null, action_sf, action_sl, action_sr, action_show, action_stop
};
enum option {
    option_null, option_h, option_o, option_online, option_unreadable
};

// converts a string to action enum
action get_action(const std::string &actionString) {
    if (actionString == "-sf") return action_sf;
    else if (actionString == "-sl") return action_sl;
    else if (actionString == "-sr") return action_sr;
    else if (actionString == "--show") return action_show;
    else if (actionString == "--stop") return action_stop;
    return action_null;
}

// converts a string to option enum
option get_option(const std::string &optionString) {
    if (optionString == "-h") return option_h;
    else if (optionString == "-r") return option_o;
    else if (optionString == "-o") return option_online;
    else if (optionString == "--online") return option_online;
    else if (optionString == "-u") return option_unreadable;
    else if (optionString == "--unreadable") return option_unreadable;
    return option_null;
}

// converts an action enum to scan scope enum
Scan::scan_scope get_scan_scope(const action &action) {
    if (action == action_sf) return Scan::scope_file;
    else if (action == action_sl) return Scan::scope_directory_linear;
    else if (action == action_sr) return Scan::scope_directory_recursive;
    return Scan::scope_file;
}

// prints app usage to the console
void print_usage() {
    std::cout << "Usage: sudo avir [action] [options]" << std::endl;
    std::cout << " Scan actions" << std::endl;
    std::cout << "   -sf <path>      scan a single file" << std::endl;
    std::cout << "   -sl <path>      scan a directory linearly" << std::endl;
    std::cout << "   -sr <path>      scan a directory recursively" << std::endl;
    std::cout << " Other actions" << std::endl;
    std::cout << "   --show          show last scan report" << std::endl;
    std::cout << "   --stop          stop all ongoing scans" << std::endl;
    std::cout << " Scan options" << std::endl;
    std::cout << "   -h <path>       specify an additional hash list file" << std::endl;
    std::cout << "   -r <path>       specify an additional report file" << std::endl;
    std::cout << "   -o, --online    check hashes online instead of locally" << std::endl;
    std::cout << "   -u, --unread    list unreadable files in report" << std::endl;
}

// finds files in a directory recursively
void find_files_recursive(std::vector<fs::path> &filePaths, const fs::path &scanPath) {
    fs::recursive_directory_iterator it = fs::recursive_directory_iterator(scanPath);
    fs::recursive_directory_iterator end;
    while (it != end) {
        try {
            if (is_regular_file(it->path())) { filePaths.push_back(it->path()); };
            ++it;
        } catch (std::exception &ex) {
            it.no_push();
            try { ++it; } catch (std::exception &ex) {}
        }
    }
}

//finds files in a directory linearly
void find_files_linear(std::vector<fs::path> &filePaths, const fs::path &scanPath) {
    fs::directory_iterator it = fs::directory_iterator(scanPath);
    fs::directory_iterator end;
    while (it != end) {
        try {
            if (is_regular_file(it->path())) { filePaths.push_back(it->path()); };
            ++it;
        } catch (std::exception &ex) {
            try { ++it; } catch (std::exception &ex) {}
        }
    }
}

// finds a single file
void find_file(std::vector<fs::path> &filePaths, const fs::path &scanPath) {
    if (is_regular_file(scanPath)) {
        filePaths.push_back(scanPath);
    }
}

// shows the last generated report
void show_last_report(const fs::path &directoryPath) {
    // determine which report is the latest based on its number
    int bestNumber = 0;
    std::string bestPath;
    for (const auto &resultFile : fs::directory_iterator(directoryPath)) {
        std::string path = resultFile.path().string();
        std::string numberString = path.substr(path.find_last_of('_') + 1, 10);
        int number = stoi(numberString);
        if (number > bestNumber) {
            bestNumber = number;
            bestPath = path;
        }
    }
    // print the contents if found
    if (bestNumber != 0) {
        std::cout << "Showing result nr " << bestNumber << ":" << std::endl;
        std::cout << std::endl;
        std::ifstream file(bestPath);
        if (file.is_open())
            std::cout << file.rdbuf();
        else {
            std::cout << "Can't open the file." << std::endl;
        }
    } else {
        std::cout << "No result files found." << std::endl;
    }
}

// gets a PID by its (original) name
// "borrowed" code, can't find the source now :(
int getProcIdByName(const std::string &procName) {
    int pid = -1;
    // Open the /proc directory
    DIR *dp = opendir("/proc");
    if (dp != nullptr) {
        // Enumerate all entries in directory until process found
        struct dirent *dirp;
        while (pid < 0 && (dirp = readdir(dp))) {
            // Skip non-numeric entries
            int id = atoi(dirp->d_name);
            if (id > 0) {
                // Read contents of virtual /proc/{pid}/cmdline file
                std::string cmdPath = std::string("/proc/") + dirp->d_name + "/cmdline";
                std::ifstream cmdFile(cmdPath.c_str());
                std::string cmdLine;
                getline(cmdFile, cmdLine);
                if (!cmdLine.empty()) {
                    // Keep first cmdline item which contains the program path
                    size_t pos = cmdLine.find('\0');
                    if (pos != std::string::npos)
                        cmdLine = cmdLine.substr(0, pos);
                    // Keep program name only, removing the path
                    pos = cmdLine.rfind('/');
                    if (pos != std::string::npos)
                        cmdLine = cmdLine.substr(pos + 1);
                    // Compare against requested process name
                    if (procName == cmdLine)
                        pid = id;
                }
            }
        }
    }
    closedir(dp);
    return pid;
}

void stop_ongoing_scans() {
    // get first PIDs
    auto thisPid = getpid();
    auto scanPid = getProcIdByName("avir");
    // break if no process found
    if (scanPid == 0 || scanPid == thisPid) {
        std::cout << "No Avir processes found." << std::endl;
        return;
    }
    // try to kill all processes
    while (scanPid != 0 && scanPid != thisPid) {
        uint result = kill(scanPid, SIGTERM);
        if (result == 0) {
            std::cout << "Successfully terminated Avir scan with PID " + std::to_string(scanPid) + "." << std::endl;
            scanPid = getProcIdByName("avir");
        } else {
            std::cout << "Process " + std::to_string(scanPid) + " couldn't be stopped." << std::endl;
            scanPid = 0;
        }
    }
}

int main(int argc, char *argv[]) {

    // print usage if no arguments are given
    if (argc == 1) {
        print_usage();
        exit(0);
    }

    // check if root access
    if (geteuid()) {
        std::cout << "You must run Avir as root." << std::endl;
        exit(1);
    }

    // get current time
    auto start = std::chrono::system_clock::now();
    time_t start_time = std::chrono::system_clock::to_time_t(start);

    // create a base directory if it doesn't exist
    std::string avirString = "/avir";
    fs::create_directories(avirString);

    // create a hash list file if it doesn't exist and add its path
    fs::path hashListPath = avirString + "/hashlist.txt";
    if (!exists(hashListPath)) std::ofstream o(hashListPath.string());
    std::vector<fs::path> hashListPaths;
    hashListPaths.push_back(hashListPath);

    // create reports directory if it doesn't exist
    std::string resultsString = avirString + "/reports";
    fs::create_directories(resultsString);
    fs::path resultsPath = fs::canonical(resultsString);

    // add the default report file path
    fs::path defaultOutputPath = resultsString + "/avir_" + std::to_string(start_time) + ".txt";
    std::vector<fs::path> outputPaths;
    outputPaths.push_back(defaultOutputPath);

    // create a quarantine directory if it doesn't exist
    std::string quarantineDirString = avirString + "/quarantine";
    fs::create_directories(quarantineDirString);
    fs::path quarantineDirPath = fs::canonical(quarantineDirString);

    // modify quarantine directory permission rules
    chmod(quarantineDirString.c_str(), S_IREAD | S_IWRITE);

    // create quarantine list file if it doesn't exist
    //string quarantineListString = avirString + "/quarantine.txt";
    //path quarantineListPath = canonical(quarantineListString);

    // prepare
    Scan::scan scan = {};
    scan.method = Scan::method_local;
    int nextOption = 3;

    // determine action argument
    action action = get_action(argv[1]);
    if (argc >= 3 && (action == action_sf || action == action_sl || action == action_sr)) {
        std::string pathString = argv[2];
        scan.scope = get_scan_scope(action);
        scan.scanPath = fs::canonical(pathString);
    } else if (action == action_show) {
        show_last_report(resultsPath);
        exit(0);
    } else if (action == action_stop) {
        stop_ongoing_scans();
        exit(0);
    } else {
        std::cout << "Wrong usage." << std::endl;
        exit(1);
    }

    // determine option arguments
    while (argc > nextOption) {
        option option = get_option(argv[nextOption]);
        if (option == option_h || option == option_o) {
            nextOption++;
            if (argc > nextOption) {
                std::string optionPathString = argv[nextOption];
                switch (option) {
                    case option_h: {
                        std::ifstream file{optionPathString};
                        if (access(optionPathString.c_str(), F_OK) != -1) {
                            hashListPaths.push_back(fs::canonical(optionPathString));
                        } else {
                            std::cout << "Error - hash list file can't be opened." << std::endl;
                            exit(1);
                        }
                        break;
                    }
                    case option_o: {
                        std::ofstream file{optionPathString};
                        if (access(optionPathString.c_str(), F_OK) != -1) {
                            outputPaths.push_back(fs::canonical(optionPathString));
                        } else {
                            std::cout << "Error - output file can't be opened." << std::endl;
                            exit(1);
                        }
                        break;
                    }
                }
            } else {
                std::cout << "Wrong usage." << std::endl;
                exit(1);
            }
        } else if (option == option_online) {
            scan.method = Scan::method_online;
        } else if (option == option_unreadable) {
            scan.printUnreadable = true;
        } else {
            std::cout << "Wrong usage." << std::endl;
            exit(1);
        }
        nextOption++;
    }

    // ensure that the file or directory exists
    switch (action) {
        case action_sf:
            if (!is_regular_file(scan.scanPath)) {
                std::cout << "That's not a file." << std::endl;
                exit(1);
            };
            break;
        case action_sl:
        case action_sr:
            if (!is_directory(scan.scanPath)) {
                std::cout << "That's not a directory." << std::endl;
                exit(1);
            }
            break;
    }

    // search for files
    std::vector<fs::path> filePaths;
    switch (scan.scope) {
        case Scan::scope_file:
            find_file(filePaths, scan.scanPath);
            break;
        case Scan::scope_directory_linear:
            std::cout << "Searching for files..." << std::endl;
            find_files_linear(filePaths, scan.scanPath);
            break;
        case Scan::scope_directory_recursive:
            std::cout << "Searching for files..." << std::endl;
            find_files_recursive(filePaths, scan.scanPath);
            break;
    }

    // directory scan prompt
    if (scan.scope == Scan::scope_directory_linear || scan.scope == Scan::scope_directory_recursive) {
        if (filePaths.empty()) {
            std::cout << "Found no files at " << scan.scanPath << std::endl;
            exit(0);
        }
        std::cout << "Found " << filePaths.size() << " files at " << scan.scanPath << std::endl;
        std::cout << "Do you want to continue? [Y/n] ";
        std::string yn;
        std::cin >> yn;
        if (yn != "y" && yn != "Y") {
            exit(0);
        }
    }

    // begin the actual scan in the background
    switch (fork()) {
        case -1: {
            printf("Fork error! Scan aborted.");
            exit(1);
        }
        case 0: {
            prctl(PR_SET_NAME, (unsigned long) "avirscan");
            scan.filePaths = filePaths;
            scan.hashListPaths = hashListPaths;
            scan.reportPaths = outputPaths;
            scan.quarantinePath = quarantineDirPath;
            //scan.quarantineListPath = quarantineListPath;
            Scan::begin(scan);
            exit(0);
        }
        default: {
            std::cout << "Scan successfully started." << std::endl;
        }
    }
}