#include "common.h"

void inputParser(string &input, Command& command) {
    unsigned long long pos = 0;
   unsigned long long start = 0;
    pos = input.find(';');
    if (pos != string::npos) {
        command.command = input.substr(0, pos);
        start = pos + 1;
    } else {
        command.command = input;
        return;
    }

    while ((pos = input.find(';', start)) != string::npos) {
        command.params.push_back(input.substr(start, pos - start));
        start = pos + 1;
    }

    if (start < input.size()) {
        command.params.push_back(input.substr(start));
    }
}

void commandLineParser(string& input, int argc, char* argv[]) {
     for (int i = 1; i<argc; i++) {
        input += argv[i];
        input += ";";
    }
}

void displayMessage(const string& message) {
    cout<<message;
}


void displayError(const string& error) {
    cout<<"Error: "<<error;
    exit(EXIT_FAILURE);
}