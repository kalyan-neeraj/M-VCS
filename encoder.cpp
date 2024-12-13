#include "common.h"
#include <filesystem>
namespace fs = std::filesystem;
unordered_set<string> visited; 
bool isFile(const string& path) {
    struct stat pathStat;
    if (stat(path.c_str(), &pathStat) != 0) {
        return false;
    }
    return S_ISREG(pathStat.st_mode);
}

bool isDirectory(const string& path) {
    struct stat pathStat;
    if (stat(path.c_str(), &pathStat) != 0) {
        return false;
    }
    return S_ISDIR(pathStat.st_mode);
}

string extractParentSha(string& commitContent) {
    string parent_sha;
    istringstream ss(commitContent);
    string line;
    while (getline(ss, line)) {
        if (line.substr(0, 7) == "parent ") {
            parent_sha = line.substr(7);
            break;
        }
    }
    return parent_sha;
}

string readHeadSHA() {
    const string& headFilePath = HEAD_FILE;
    ifstream headFile(headFilePath);
    if (!headFile.is_open()) {
        string msg = "Error: Could not open HEAD file\n";
        displayError(msg);
        return "";
    }
    string sha;
    getline(headFile, sha);
    headFile.close();
    return sha;
}

bool loadCommitObject(const string& commitFilePath, Commit_Object& commit) {
    ifstream commitFile(commitFilePath);
    if (!commitFile.is_open()) {
       string msg =   "Error: Could not open commit file.\n";
       displayError(msg);
        return false;
    }

    string line;
    while (getline(commitFile, line)) {
        if (line.rfind("tree ", 0) == 0) {
            commit.tree_sha = line.substr(5);
        } else if (line.rfind("parent ", 0) == 0) {
            commit.parent_commit_sha = line.substr(7);
        } else if (line.rfind("author ", 0) == 0) {
            size_t pos = line.find_last_of(" ");
            commit.author = line.substr(7, pos - 7);
            commit.timestamp = line.substr(pos + 1);
        } else if (line.empty()) {
            // The message starts after an empty line
            getline(commitFile, commit.message, '\0');
            break;
        }
    }
    commitFile.close();
    return true;
}

void deleteExistingObject(const string& sha) {
    string directory = OBJECT_DIR + "/" + sha.substr(0, 2);
    
    try {
        // Remove directory and all its contents
        filesystem::remove_all(directory);
    }
    catch (const filesystem::filesystem_error& e) {
        string msg = "Error deleting directory: " + string(e.what()) + "\n";
        displayError(msg);
    }
}

string getFilePermissions(const string& path) {
    struct stat fileStat;

    if (stat(path.c_str(), &fileStat) < 0) {
        return "Error retrieving file permissions.";
    }

    string permissions;
    permissions += ((fileStat.st_mode & S_IRUSR) ? "r" : "-");
    permissions += ((fileStat.st_mode & S_IWUSR) ? "w" : "-");
    permissions += ((fileStat.st_mode & S_IXUSR) ? "x" : "-");
    permissions += ((fileStat.st_mode & S_IRGRP) ? "r" : "-");
    permissions += ((fileStat.st_mode & S_IWGRP) ? "w" : "-");
    permissions += ((fileStat.st_mode & S_IXGRP) ? "x" : "-");
    permissions += ((fileStat.st_mode & S_IROTH) ? "r" : "-");
    permissions += ((fileStat.st_mode & S_IWOTH) ? "w" : "-");
    permissions += ((fileStat.st_mode & S_IXOTH) ? "x" : "-");
    permissions += " (" + to_string((fileStat.st_mode & 0777)) + ")";
    return permissions;
}

void writeToHEAD(const string& sha) {
    ofstream headFile(HEAD_FILE, ios::out | ios::trunc);
    if (!headFile) {
        cerr << "Error: Could not open HEAD file for writing." << endl;
        return;
    }

    // Write the SHA to the HEAD file
    headFile << sha;
    headFile.close();
}


void retrieveHeadSHA(const string& shaFilePath, string& sha) {
    ifstream shaFile(shaFilePath);
    if (!shaFile) {
        cerr << "Error: Could not open file " << shaFilePath << endl;
        return;
    }
    string currentSHA;
    getline(shaFile, currentSHA);
    shaFile.close();
    sha = currentSHA;
}


void processDirectory(const fs::path& dir_path, vector<AddFormat>& add_format) {
    for (const auto& entry : fs::directory_iterator(dir_path)) {
        if (entry.is_directory()) {
            // Recursively process subdirectories
            processDirectory(entry.path(), add_format);
        } else if (entry.is_regular_file()) {
            // Process the regular file
            AddFormat new_entry;
            new_entry.file_path = entry.path().string();
            new_entry.sha = "";
            // new_entry.file_per = fs::status(entry.path()).permissions();

            add_format.push_back(new_entry);
        }
    }
}


void writeToFile(const std::vector<AddFormat>& add_format, const std::string& filename) {
    // First, read existing content
    std::vector<AddFormat> existing_entries;
    std::ifstream infile(filename);
    if (infile.is_open()) {
        std::string line;
        while (std::getline(infile, line)) {
            std::istringstream iss(line);
            AddFormat entry;
            iss >> entry.file_path >> entry.sha >> entry.file_per;
            existing_entries.push_back(entry);
        }
        infile.close();
    }

    // Open file for writing
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        displayError("Error opening file " + filename + "\n");
        return;
    }

    // Process each new entry
    for (const auto& new_entry : add_format) {
        bool found = false;
        
        if (fs::is_directory(new_entry.file_path)) {
            for (const auto& entry : fs::recursive_directory_iterator(new_entry.file_path)) {
                if (entry.path().string().find("./.") == 0) {
                    continue;
                }
                if (isFile(entry.path().string())) {
                    AddFormat nested_entry;
                    nested_entry.file_path = entry.path().string();
                    nested_entry.sha = new_entry.sha;
                    nested_entry.file_per = new_entry.file_per;
                    writeToFile({nested_entry}, filename);
                }
            }
            continue;
        } else if (!isFile(new_entry.file_path)) {
            displayError("Path given is not a file\n");
            return;
        }
        
        // Update existing entries if file path matches
        for (auto& existing : existing_entries) {
            if (existing.file_path == new_entry.file_path) {
                deleteExistingObject(existing.sha);
                existing.sha = new_entry.sha;
                existing.file_per = new_entry.file_per;
                found = true;
                break;
            }
        }

        // If not found, add to existing entries
        if (!found) {
            existing_entries.push_back(new_entry);
        }
    }

    // Write all entries to file
    for (const auto& entry : existing_entries) {
        outfile << entry.file_path << " " << entry.sha << " " << entry.file_per << "\n";
    }

    outfile.close();
    displayMessage("Changes staged successfully\n");
}

void setBlobContentToTree(const string& blobcontent,  TreeEntry& tree_entry) {
    istringstream stream(blobcontent);
    stream >> tree_entry.type;
    if (tree_entry.type == BLOB) {
            stream.get();
            getline(stream, tree_entry.content, ' ');
            getline(stream, tree_entry.fileSize, ' ');
            getline(stream, tree_entry.name, '\0');
    } else if (tree_entry.type == TREE) {
            getline(stream, tree_entry.fileSize, '\n');
            getline(stream, tree_entry.content, '\0');
    }
}

const string getSHA(const string& gitBlob) {
    //Initialize the SHA-1 context
    SHA_CTX sha_ctx;
    SHA1_Init(&sha_ctx);
    
    //Update the SHA-1 context with the blob data
    SHA1_Update(&sha_ctx, gitBlob.data(), gitBlob.size());
    
    //Finalize the SHA-1 hash
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1_Final(hash, &sha_ctx);
    
    //Convert hash to hexadecimal string
    stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        ss << hex << setw(2) << setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

const string getBlobString(string& path) {
    TreeEntry tree_entry;
    
    // Extract the file name from the path
    size_t pos = path.find_last_of("/\\");
    tree_entry.name = (pos != string::npos) ? path.substr(pos + 1) : path;
    
    ifstream file(path, ifstream::binary);
    if (!file) {
        const string msg = "File not found: " + path + "\n";
        displayError(msg + "\n");
        return "";
    }

    // Get the file size
    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);

    // Read file content into a string
    string fileContent(fileSize, '\0');
    file.read(&fileContent[0], fileSize);
    tree_entry.content = fileContent;
    tree_entry.fileSize = fileSize;
    tree_entry.type = BLOB;

    // Create blob string with file size, null character, content, and file name
    stringstream blobStream;
    blobStream << tree_entry.type << ' ' << fileContent << ' ' <<fileSize <<' ' << tree_entry.name <<' ' <<"\0";
    return blobStream.str();
}


const string readBlobFile(const string& path) {
    ifstream file(path, ifstream::binary);
    if (!file) {
        const string msg = FILE_NOT_FOUND + " Path given: " + path + "\n";
        displayError(msg);
        return "";
    }

    // Get file size
    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);

    // Read the entire blob content
    string blobContent(fileSize, '\0');
    file.read(&blobContent[0], fileSize);

    return blobContent;
}

void display_ls_tree(int n, TreeEntry& tree_entry) {
    if (n == 1) {
        displayMessage(tree_entry.content + "\n");
    } else {
        istringstream stream(tree_entry.content);
        string line;
        while (getline(stream, line)) {
            size_t last_space = line.find_last_of(' ');
            if (last_space != string::npos) {
                displayMessage(line.substr(last_space + 1) + "\n");
            }
        }
    }
}

void handleBlobFlags(const string& blobContent, TreeEntry& tree_entry, const string& flag) {
    setBlobContentToTree(blobContent, tree_entry);
    if (flag == "-p") {
        displayMessage(tree_entry.content + "\n");
    } else if (flag == "-s") {
        displayMessage(tree_entry.fileSize + "\n");
    } else if (flag == "-t") {
        displayMessage(tree_entry.type + "\n");
    }
}

void ensure_directory_exists(string &dir_path) {
    struct stat st;
    if (stat(dir_path.c_str(), &st) != 0) {
        mkdir(dir_path.c_str(), 0755);
    }
}

void create_file(string& full_path, const string& gitBlob) {
    ofstream out(full_path);
    if (out) {
        out << gitBlob;
        out.close();
    } else {
        displayError("Error writing to file: " + full_path + "\n");
    }
}


void create_gitObject(string& full_path, string& object_sha, int req) {
    bool is_dir = false;
    bool is_file = false;
    is_file = isFile(full_path);
    is_dir = isDirectory(full_path);
    if (is_file && !is_dir) {
        string gitBlob = getBlobString(full_path);
        string blob_sha = getSHA(gitBlob);
        if (req) {
            string dir_path =  OBJECT_DIR +"/" +blob_sha.substr(0, 2);
            ensure_directory_exists(dir_path);
            dir_path = dir_path +"/" + blob_sha.substr(2); 
            create_file(dir_path, gitBlob);
        }
        object_sha = blob_sha;
    } else if (!is_file && is_dir) {
        Tree cw_tree;
        create_Tree(full_path, cw_tree);
        object_sha = cw_tree.sha_of_tree;
    } else {
        displayError(full_path + " " + "Not Valid\n");
    }
}

void create_TreeObject(Tree& tree) {
        string dir_path =  OBJECT_DIR +"/" +tree.sha_of_tree.substr(0, 2);
        ensure_directory_exists(dir_path);
        dir_path = dir_path +"/" + tree.sha_of_tree.substr(2); 
        create_file(dir_path, tree.tree_blob);
}

void create_Tree(string& root, Tree& cwd_tree) {
    //Get all entries in the current directory
    if (visited.find(root) != visited.end()) {
        return;
    }
    visited.insert(root);
    DIR* dir = opendir(root.c_str());
    if (!dir) {
        displayError("cant open the dir : " + root + "\n");
    }
    struct dirent* entry;
    vector<TreeEntry> entries;
        while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;
        if (name[0] == '.' || name == ".." || name == ".mygit") {
            continue;
        }

        TreeEntry tree_entry;
        tree_entry.name = name;
        string full_path = root + "/" + name;
        struct stat st;
        if (stat(full_path.c_str(), &st) == -1) {
            continue;
        }

        if (S_ISREG(st.st_mode)) {
            stringstream permissions;
            permissions << oct << (st.st_mode & 0777);
            tree_entry.permissions = "100" + permissions.str();
            string file_sha = "";
            create_gitObject(full_path, file_sha, 1);
            tree_entry.sha = file_sha;
            tree_entry.type = BLOB;
        } else if (S_ISDIR(st.st_mode)) {
            //Handle directory: recursively build tree
            stringstream permissions;
            permissions << oct << (st.st_mode & 0777);
            tree_entry.permissions = "040000";
            tree_entry.type = TREE;
            Tree subtree;
            create_Tree(full_path, subtree);
            subtree.calculateTreeSHA();
            tree_entry.sha = subtree.sha_of_tree;
            create_TreeObject(subtree);
        }
        entries.emplace_back(tree_entry);
    }
    closedir(dir);
    sort(entries.begin(), entries.end(), [](const TreeEntry& a, const TreeEntry& b) {
        return a.name < b.name;
    });
    cwd_tree.other_trees.insert(cwd_tree.other_trees.end(), entries.begin(), entries.end());
    cwd_tree.calculateTreeSHA();
    create_TreeObject(cwd_tree);
}


