#include <iostream>
#include <cstdlib>
#include <string>

#include <boost/filesystem.hpp>

#include "hash.h"


using namespace std;
using namespace boost::filesystem;


enum command{
    bad_command, scan_dir_command, close_app_command
};


command accept_input_command();
string accept_input_directory();
void scan_directory_linear(path directory_path);
bool scan_file(path file_path);
string execute(const char* cmd);
void close();


command accept_input_command()
{
    cout << " 1 - Scan a directory" << endl;
    cout << " 2 - Close the application" << endl;
    cout << "Please choose a command: ";
    int c;
    cin >> c;

    switch(c){
        case 1: return scan_dir_command;
        case 2: return close_app_command;
        default: return bad_command;
    }
}

string accept_input_directory()
{
    cout << "Please specify a directory: ";
    string dir_path;
    cin >> dir_path;

    if(!is_directory(dir_path)){
        cout << "That's not a directory" << endl;
        return "";
    }

    return dir_path;
}

void scan_directory_linear(path directory_path)
{

}

bool scan_file(path file_path)
{
    cout << "File: \t" << file_path.filename().string() << endl;

    string hash = md5_of_file(file_path.string());
    cout << "Hash: \t" << hash << endl;

    string requestStr = "whois -h hash.cymru.com " + hash;
    const char* request = &requestStr[0];

    string result = execute(request);
    int pos = result.find("NO_DATA");
    cout << "Resp: \t" << result;

    cout << "Safe: \t";
    if(pos != string::npos){
        cout << "yes" << endl;
        return false;
    }
    else{
        cout << "NOT SAFE - MALWARE DETECTED" << endl;
        return true;
    }

    cout << result << endl;

    return true;
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
    cout << "Press anything to exit...";
    getchar();
}

int main(int argc, char *argv[])
{
    // accept command input

    //accept_input_command();

    // accept directory input

    string input_path;
    do input_path = accept_input_directory();
    while (input_path.empty());
    path full_path = canonical(input_path);

    // count files

    int file_count = 0;
    for(recursive_directory_iterator iterator(full_path); iterator != recursive_directory_iterator(); ++iterator)
    {
        if (is_regular_file(iterator -> path()))
        {
            file_count++;
        }
    }
    cout << "Found " << file_count << " files at " << full_path.string() << endl;

    // scan files

    cout << "Beginning scan..." << endl;
    int number = 0;
    for(recursive_directory_iterator iterator(full_path); iterator != recursive_directory_iterator(); ++iterator)
    {
        if (is_regular_file(iterator -> path()))
        {
            cout << "--- " << ++number << "/" << file_count << " ---" << endl;
            scan_file(iterator -> path());
        }
    }

    // close

    close();
}