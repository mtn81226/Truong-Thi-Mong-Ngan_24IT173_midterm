#include <stdio.h>
#include <unistd.h>
#include "ls.h"

int parse_options(int argc, char *argv[], Options *options)
{
    int option;

    opterr = 0;
    while ((option = getopt(argc, argv, "aAfdiF")) != -1) {
        switch (option) {
        case 'a':
            options->hidden_mode = SHOW_ALL;
            break;
        case 'A':
            options->hidden_mode = SHOW_ALMOST_ALL;
            break;
        case 'f':
            options->hidden_mode = SHOW_ALL;
            break;
        case 'd':
            options->directory_only = 1;
            break;
case 'i':
    options->show_inode = 1;
    break;
case 'F':
    options->classify = 1;
    break;
        default:
            fprintf(stderr, "%s: unsupported option -%c\n",
                    argv[0], optopt);
            return -1;
        }
    }

    return optind;
}