#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include "ls.h"

static int visible(const char *name, const Options *options)
{
    if (options->hidden_mode == SHOW_ALL) return 1;
    if (name[0] != '.') return 1;
    if (options->hidden_mode == SHOW_ALMOST_ALL &&
        strcmp(name, ".") != 0 && strcmp(name, "..") != 0)
        return 1;
    return 0;
}

int list_directory(const char *path, const Options *options)
{
    DIR *directory = opendir(path);
    struct dirent *entry;

    if (directory == NULL) {
        perror(path);
        return 1;
    }

    while ((entry = readdir(directory)) != NULL) {
      if (visible(entry->d_name, options)) {
    if (options->show_inode)
        printf("%llu %s\n",
               (unsigned long long)entry->d_fileno, entry->d_name);
    else
        puts(entry->d_name);
}
    }

    if (closedir(directory) != 0) {
        perror(path);
        return 1;
    }

    return 0;
}