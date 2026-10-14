#define _NETBSD_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>

#include "options.h"

int parse_options(int argc, char *argv[], Options *options)
{
    int option;

    options->hidden_mode = (geteuid() == 0) ? SHOW_ALMOST_ALL : HIDE_DOTFILES;
    options->sort_mode = SORT_BY_NAME;
    options->time_mode = TIME_MODIFIED;
    options->size_mode = SIZE_DEFAULT;
    options->name_mode = NAMES_DEFAULT;
    options->directories_as_files = 0;
    options->recursive = 0;
    options->unsorted = 0;
    options->classify = 0;
    options->show_inode = 0;
    options->long_format = 0;
    options->numeric_ids = 0;
    options->show_blocks = 0;
    options->reverse = 0;

    /* The last flag in each documented pair determines the active behavior. */
    opterr = 0;
    optind = 1;
    while ((option = getopt(argc, argv, "AacdFfhiklnqRrSstuw")) != -1) {
        switch (option) {
        case 'A':
            options->hidden_mode = SHOW_ALMOST_ALL;
            break;
        case 'a':
            options->hidden_mode = SHOW_ALL;
            break;
        case 'c':
            options->time_mode = TIME_CHANGED;
            break;
        case 'd':
            options->directories_as_files = 1;
            options->recursive = 0;
            break;
        case 'F':
            options->classify = 1;
            break;
        case 'f':
            /* The supplied manual defines -f as disabling sorting only. */
            options->unsorted = 1;
            break;
        case 'h':
            options->size_mode = SIZE_HUMAN;
            break;
        case 'i':
            options->show_inode = 1;
            break;
        case 'k':
            options->size_mode = SIZE_KILOBYTES;
            break;
        case 'l':
            options->long_format = 1;
            options->numeric_ids = 0;
            break;
        case 'n':
            options->long_format = 1;
            options->numeric_ids = 1;
            break;
        case 'q':
            options->name_mode = NAMES_QUOTED;
            break;
        case 'R':
            options->directories_as_files = 0;
            options->recursive = 1;
            break;
        case 'r':
            options->reverse = 1;
            break;
        case 'S':
            options->sort_mode = SORT_BY_SIZE;
            break;
        case 's':
            options->show_blocks = 1;
            break;
        case 't':
            options->sort_mode = SORT_BY_TIME;
            break;
        case 'u':
            options->time_mode = TIME_ACCESSED;
            break;
        case 'w':
            options->name_mode = NAMES_RAW;
            break;
        default:
            return -1;
        }
    }

    return optind;
}

void print_usage(const char *program_name)
{
    fprintf(stderr,
            "Usage: %s [-AacdFfhiklnqRrSstuw] [file ...]\n",
            program_name);
}
