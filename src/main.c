#include <stdio.h>
#include <unistd.h>
#include "ls.h"

int main(int argc, char *argv[])
{
options.classify = 0;
    Options options;
    options.hidden_mode =
        (getuid() == 0) ? SHOW_ALMOST_ALL : HIDE_DOTFILES;
    options.directory_only = 0;
options.show_inode = 0;

    int first_operand = parse_options(argc, argv, &options);

    if (first_operand < 0 || argc - first_operand > 1) {
        fprintf(stderr, "Usage: %s [-aAfd] [directory]\n", argv[0]);
        return 2;
    }

    const char *path =
        (first_operand < argc) ? argv[first_operand] : ".";

    if (options.directory_only) {
        puts(path);
        return 0;
    }

    return list_directory(path, &options);
}