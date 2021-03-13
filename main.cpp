#include <iostream>
#include <cstdlib>
#include <string>

#include <boost/filesystem.hpp>

#include "hash.h"


using namespace std;
using namespace boost::filesystem;


enum command{
    bad_command, scan_dir_command, scan_file_command, close_app_command
};

struct scan_result{
    bool is_safe;
};

command accept_input_command();
string accept_input_file_path();
string accept_input_directory_path();
void scan_directory_linearly(path directory_path);
void scan_directory_recursively(path directory_path);
scan_result scan_file(const path& file_path);
string execute(const char* cmd);
void close();


command accept_input_command()
{
    cout << " 1 - Scan a file" << endl;
    cout << " 2 - Scan a directory" << endl;
    cout << " 3 - Close the application" << endl;
    cout << "Please choose a command: ";
    int c;
    cin >> c;

    switch(c){
        case 1: return scan_file_command;
        case 2: return scan_dir_command;
        case 3: return close_app_command;
        default: return bad_command;
    }
}

string accept_input_file_path()
{
    string file_path;
    bool is_file;

    do{
        cout << "Please specify a file path: ";
        cin >> file_path;

        if(!is_regular_file(file_path)){
            cout << "That's not a file." << endl;
        }
        else{
            is_file = true;
        }
    }
    while(!is_file);

    return file_path;
}

string accept_input_directory_path()
{
    string dir_path;
    bool is_dir;

    do{
        cout << "Please specify a directory path: ";
        cin >> dir_path;

        if(!is_directory(dir_path)){
            cout << "That's not a directory." << endl;
        }
        else{
            is_dir = true;
        }
    }
    while(!is_dir);

    return dir_path;
}

void scan_directory_linearly(path directory_path)
{
    if(!is_directory(directory_path)){
        cout << "That's not a directory." << endl;
        return;
    }

    int file_count = 0;
    for(directory_iterator iterator(directory_path); iterator != directory_iterator(); ++iterator)
    {
        if (is_regular_file(iterator -> path()))
        {
            file_count++;
        }
    }
    cout << "Found " << file_count << " files at " << directory_path.string() << endl;

    string yn;
    cout << "Do you want to continue? [Y/n] ";
    cin >> yn;
    if(yn != "y" && yn != "Y"){
        return;
    }

    int number = 0;
    for(directory_iterator iterator(directory_path); iterator != directory_iterator(); ++iterator)
    {
        if (is_regular_file(iterator -> path()))
        {
            cout << "--- " << ++number << "/" << file_count << " --------------------------------" << endl;
            scan_file(iterator -> path());
        }
    }
}

void scan_directory_recursively(path directory_path)
{
    if(!is_directory(directory_path)){
        cout << "That's not a directory." << endl;
        return;
    }

    int file_count = 0;
    for(recursive_directory_iterator iterator(directory_path); iterator != recursive_directory_iterator(); ++iterator)
    {
        if (is_regular_file(iterator -> path()))
        {
            file_count++;
        }
    }
    cout << "Found " << file_count << " files at " << directory_path.string() << endl;

    string yn;
    cout << "Do you want to continue? [Y/n] ";
    cin >> yn;
    if(yn != "y" && yn != "Y"){
        return;
    }

    int number = 0;
    for(recursive_directory_iterator iterator(directory_path); iterator != recursive_directory_iterator(); ++iterator)
    {
        if (is_regular_file(iterator -> path()))
        {
            cout << "--- " << ++number << "/" << file_count << " --------------------------------" << endl;
            scan_file(iterator -> path());
        }
    }
}

scan_result scan_file(const path& file_path)
{
    if(!is_regular_file(file_path)){
        cout << "That's not a directory." << endl;
        return scan_result{};
    }

    cout << "File: \t" << file_path.filename().string() << endl;
    cout << "Path: \t" << file_path.parent_path().string() << endl;

    string hash = md5_of_file(file_path.string());
    cout << "Hash: \t" << hash << endl;

    string requestStr = "whois -h hash.cymru.com " + hash;
    const char* request = &requestStr[0];

    string response = execute(request);
    int pos = response.find("NO_DATA");

    scan_result result{};

    cout << "Safe: \t";
    if(pos != string::npos){
        cout << "yes" << endl;
        result.is_safe = false;
    }
    else{
        cout << "NOT SAFE - MALWARE DETECTED" << endl;
        result.is_safe = true;
    }

    return result;
}

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

void print_usage()
{
    cout << "Usage: avir [type] [path]" << endl;
    cout << "  -f <filepath>: scan a single file" << endl;
    cout << "  -dl <dirpath>: scan a directory linearly" << endl;
    cout << "  -dr <dirpath>: scan a directory recursively" << endl;
}

int main(int argc, char *argv[])
{
    if(argc <= 2){
        print_usage();
        return 0;
    }

    string arg1 = argv[1];
    string arg2 = argv[2];

    if(arg1 == "-f"){
        scan_file(canonical(arg2));
    }
    else if(arg1 == "-dl"){
        scan_directory_linearly(arg2);
    }
    else if(arg1 == "-dr"){
        scan_directory_recursively(arg2);
    }
}