# Mini Version Control System (VCS)

## Overview
This project is a mini version control system that mimics basic functionalities of Git. It allows users to initialize a repository, add files, commit changes, and checkout specific commits. The system uses the SHA-1 hashing algorithm for integrity checks and supports file compression.

## Features
- **Initialize Repository**: Create a hidden `.mygit` directory for tracking changes.
- **Hash-Object**: Calculate the SHA-1 hash of a file and optionally store it as a blob object.
- **Cat-File**: Display the content or metadata of a file using its SHA-1 hash.
- **Write Tree**: Capture the current directory structure and store it as a tree object.
- **List Tree**: List contents of a tree object with detailed information or just names.
- **Add Files**: Stage files or directories for the next commit.
- **Commit Changes**: Create a commit object representing the staged changes.
- **Log Command**: Display the commit history.
- **Checkout Command**: Restore the state of the project to a specific commit.

## Getting Started

### Prerequisites
- C/C++ compiler
- C++17 or higher

### Installation
1. Clone the repository or download the ZIP file.
2. Navigate to the project directory.
3. Run `make` to compile the code.

## Commands
```
make clean
```

```
make
```

### Usage
## Commands

### Initialize Repository
```bash
./mygit init
```

### Hash Object
```bash
echo -n "hello world" > test.txt
./mygit hash-object [-w] test.txt
```

### Cat File
```bash
./mygit cat-file <flag> <file_sha>
```

### Write Tree
```bash
./mygit write-tree
```

### List Tree
```bash
./mygit ls-tree [--name-only] <tree_sha>
```

### Add Files
```bash
./mygit add <file_name>
```

### Add All Files
```bash
./mygit add .
```

### Commit Changes
```bash
./mygit commit -m "Your commit message"
```

### View Log
```bash
./mygit log
```

### Checkout a Commit
```bash
./mygit checkout <commit_sha>
```

### Additional Commands
To view the content of a file by its SHA-1:
```bash
./mygit cat-file -p <file_sha>
```

To list the contents of a tree:
```bash
./mygit ls-tree <tree_sha>
```


