#include "common.h"
#include <sys/stat.h>
#include <unistd.h>

int FILE_FLAGS = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

string getCurrentTimestamp() {
    time_t now = time(0);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return string(buffer);
};

void clearFile(const string& filePath) {
    ofstream file(filePath, ios::out | ios::trunc);
    if (!file) {
        string msg = "Error in opening the file\n";
    }
    file.close(); 
}


/********************************************************************************************************************************************/
//INIT ADD LOG CHECKOUT //
void execute_init() {
    struct stat INFO;

    //check if Directory alrdy present//
    if (stat(DEFAULT_PATH.c_str(), &INFO) == 0 && S_ISDIR(INFO.st_mode)) {
        displayMessage(ALRDY_EXISTS);
        return;
    }

    //making the hidden dir//
    if (mkdir(DEFAULT_PATH.c_str(), FILE_FLAGS) == -1) {
        displayError(PATH_NOT_FOUND);
    }

    //making object and refs dir//
    if (mkdir(OBJECT_DIR.c_str(), FILE_FLAGS) == -1) {
        displayError(PATH_NOT_FOUND);
    }

    if (mkdir(REFS_DIR.c_str(), FILE_FLAGS) == -1) {
        displayError(PATH_NOT_FOUND);
    }

    //making heads file //
    FILE *fp = fopen(HEAD_FILE.c_str(), "a+");
    if (fp == nullptr) {
        displayError(FILE_NOT_FOUND);
    }
    fclose(fp);

    //making index file//
    FILE *fp2 = fopen(INDEX.c_str(), "a+");
    if (fp == nullptr) {
        displayError(FILE_NOT_FOUND);
    }
    fclose(fp2);

}

void execute_add(vector<string>&params) {
    vector<AddFormat>add_formats(params.size());
    int n = params.size();
    for (int i = 0; i<n; i++) {
        add_formats[i].file_path = params[i];
        string sha = "";
        create_gitObject(params[i], sha, 1);
        add_formats[i].sha = sha;
        add_formats[i].file_per = getFilePermissions(params[i]);
    }
    writeToFile(add_formats, INDEX);
}

void execute_log() {
    string commit_sha = "";
    retrieveHeadSHA(HEAD_FILE, commit_sha);
    displayMessage("Displaying Commit Log\n");
    while(commit_sha.size() != 0) {
        string path = OBJECT_DIR+"/"+commit_sha.substr(0, 2)+"/"+commit_sha.substr(2);
        string content = readBlobFile(path);
        displayMessage(content + "\n");
        commit_sha = extractParentSha(content);
    }
    displayMessage("Process Log Done\n");
}


void restore_file_content(const string& filename, const string& file_sha) {
    string file_path = OBJECT_DIR + "/" + file_sha.substr(0, 2) + "/" + file_sha.substr(2);
    string file_content = readBlobFile(file_path);
    
    // Remove the first string (up to the first space)
    size_t first_space = file_content.find(' ');
    if (first_space != string::npos) {
        file_content.erase(0, first_space + 1);
    }

    // Remove the last three space-separated strings
    size_t last_space = file_content.find_last_of(' ');
    if (last_space != string::npos) {
        size_t second_last_space = file_content.find_last_of(' ', last_space - 1);
        if (second_last_space != string::npos) {
            size_t third_last_space = file_content.find_last_of(' ', second_last_space - 1);
            if (third_last_space != string::npos) {
                file_content.erase(third_last_space);
            }
        }
    }
    // Write the modified content to the output file
    ofstream outfile(filename);
    outfile << file_content;
    outfile.close();
}

void processTree(const string& tree_sha, const string& current_dir, set<string>& restored_files) {
    string tree_path = OBJECT_DIR + "/" + tree_sha.substr(0, 2) + "/" + tree_sha.substr(2);
    string tree_content = readBlobFile(tree_path);

    istringstream stream(tree_content);
    string line;

    while (getline(stream, line)) {
        istringstream lineStream(line);
        string type, content, path;
        string permissions, size;

        lineStream >> permissions >> type >> content >> path;

        // Skip hidden directories
        if (path[0] == '.') {
            continue;
        }

        if (type == "tree") {
            // Create directory path
            string dir_path = current_dir + "/" + path;

            // Create directory if it doesn't exist
            mkdir(dir_path.c_str(), 0755);

            // Process the tree recursively using the actual tree SHA
            processTree(content, dir_path, restored_files);

            // Add the directory to the set of restored files
            restored_files.insert(dir_path);
        } 
        else if (type == "file") {
            // Format: <permissions> file <content> <path> <size>
            lineStream >> path >> size;

            // Skip .o files
            if (path.size() >= 3 && path.substr(path.size() - 2) == ".o") {
                continue;
            }

            // Construct full path for the file
            string full_path = current_dir + "/" + path;

            // Restore the file content using the correct SHA
            restore_file_content(full_path, content);

            // Add the restored file to the set
            restored_files.insert(full_path);
        }
    }
}

void removeUnrestoredFiles(const string& current_dir, const set<string>& restored_files) {
    for (const auto& entry : filesystem::directory_iterator(current_dir)) {
        string file_path = entry.path().string();
        
        // Ignore hidden directories
        if (entry.is_directory() && entry.path().filename().string().front() == '.') {
            continue;
        }

        // If the entry is a directory, recursively remove its contents
        if (entry.is_directory()) {
            // Check if the directory is in the set of restored files
            if (restored_files.find(file_path) == restored_files.end()) {
                filesystem::remove_all(file_path);
            }
        }
        // If the entry is a regular file, check if it needs to be removed
        else if (entry.is_regular_file()) {
            // Skip .o files
            if (file_path.size() >= 3 && file_path.substr(file_path.size() - 2) == ".o") {
                continue;
            }

            if (restored_files.find(file_path) == restored_files.end()) {
                filesystem::remove(file_path);
            }
        }
    }
}

void execute_checkout(vector<string>& params) {
    string sha = params[0];
    string path = OBJECT_DIR + "/" + sha.substr(0, 2) + "/" + sha.substr(2);
    vector<TreeEntry> all_files;
    string content = readBlobFile(path);
    size_t treePos = content.find("tree ");
    string tree_sha;
    if (treePos != string::npos) {
        treePos += 5;
        size_t endPos = content.find('\n', treePos);
        tree_sha = content.substr(treePos, endPos - treePos);
    }

    set<string> restored_files;
    processTree(tree_sha, ".", restored_files);
    
    // Remove files and directories not restored
    removeUnrestoredFiles(".", restored_files);
}
/**********************************************************************************************************************************************/




/********************************************************************************************************************************************/
//HASH  CAT WRITE  LIST COMMIT //
void execute_hash(vector<string>&params) {
    int n = params.size();
    string path = "";
    string flag = "";

    if (n == 1) {
        path = params[0];
    } else {
        flag = params[0];
        path = params[1];
    }
    int req = n == 1 ? 0 : 1;
    string object_sha = "";
    create_gitObject(path, object_sha, req);
    displayMessage(object_sha + "\n");
}

void execute_cat(vector<string>&params) {
    TreeEntry tree_entry;
    string flag = params[0];
    string sha = OBJECT_DIR + "/" + params[1].substr(0, 2) + "/" + params[1].substr(2);
    string blob = readBlobFile(sha);
    handleBlobFlags(blob, tree_entry, flag);
}

void execute_write(vector<string>&params) {
    Tree curr_Tree;
    string path = ".";
    create_Tree(path, curr_Tree);
    displayMessage(curr_Tree.sha_of_tree + "\n");
}

void execute_list(vector<string>&params) {
    int n = params.size();
    string sha = "";
    string flag = "";
    if (n == 1) {
        sha = params[0];
    }
    else { 
        sha = params[1]; flag = params[0]; 
    };
    TreeEntry tree_entry;
    sha = OBJECT_DIR + "/" + sha.substr(0, 2) + "/" + sha.substr(2);
    string blob = readBlobFile(sha);
    tree_entry.type = TREE;
    setBlobContentToTree(blob, tree_entry);
    display_ls_tree(n, tree_entry);
}


void execute_commit(vector<string>& params) {
    ifstream index_file(INDEX);
    if (index_file.peek() == ifstream::traits_type::eof()) {
        displayMessage("Nothing to commit.\n");
        return;
    }

    Tree current_tree;
    string sha = readHeadSHA();
    Commit_Object commit;
    string root = ".";
    create_Tree(root, current_tree);
    commit.message = params.size() == 0 ? DEFAULT_COMMIT_MSG : params[1];
    commit.parent_commit_sha = sha.empty() ? "" : sha;
    commit.tree_sha = current_tree.sha_of_tree;
    commit.timestamp = getCurrentTimestamp();
    commit.author = getenv("USER");
    string commit_blob = commit.formatCommit();
    string commit_path = getSHA(commit_blob);

    // Create a commit object
    string dir_path = OBJECT_DIR + "/" + commit_path.substr(0, 2);
    ensure_directory_exists(dir_path);   
    dir_path = OBJECT_DIR + "/" + commit_path.substr(0, 2) + "/" + commit_path.substr(2);
    create_file(dir_path, commit_blob);
    writeToHEAD(commit_path);
    clearFile(INDEX);
    displayMessage("Current Commit Sha: " + commit_path + "\n");
}
/**********************************************************************************************************************************************/


void additionalCommandHandler(string & command, vector<string>& params, int no_params) {
    if (command == HASH) {
        if (no_params > 2 || no_params < 1) {
            displayError(HASH_FORMAT);
        }
        if (no_params == 2 && params[0] != "-w") {
            displayError(HASH_FLAG);
        }
        execute_hash(params);
        displayMessage("hashed_object successfully\n");

    } else if (command == CAT) {
        if (no_params != 2) {
            displayError(CAT_FORMAT);
        }

        if (params[0] != "-p" && params[0] != "-s" && params[0] != "-t") {
            displayError(CAT_FLAG);
        }
        //execute//
         execute_cat(params);
    } else if (command == WRITE) {
        if (no_params != 0) {
            displayError(WRITE_FORMAT);
        }
        //execute;
         execute_write(params);
         displayMessage("Write tree executed successfully\n");
    } else if (command == LIST) {
        if  (no_params != 1  && no_params != 2) {
            displayError(LIST_FORAMT);
        }
        if (no_params == 2 && params[0] != "--name-only") {
            displayError(LIST_FLAG);
        }
        //execute;
         execute_list(params);
    } else if (command == COMMIT) {
        if (no_params != 0 && no_params != 2) {
            displayError(COMMIT_FORMAT);
        }

        if (no_params == 2 && params[0] != "-m") {
            displayError(COMMIT_FLAG);
        }

        //execute;
         execute_commit(params);
         displayMessage("committed successfully\n");
    } else {
        const string e = command + ":"+ NO_COMMAND;
        displayError(e);
    }
}

void handleCommand(Command& cmd) {
    string command = cmd.command;
    vector<string>params = cmd.params;
    int no_params = params.size();

    if (command == INIT) {
        if (no_params != 0) {
            displayError(INIT_FORMAT);
        }
        //execute;
         execute_init();
         displayMessage("Git Initialized\n");
    } else if (command == ADD) {
        if (no_params < 1 || no_params > 2) {
            displayError(ADD_FORMAT);
        }
        //EXECUTE
         execute_add(params);
         displayMessage("stagged files successfully\n");
    } else if (command == LOG) {
        if (no_params != 0) {
            displayError(LOG_FORMAT);
        }
        //execute;
         execute_log();
    } else if (command == CHECK_OUT) {
        if (no_params != 1) {
            displayError(CHECK_OUT_FORMAT);
        }
        //execute
         execute_checkout(params);
         displayMessage("Successfully checkedout\n");
    } else {
        additionalCommandHandler(command, params, no_params);
    }   
}