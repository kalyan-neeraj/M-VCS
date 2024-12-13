#include "common.h"

using namespace std;

int main(int argc , char* argv[]) {
    string input;
    Command command;
    commandLineParser(input, argc, argv);
    inputParser(input, command);
    handleCommand(command);
    /******************************************************   You can comment out this as when required for debugging else is not required****************************************************************************************/
    // cout<<command.command<<"\n";
    // for (auto it : command.params) {
    //     cout<<"Parms: "<<it<<"\n";
    // }
    /**************************************************************************************************************************************************************************************************************************/
}