#ifndef LS_H
#define LS_H

typedef enum {
    HIDE_DOTFILES,
    SHOW_ALMOST_ALL,
    SHOW_ALL
} HiddenMode;

typedef struct {
    HiddenMode hidden_mode;
    int directory_only;
 int show_inode;
int classify;
} Options;

int parse_options(int argc, char *argv[], Options *options);
int list_directory(const char *path, const Options *options);

#endif