# Zip Utility

A command-line zip utility written in C that supports zipping, unzipping, and rezipping operations.

## Features

- **Zip files and folders**: Create zip archives from multiple files and directories
- **Unzip archives**: Extract zip archives to the current directory
- **Rezip functionality**: Unzip an archive, add new files, and rezip it

## Requirements

- GCC compiler
- `zip` command-line utility (usually pre-installed on most Linux systems)
- `unzip` command-line utility (usually pre-installed on most Linux systems)

### Installing zip/unzip (if needed)

**Ubuntu/Debian:**
```bash
sudo apt-get install zip unzip
```

**macOS:**
```bash
brew install zip unzip
```

**Fedora/RHEL:**
```bash
sudo dnf install zip unzip
```

## Compilation

```bash
make
```

## Usage

### Basic Usage

```bash
./zip_util [OPTIONS] <files/folders...>
```

### Options

- `-o <output.zip>` - Specify output zip file (default: archive.zip)
- `-u <archive.zip>` - Unzip the specified archive
- `-r <archive.zip> <files...>` - Rezip: unzip archive, add files, then rezip
- `-h` - Show help message

### Examples

**Zip files:**
```bash
./zip_util file1.txt file2.txt
# Creates archive.zip with file1.txt and file2.txt
```

**Zip with custom output name:**
```bash
./zip_util -o myarchive.zip file1.txt folder/
# Creates myarchive.zip with file1.txt and all contents of folder/
```

**Zip a directory:**
```bash
./zip_util -o backup.zip myfolder/
# Creates backup.zip containing all files in myfolder/
```

**Unzip an archive:**
```bash
./zip_util -u archive.zip
# Extracts all files from archive.zip to current directory
```

**Rezip (add files to existing archive):**
```bash
./zip_util -r archive.zip newfile.txt newfile2.txt
# Unzips archive.zip, adds newfile.txt and newfile2.txt, then rezips
```

## How It Works

### Zip Operation
The utility recursively traverses directories and adds all files to the zip archive while preserving the directory structure.

### Unzip Operation
Extracts all files from the archive, creating necessary directories to maintain the original structure.

### Rezip Operation
1. Extracts the existing archive to a temporary directory
2. Copies the new files to the temporary directory
3. Creates a new archive with all files (original + new)
4. Replaces the original archive with the new one

## Clean Up

```bash
make clean
```

## Installation

To install system-wide (requires root/sudo):

```bash
sudo make install
```

This installs the utility to `/usr/local/bin/zip_util`.
