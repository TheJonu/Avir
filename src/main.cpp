#include <iostream>
#include <string>
#include <unistd.h>
#include <csignal>
#include <sys/prctl.h>
#include <boost/filesystem.hpp>
#include <sys/types.h>
#include <dirent.h>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <chrono>
#include "scan.h"

using namespace std;
using namespace boost::filesystem;

enum action {
    action_null, action_sf, action_sl, action_sr, action_show, action_stop
};
enum option {
    option_null, option_h, option_o, option_online, option_unreadable
};

// converts a string to action enum
action get_action(const string &actionString) {
    if (actionString == "-sf") return action_sf;
    else if (actionString == "-sl") return action_sl;
    else if (actionString == "-sr") return action_sr;
    else if (actionString == "--show") return action_show;
    else if (actionString == "--stop") return action_stop;
    return action_null;
}

// converts a string to option enum
option get_option(const string &optionString) {
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
    cout << "Usage: sudo avir [action] [options]" << endl;
    cout << " Scan actions" << endl;
    cout << "   -sf <path>      scan a single file" << endl;
    cout << "   -sl <path>      scan a directory linearly" << endl;
    cout << "   -sr <path>      scan a directory recursively" << endl;
    cout << " Other actions" << endl;
    cout << "   --show          show last scan report" << endl;
    cout << "   --stop          stop all ongoing scans" << endl;
    cout << " Scan options" << endl;
    cout << "   -h <path>       specify an additional hash list file" << endl;
    cout << "   -r <path>       specify an additional report file" << endl;
    cout << "   -o, --online    check hashes online instead of locally" << endl;
    cout << "   -u, --unread    list unreadable files in report" << endl;
}

// finds files in a directory recursively
void find_files_recursive(vector<path> &filePaths, const path &scanPath) {
    recursive_directory_iterator it = recursive_directory_iterator(scanPath);
    recursive_directory_iterator end;
    while (it != end) {
        try {
            if (is_regular_file(it->path())) { filePaths.push_back(it->path()); };
            ++it;
        } catch (exception &ex) {
            it.no_push();
            try { ++it; } catch (exception &ex) {}
        }
    }
}

//finds files in a directory linearly
void find_files_linear(vector<path> &filePaths, const path &scanPath) {
    directory_iterator it = directory_iterator(scanPath);
    directory_iterator end;
    while (it != end) {
        try {
            if (is_regular_file(it->path())) { filePaths.push_back(it->path()); };
            ++it;
        } catch (exception &ex) {
            try { ++it; } catch (exception &ex) {}
        }
    }
}

// finds a single file
void find_file(vector<path> &filePaths, const path &scanPath) {
    if (is_regular_file(scanPath)) {
        filePaths.push_back(scanPath);
    }
}

// shows the last generated report
void show_last_report(const path &directoryPath) {
    // determine which report is the latest based on its number
    int bestNumber = 0;
    string bestPath;
    for (const auto &resultFile : directory_iterator(directoryPath)) {
        string path = resultFile.path().string();
        string numberString = path.substr(path.find_last_of('_') + 1, 10);
        int number = stoi(numberString);
        if (number > bestNumber) {
            bestNumber = number;
            bestPath = path;
        }
    }
    // print the contents if found
    if (bestNumber != 0) {
        cout << "Showing result nr " << bestNumber << ":" << endl;
        cout << endl;
        std::ifstream file(bestPath);
        if (file.is_open())
            cout << file.rdbuf();
        else {
            cout << "Can't open the file." << endl;
        }
    } else {
        cout << "No result files found." << endl;
    }
}

// gets a PID by its (original) name
// "borrowed" code, can't find the source now :(
int getProcIdByName(const string &procName) {
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
                string cmdPath = string("/proc/") + dirp->d_name + "/cmdline";
                std::ifstream cmdFile(cmdPath.c_str());
                string cmdLine;
                getline(cmdFile, cmdLine);
                if (!cmdLine.empty()) {
                    // Keep first cmdline item which contains the program path
                    size_t pos = cmdLine.find('\0');
                    if (pos != string::npos)
                        cmdLine = cmdLine.substr(0, pos);
                    // Keep program name only, removing the path
                    pos = cmdLine.rfind('/');
                    if (pos != string::npos)
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
        cout << "No Avir processes found." << endl;
        return;
    }
    // try to kill all processes
    while (scanPid != 0 && scanPid != thisPid) {
        uint result = kill(scanPid, SIGTERM);
        if (result == 0) {
            cout << "Successfully terminated Avir scan with PID " + to_string(scanPid) + "." << endl;
            scanPid = getProcIdByName("avir");
        } else {
            cout << "Process " + to_string(scanPid) + " couldn't be stopped." << endl;
            scanPid = 0;
        }
    }
}

int main(int argc, char *argv[]) {

    // check if root access
    if (geteuid()) {
        cout << "You must run Avir as root." << endl;
        exit(1);
    }

    // get current time
    auto start = chrono::system_clock::now();
    time_t start_time = chrono::system_clock::to_time_t(start);

    // create a base directory if it doesn't exist
    string avirString = "/avir";
    create_directories(avirString);

    // create a hash list file if it doesn't exist and add its path
    path hashListPath = avirString + "/hashlist.txt";
    if (!exists(hashListPath)) std::ofstream o(hashListPath.string());
    vector<path> hashListPaths;
    hashListPaths.push_back(hashListPath);

    // create reports directory if it doesn't exist
    string resultsString = avirString + "/reports";
    create_directories(resultsString);
    path resultsPath = canonical(resultsString);

    // add the default report file path
    path defaultOutputPath = resultsString + "/avir_" + to_string(start_time) + ".txt";
    vector<path> outputPaths;
    outputPaths.push_back(defaultOutputPath);

    // create a quarantine directory if it doesn't exist
    string quarantineDirString = avirString + "/quarantine";
    create_directories(quarantineDirString);
    path quarantineDirPath = canonical(quarantineDirString);

    // modify quarantine directory permission rules
    chmod(quarantineDirString.c_str(), S_IREAD | S_IWRITE);

    // create quarantine list file if it doesn't exist
    //string quarantineListString = avirString + "/quarantine.txt";
    //path quarantineListPath = canonical(quarantineListString);

    // print usage if no arguments are given
    if (argc == 1) {
        print_usage();
        exit(0);
    }

    // prepare
    Scan::scan scan = {};
    scan.method = Scan::method_local;
    int nextOption = 3;

    // determine action argument
    action action = get_action(argv[1]);
    if (argc >= 3 && (action == action_sf || action == action_sl || action == action_sr)) {
        string pathString = argv[2];
        scan.scope = get_scan_scope(action);
        scan.scanPath = canonical(pathString);
    } else if (action == action_show) {
        show_last_report(resultsPath);
        exit(0);
    } else if (action == action_stop) {
        stop_ongoing_scans();
        exit(0);
    } else {
        cout << "Wrong usage." << endl;
        exit(1);
    }

    // determine option arguments
    while (argc > nextOption) {
        option option = get_option(argv[nextOption]);
        if (option == option_h || option == option_o) {
            nextOption++;
            if (argc > nextOption) {
                string optionPathString = argv[nextOption];
                switch (option) {
                    case option_h: {
                        std::ifstream file{optionPathString};
                        if (access(optionPathString.c_str(), F_OK) != -1) {
                            hashListPaths.push_back(canonical(optionPathString));
                        } else {
                            cout << "Error - hash list file can't be opened." << endl;
                            exit(1);
                        }
                        break;
                    }
                    case option_o: {
                        std::ofstream file{optionPathString};
                        if (access(optionPathString.c_str(), F_OK) != -1) {
                            outputPaths.push_back(canonical(optionPathString));
                        } else {
                            cout << "Error - output file can't be opened." << endl;
                            exit(1);
                        }
                        break;
                    }
                }
            } else {
                cout << "Wrong usage." << endl;
                exit(1);
            }
        } else if (option == option_online) {
            scan.method = Scan::method_online;
        } else if (option == option_unreadable) {
            scan.printUnreadable = true;
        } else {
            cout << "Wrong usage." << endl;
            exit(1);
        }
        nextOption++;
    }

    // ensure that the file or directory exists
    switch (action) {
        case action_sf:
            if (!is_regular_file(scan.scanPath)) {
                cout << "That's not a file." << endl;
                exit(1);
            };
            break;
        case action_sl:
        case action_sr:
            if (!is_directory(scan.scanPath)) {
                cout << "That's not a directory." << endl;
                exit(1);
            }
            break;
    }

    // search for files
    std::vector<path> filePaths;
    switch (scan.scope) {
        case Scan::scope_file:
            find_file(filePaths, scan.scanPath);
            break;
        case Scan::scope_directory_linear:
            cout << "Searching for files..." << endl;
            find_files_linear(filePaths, scan.scanPath);
            break;
        case Scan::scope_directory_recursive:
            cout << "Searching for files..." << endl;
            find_files_recursive(filePaths, scan.scanPath);
            break;
    }

    // directory scan prompt
    if (scan.scope == Scan::scope_directory_linear || scan.scope == Scan::scope_directory_recursive) {
        if (filePaths.empty()) {
            cout << "Found no files at " << scan.scanPath << endl;
            exit(0);
        }
        cout << "Found " << filePaths.size() << " files at " << scan.scanPath << endl;
        cout << "Do you want to continue? [Y/n] ";
        string yn;
        cin >> yn;
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
            cout << "Scan successfully started." << endl;
        }
    }
}