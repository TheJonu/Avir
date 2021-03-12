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
void scan_directory(path directory_path);
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

void scan_directory(path directory_path)
{
    int file_count = 0;
    for(recursive_directory_iterator iterator(directory_path); iterator != recursive_directory_iterator(); ++iterator)
    {
        if (is_regular_file(iterator -> path()))
        {
            file_count++;
        }
    }
    cout << "Found " << file_count << " files at " << directory_path.string() << endl;

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

void close()
{
    cout << endl;
    cout << "That's all! Thanks for using me!" << endl;
}

int main(int argc, char *argv[])
{
    command comm = accept_input_command();
    switch(comm){
        case scan_file_command:
        {
            string file_path = accept_input_file_path();
            scan_file(canonical(file_path));
            close();
            break;
        }
        case scan_dir_command:
        {
            string dir_path = accept_input_directory_path();
            scan_directory(canonical(dir_path));
            close();
            break;
        }
        case close_app_command:
        {
            close();
            break;
        }
    }
}