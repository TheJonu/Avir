#include <iostream>
#include <string>
#include <unistd.h>
#include <boost/filesystem.hpp>
#include "scan.h"

using namespace std;
using namespace boost::filesystem;

enum action {
    action_null, action_sf, action_sl, action_sr, action_show, action_stop
};
enum option {
    option_null, option_b, option_o, option_online
};

action get_action(const string &actionString) {
    if (actionString == "-f") return action_sf;
    else if (actionString == "-l") return action_sl;
    else if (actionString == "-r") return action_sr;
    else if (actionString == "--show") return action_show;
    else if (actionString == "--stop") return action_stop;
    return action_null;
}

option get_option(const string &optionString) {
    if (optionString == "-b") return option_b;
    else if (optionString == "-o") return option_o;
    else if (optionString == "--online") return option_online;
    return option_null;
}

Scan::scan_type get_scan_type(const action &action){
    if(action == action_sf) return Scan::type_file;
    else if(action == action_sl) return Scan::type_directory_linear;
    else if(action == action_sr) return Scan::type_directory_recursive;
    return Scan::type_file;
}

void print_usage() {
    cout << "Usage: avir [action] [options]" << endl;
    cout << " Scan actions" << endl;
    cout << "   -f <path>     scan a single file" << endl;
    cout << "   -l <path>     scan a directory linearly" << endl;
    cout << "   -r <path>     scan a directory recursively" << endl;
    cout << " Other actions" << endl;
    cout << "   --show        show last scan result" << endl;
    cout << "   --stop        stop all ongoing scans" << endl;
    cout << " Scan options" << endl;
    cout << "   -b <path>     specify additional hash base" << endl;
    cout << "   -o <path>     specify additional output file" << endl;
    cout << "   --online      check hashes online" << endl;
}

void find_file(vector<path> &filePaths, path &scanPath) {
    if (is_regular_file(scanPath)) {
        filePaths.push_back(scanPath);
    }
}

void find_files_linear(vector<path> &filePaths, path &scanPath) {
    for (directory_iterator iterator(scanPath); iterator != directory_iterator(); ++iterator) {
        path path = iterator->path();
        if (is_regular_file(path)) {
            filePaths.push_back(path);
        }
    }
}

void find_files_recursive(vector<path> &filePaths, path &scanPath) {
    for (recursive_directory_iterator iterator(scanPath); iterator != recursive_directory_iterator(); ++iterator) {
        path path = iterator->path();
        if (is_regular_file(path)) {
            filePaths.push_back(path);
        }
    }
}

void show_last_report(path &directoryPath){
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

void stop_ongoing_scans(){

}

int main(int argc, char *argv[]) {
    // start timer

    auto start = std::chrono::system_clock::now();
    time_t start_time = chrono::system_clock::to_time_t(start);

    // determine paths

    char const *home = getenv("HOME");
    string homeString(home);

    string avirString = homeString + "/Avir";
    create_directories(avirString);

    string hashbaseString = avirString + "/hashbase.txt";
    vector<path> hashBasePaths;
    hashBasePaths.push_back(canonical(hashbaseString));

    string resultsString = avirString + "/results";
    path resultsPath = canonical(resultsString);
    create_directories(resultsString);
    path defaultOutputPath = resultsString + "/avir_" + to_string(start_time) + ".txt";
    vector<path> outputPaths;
    outputPaths.push_back(defaultOutputPath);

    // print usage if no arguments

    if (argc == 1) {
        print_usage();
        return 0;
    }

    // prepare scan

    Scan::scan scan;
    int nextOption = 3;

    // determine action argument

    action action = get_action(argv[1]);

    if (argc >=3 && (action == action_sf || action == action_sl || action == action_sr)) {
        string pathString = argv[2];
        scan.type = get_scan_type(action);
        scan.scanPath = canonical(pathString);
    } else if (action == action_show) {
        show_last_report(resultsPath);
        return 0;
    } else if (action == action_stop) {
        stop_ongoing_scans();
        return 0;
    } else {
        cout << "Wrong usage." << endl;
        return 0;
    }

    // determine option arguments

    while (argc > nextOption) {
        option option = get_option(argv[nextOption]);
        if (option == option_b || option == option_o) {
            nextOption++;
            if (argc > nextOption) {
                string optionPathString = argv[nextOption];
                switch (option) {
                    case option_b: {
                        std::ifstream file{optionPathString};
                        if (access(optionPathString.c_str(), F_OK) != -1) {
                            hashBasePaths.push_back(canonical(optionPathString));
                        } else {
                            cout << "Error - hash base file can't be opened." << endl;
                            return 0;
                        }
                    }
                    case option_o: {
                        std::ofstream file{optionPathString};
                        if (access(optionPathString.c_str(), F_OK) != -1) {
                            outputPaths.push_back(canonical(optionPathString));
                        } else {
                            cout << "Error - output file can't be opened." << endl;
                            return 0;
                        }
                    }
                    case option_null:
                        break;
                }
            } else {
                cout << "Wrong usage." << endl;
                return 0;
            }
        } else if (option == option_online) {
            scan.online = true;
        } else {
            cout << "Wrong usage." << endl;
            return 0;
        }
        nextOption++;
    }

    // ensure that the file or directory exists

    switch (action) {
        case action_sf:
            if (!is_regular_file(scan.scanPath)) {
                cout << "That's not a file." << endl;
                return 0;
            };
            break;
        case action_sl:
        case action_sr:
            if (!is_directory(scan.scanPath)) {
                cout << "That's not a directory." << endl;
                return 0;
            }
            break;
    }

    // search for files

    std::vector<path> filePaths;

    switch (scan.type) {
        case Scan::type_file:
            find_file(filePaths, scan.scanPath);
            break;
        case Scan::type_directory_linear:
            cout << "Searching for files..." << endl;
            find_files_linear(filePaths, scan.scanPath);
            break;
        case Scan::type_directory_recursive:
            cout << "Searching for files..." << endl;
            find_files_recursive(filePaths, scan.scanPath);
            break;
    }

    // directory scan prompt

    if (scan.type == Scan::type_directory_linear || scan.type == Scan::type_directory_recursive) {

        if (filePaths.empty()) {
            cout << "Found no files at " << scan.scanPath << endl;
            return 0;
        }

        cout << "Found " << filePaths.size() << " files at " << scan.scanPath << endl;

        string yn;
        cout << "Do you want to continue? [Y/n] ";
        cin >> yn;
        if (yn != "y" && yn != "Y") {
            return 0;
        }
    }

    // begin scan

    switch (fork()) {
        case -1: {
            printf("Fork error! Scan aborted.");
            return -1;
        }
        case 0: {
            scan.filePaths = filePaths;
            scan.hashBasePaths = hashBasePaths;
            scan.outputPaths = outputPaths;
            Scan::begin(scan);
            return 0;
        }
        default: {
            cout << "Scan started." << endl;
            cout << "Result file locations:" << endl;
            for (const path &outputPath : outputPaths) {
                cout << "  " << outputPath.string() << endl;
            }
        }
    }
}