#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include<string>
#include <vector>
#include <sys/stat.h>
#include <filesystem>
#include <openssl/sha.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <dirent.h>
#include <unordered_set>
#include <stack>
#include <algorithm>
#include <unordered_map>
#include <ctime>
#include <set>
using namespace std;


//Commmand Line interepreter//
struct Command {
    string command;
    vector<string> params;
};

struct AddFormat {
    string sha;
    string file_path;
    string file_per;
};

struct TreeEntry {
    string sha;
    string type;
    string name;
    string permissions;
    string content;
    string fileSize;
};

struct Commit_Object {
    string tree_sha;
    string parent_commit_sha;
    string author;
    string timestamp;
    string message;
    
    string formatCommit() const {
        stringstream ss;
        ss << "tree " << tree_sha << "\n";
        if (!parent_commit_sha.empty()) {
            ss << "parent " << parent_commit_sha << "\n";
        }
        ss << "author " << author << " " << timestamp << "\n";
        ss << "\n" << message;
        
        return ss.str();
    }
};


//SHA HASHING AND BLOB CREATION//

const string getSHA(const string& path);
const string getBlobString(string& path);
const string readBlobFile(const string& path);
void handleBlobFlags(const string& blobString, TreeEntry& tree_entry, const string& flag);
void ensure_directory_exists(string &dir_path);
void create_gitObject(string& full_path, string& object_sha, int req);
void create_file(string& full_path, const string& gitBlob);
void handleCommand(Command& command);
void inputParser(string &input, Command& cmd);
void commandLineParser(string &input, int argc, char* argv[]);
void display_ls_tree(int n, TreeEntry& tree_entry);

void displayMessage(const string& message);
void displayError(const string& error);
void setBlobContentToTree(const string & blob, TreeEntry& tree_entry);
string getFilePermissions(const string& path);
void writeToFile(const vector<AddFormat>& add_format, const string& filename);
string readHeadSHA();
bool loadCommitObject(const string& commitFilePath, Commit_Object& commit);
void clearFile(const string& filePath);
void writeToHEAD(const string& sha);
void retrieveHeadSHA(const string& shaFilePath, string& sha);
string extractParentSha(string& commitContent);

//MESSAGES//
const string ALRDY_EXISTS = "REPO HAS ALREADY BEEN INITIALIZED\n";

//DEFAULTS//
const string DEFAULT_COMMIT_MSG = "successfully commited\n";
const string DEFAULT_PATH = ".mygit";
const string OBJECT_DIR = DEFAULT_PATH + "/objects";
const string REFS_DIR = DEFAULT_PATH + "/refs";
const string HEAD_FILE = DEFAULT_PATH + "/HEAD";
const string INDEX = DEFAULT_PATH + "/index";


//COMMANDS//
const string INIT = "init";
const string ADD = "add";
const string LOG = "log";
const string CHECK_OUT ="checkout";


const string HASH = "hash-object";
const string CAT = "cat-file";


const string WRITE = "write-tree";
const string LIST = "ls-tree";
const string COMMIT= "commit";


//ERRORS//

const string NO_COMMAND = "no such command found\n";
const string INIT_FORMAT ="./mygit init is the format \n";


const string HASH_FORMAT ="./mygit hash-object [-w] test.txt is the format \n";
const string HASH_FLAG = "can only contain [-w] as flag\n";

const string CAT_FORMAT ="./mygit cat-file <flag> <file_sha> is the format \n";
const string CAT_FLAG = "can only contain -p, -s, -t flags\n";


const string WRITE_FORMAT ="./mygit write-tree is the format \n";
const string LIST_FORAMT ="./mygit ls-tree [--name-only] <tree_sha> is the format \n";
const string LIST_FLAG = "can only have [--name-only] as flag\n";

const string ADD_FORMAT ="/mygit add . or ./mygit add main.cpp utils.cpp is the format \n";
const string COMMIT_FORMAT ="./mygit commit -m 'Commit message' or ./mygit commit is the format \n";
const string COMMIT_FLAG = "either should not contain a flag only a `.` or shld only contain -m as flag\n";


const string LOG_FORMAT ="./mygit log is the format \n";
const string CHECK_OUT_FORMAT ="./mygit checkout <commit_sha>  is the format \n";

const string PATH_NOT_FOUND = "Error creating directory\n";
const string FILE_NOT_FOUND = "Error opening the file in the path\n";
/*****************************************************************************************************************************************************************************************/

const string BLOB = "file";
const string TREE = "tree";
const string COMMIT_OBJ = "commit";
const string FILE_GIT = "100";
const string OTHERS = "040000";

/*****************************************************************************TREE STRUCTURE**********************************************************************************************/
    //Essentially the sha can be anything like other Tree which again is a sha or a blob which again is a SHA
    //For instance Treea --> subTreeB--->fileA ===> content of Tree is simply Tree + SHA OF TREE
/*****************************************************************************************************************************************************************************************/


class Tree {
public:
    string sha_of_tree;
    string tree_blob;
    vector<TreeEntry> other_trees;

    void calculateTreeSHA() {
        try {
            TreeEntry tree_entry;
            stringstream reqContent;
            for (const auto& entry : other_trees) {
                reqContent << entry.permissions << " " << entry.type << " " << entry.sha << " " << entry.name << "\n";
            }
            string reqContentStr = reqContent.str() + "\0";
            string header = "tree " + to_string(reqContentStr.size());
            // Construct the tree_blob as required for SHA calculation
            tree_blob = header + '\n' + reqContentStr + "\0";
            sha_of_tree = getSHA(tree_blob);
        } catch (const exception& e) {
            cerr << "Error in calculateTreeSHA: " << e.what() << "\n";
            throw;
        }
    }
};
void create_Tree(string& root, Tree& cwd_tree);
/******************************************************************************************************************************************************************************************/

#endif