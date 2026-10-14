# Simplified `ls(1)` - Midterm Project

Student: Truong Thi Mong Ngan  
Student ID: 24IT173

Repository: https://github.com/mtn81226/Truong-Thi-Mong-Ngan_24IT173_midterm

## Description

This project implements a simplified directory listing utility in C, based on
the supplied NetBSD 10.1 `ls(1)` manual page. It uses directory and filesystem
interfaces such as `opendir`, `readdir`, `stat`, and `lstat`.

The code is split into a command-line entry point, option parsing, and directory
and file listing modules. `include/ls.h` defines shared option types, while
`include/options.h` and `include/directory.h` declare the module interfaces.

## Implemented options

The program supports the options listed in the supplied manual:

| Option | Behavior |
| --- | --- |
| `-A` | Show hidden entries except `.` and `..`; enabled by default for the superuser. |
| `-a` | Show all entries, including `.` and `..`. |
| `-c` | Use status-change time with `-t` or `-l`; overridden by a later `-u`. |
| `-d` | List directory operands as entries instead of opening them. |
| `-F` | Add file-type suffixes, including `/`, `*`, `@`, `%`, `=`, and `|` where supported. |
| `-f` | Preserve directory order returned by the filesystem instead of sorting. |
| `-h` | Show human-readable sizes with `-l` and `-s`; overridden by a later `-k`. |
| `-i` | Show each entry's inode number. |
| `-k` | Display `-s` block counts in kilobytes; a later `-h` overrides it. |
| `-l` | Show long format with mode, links, owner, group, size, time, and name. |
| `-n` | Long format with numeric owner and group IDs; overrides `-l` formatting. |
| `-q` | Replace non-printable filename characters with `?`. |
| `-R` | Recursively list subdirectories; `-d` and `-R` follow the last option. |
| `-r` | Reverse the selected sort order. |
| `-S` | Sort by size, largest first. |
| `-s` | Show allocated filesystem blocks for each entry. |
| `-t` | Sort by the selected timestamp, newest first. |
| `-u` | Use access time with `-t` or `-l`; overridden by a later `-c`. |
| `-w` | Print filename bytes without replacing non-printable characters. |

By default, entries are sorted lexicographically. `-c` and `-u`, `-h` and
`-k`, `-l` and `-n`, and `-q` and `-w` use the last option specified. The
default filename behavior depends on whether standard output is a terminal.

## Build

Run these commands on NetBSD 10.1 or another POSIX-like system with a C compiler
and `make`:

```sh
make
```

To remove the executable and object files:

```sh
make clean
```

## Run

```sh
./myls
./myls -A demo
./myls -iF .
./myls -lns /tmp
./myls -R -l .
```

If an unsupported option is provided, the program prints its accepted command
syntax and exits with a nonzero status.

## Submission notes

The `.gitignore` excludes the `myls` executable, object files, and common
temporary files. Commit the source files, `Makefile`, this report, and
`.gitignore`; do not commit compiled output.
