# CryptoShredder

**CryptoShredder** is a simple command-line tool written in C for securely shredding files and directories using XOR encryption with a randomly generated key. It can also optionally delete files and folders after shredding.

## Features

* Overwrites file contents using random XOR-based obfuscation.
* Recursively processes directories.
* Optionally deletes shredded files and folders.
* Uses `mmap` for efficient file manipulation.
* Leverages `/dev/urandom` for secure key generation.

## Compilation

Compile with `gcc`:

```bash
gcc -o cryptoshredder cryptoshredder.c
```

## Usage

```bash
./cryptoshredder -f <file>        # Shred a single file
./cryptoshredder -d <directory>   # Shred all files in a directory
./cryptoshredder -frm <file>      # Shred and delete a single file
./cryptoshredder -drm <directory> # Shred and delete all files in a directory
```

## Notes

* Only regular files are processed; symlinks and special files are ignored.
* Files are overwritten with pseudo-random XOR data but not multiple passes or pattern-specific erasure.
* Make sure you have write permissions on the target files/directories.
* This tool is **not guaranteed to meet government or military data destruction standards**.

## Disclaimer

Use at your own risk. Once shredded and deleted, data recovery is practically impossible.

