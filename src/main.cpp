#include <iostream>
#include <string>
#include <unistd.h>

#include <boost/filesystem.hpp>
#include <sys/wait.h>

#include "scan.h"

using namespace std;
using namespace boost::filesystem;

enum action{no_action, sf, sl, sr, show, stop};
enum option{no_option, b, o, online};

action get_action(const string& actionString){
    if(actionString == "-sf") return sf;
    if(actionString == "-sl") return sl;
    if(actionString == "-sr") return sr;
    if(actionString == "--show") return show;
    if(actionString == "--stop") return stop;
    return no_action;
}

option get_option(const string& optionString){
    if(optionString == "-b") return b;
    if(optionString == "-o") return o;
    if(optionString == "--online") return online;
    return no_option;
}

void print_usage()
{
    cout << "Usage: avir [action] [options]" << endl;
    cout << " Scan actions" << endl;
    cout << "   -sf <path>    scan a single file" << endl;
    cout << "   -sl <path>    scan a directory linearly" << endl;
    cout << "   -sr <path>    scan a directory recursively" << endl;
    cout << " Other actions" << endl;
    cout << "   --show        show last scan result" << endl;
    cout << "   --stop        stop all ongoing scans" << endl;
    cout << " Scan options" << endl;
    cout << "   -b <path>     specify additional hash base" << endl;
    cout << "   -o <path>     specify additional output file" << endl;
    cout << "   --online      check hashes online" << endl;
}

void find_file(vector<path>& filePaths, path& scanPath)
{
    if(is_regular_file(scanPath)){
        filePaths.push_back(scanPath);
    }
}

void find_files_linear(vector<path>& filePaths, path& scanPath)
{
    for(directory_iterator iterator(scanPath); iterator != directory_iterator(); ++iterator)
    {
        path path = iterator -> path();
        if (is_regular_file(path))
        {
            filePaths.push_back(path);
        }
    }
}

void find_files_recursive(vector<path>& filePaths, path& scanPath)
{
    for(recursive_directory_iterator iterator(scanPath); iterator != recursive_directory_iterator(); ++iterator)
    {
        path path = iterator -> path();
        if (is_regular_file(path))
        {
            filePaths.push_back(path);
        }
    }
}

int main(int argc, char *argv[])
{
    // start timer

    auto start = std::chrono::system_clock::now();
    time_t start_time = chrono::system_clock::to_time_t(start);

    // determine directory and file paths

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
    path defaultOutputPath = resultsString + "/scan_" + to_string(start_time) + ".txt";
    vector<path> outputPaths;
    outputPaths.push_back(defaultOutputPath);

    // print usage if no arguments

    if(argc == 1){
        print_usage();
        return 0;
    }

    /*
    if(argc % 2 == 0){
        cout << "Wrong usage." << endl;
        print_usage();
        return 0;
    }
     */

    // prepare arguments info

    bool isScan = false;
    bool scanOnline = false;
    Scan::scan_type scanType;
    path scanPath;
    int nextOption = 3;

    // determine action argument

    action action = get_action(argv[1]);

    if(action == sf || action == sl || action == sr)
    {
        isScan = true;
        string pathString = argv[2];
        switch(action){
            case sf: if(!is_regular_file(pathString)){cout << "That's not a file." << endl; return 0;}; break;
            case sl:
            case sr: if(!is_directory(pathString)){cout << "That's not a directory." << endl; return 0;} break;
        }
        switch(action){
            case sf: scanType = Scan::file_scan; break;
            case sl: scanType = Scan::dir_linear_scan; break;
            case sr: scanType = Scan::dir_recursive_scan; break;
        }
        scanPath = canonical(pathString);
    }
    else if(action == show)
    {
        int bestNumber = 0;
        string bestPath;
        for (const auto & resultFile : directory_iterator(resultsPath)){
            string path = resultFile.path().string();
            string numberString = path.substr(path.find_last_of('_') + 1, 10);
            int number = stoi(numberString);
            if(number > bestNumber){
                bestNumber = number;
                bestPath = path;
            }
        }

        if(bestNumber != 0){
            cout << "Showing result nr " << bestNumber << ":" << endl;
            cout << endl;

            std::ifstream file(bestPath);
            if (file.is_open())
                cout << file.rdbuf();
            else{
                cout << "Can't open the file." << endl;
            }
        }
        else{
            cout << "No result files found." << endl;
        }

        return 0;
    }
    else if(action == stop)
    {
        cout << "Stop action" << endl;
        return 0;
    }
    else
    {
        cout << "Wrong action usage." << endl;
        return 0;
    }

    // determine option arguments

    if(isScan){
        while(argc > nextOption){
            option option = get_option(argv[nextOption]);
            if(option == b || option == o){
                nextOption++;
                if(argc > nextOption){
                    string optionPathString = argv[nextOption];
                    switch (option) {
                        case b:{
                            std::ifstream file {optionPathString};
                            if(access(optionPathString.c_str(), F_OK) != -1){
                                hashBasePaths.push_back(canonical(optionPathString));
                            }
                            else{
                                cout << "Error - hash base file can't be opened." << endl;
                                return 0;
                            }
                        }
                        case o:{
                            std::ofstream file {optionPathString};
                            if(access(optionPathString.c_str(), F_OK) != -1){
                                outputPaths.push_back(canonical(optionPathString));
                            }
                            else{
                                cout << "Error - output file can't be opened." << endl;
                                return 0;
                            }
                        }
                    }
                }
                else{
                    cout << "Wrong option usage." << endl;
                    return 0;
                }
            }
            else if (option == online){
                scanOnline = true;
            }
            else{
                cout << "Wrong option usage." << endl;
                return 0;
            }
            nextOption++;
        }
    }

    /*
    if(arg1 == "-sf"){
        if(!is_regular_file(arg2)){
            cout << "That's not a file." << endl;
            return 0;
        }
        scanType = Scan::file_scan;
    }
    else if(arg1 == "-dl"){
        if(!is_directory(arg2)){
            cout << "That's not a directory." << endl;
            return 0;
        }
        scanType = Scan::dir_linear_scan;
    }
    else if(arg1 == "-dr"){
        if(!is_directory(arg2)){
            cout << "That's not a directory." << endl;
            return 0;
        }
        scanType = Scan::dir_recursive_scan;
    }
    else{
        cout << "Wrong type." << endl;
        print_usage();
        return 0;
    }
     */

    //scanPath = canonical(arg2);

    // determine options

    /*
    if(argc >= 5){
        string arg3 = argv[3];
        string arg4 = argv[4];

        if(arg3 == "-o"){
            std::ofstream file {arg4};
            if(access(arg4.c_str(), F_OK) != -1){
                outputPaths.push_back(canonical(arg4));
            }
        }
        else{
            cout << "Wrong option." << endl;
            print_usage();
            return 0;
        }
    }
     */

    // search for files

    std::vector<path> filePaths;

    switch(scanType){
        case Scan::file_scan:
            find_file(filePaths, scanPath);
            break;
        case Scan::dir_linear_scan:
            cout << "Searching for files..." << endl;
            find_files_linear(filePaths, scanPath);
            break;
        case Scan::dir_recursive_scan:
            cout << "Searching for files..." << endl;
            find_files_recursive(filePaths, scanPath);
            break;
    }

    // directory scan prompt

    if(scanType == Scan::dir_linear_scan || scanType == Scan::dir_recursive_scan){

        if(filePaths.empty()){
            cout << "Found no files at " << scanPath << endl;
            return 0;
        }

        cout << "Found " << filePaths.size() << " files at " << scanPath << endl;

        string yn;
        cout << "Do you want to continue? [Y/n] ";
        cin >> yn;
        if(yn != "y" && yn != "Y"){
            return 0;
        }
    }

    // begin scan

    switch(fork()){
        case -1: {
            printf("Fork error! Scan aborted.");
            return -1;
        }
        case 0: {
            Scan::scan scan;
            scan.scanType = scanType;
            scan.scanOnline = scanOnline;
            scan.scanPath = scanPath;
            scan.filePaths = filePaths;
            scan.hashBasePaths = hashBasePaths;
            scan.outputPaths = outputPaths;
            scan.begin();
            return 0;
        }
        default: {
            cout << "Scan started." << endl;
            cout << "Result file locations:" << endl;
            for(const path& outputPath : outputPaths){
                cout << "  " << outputPath.string() << endl;
            }
        }
    }
}