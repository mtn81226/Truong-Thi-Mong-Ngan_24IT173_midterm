#ifndef LS_H
#define LS_H

typedef enum {
    HIDE_DOTFILES,
    SHOW_ALMOST_ALL,
    SHOW_ALL
} HiddenMode;

typedef enum {
    SORT_BY_NAME,
    SORT_BY_SIZE,
    SORT_BY_TIME,
    SORT_DISABLED
} SortMode;

typedef enum {
    TIME_MODIFIED,
    TIME_CHANGED,
    TIME_ACCESSED
} TimeMode;

typedef enum {
    SIZE_DEFAULT,
    SIZE_KILOBYTES,
    SIZE_HUMAN
} SizeMode;

typedef enum {
    NAMES_DEFAULT,
    NAMES_QUOTED,
    NAMES_RAW
} NameMode;

typedef struct {
    HiddenMode hidden_mode;
    SortMode sort_mode;
    TimeMode time_mode;
    SizeMode size_mode;
    NameMode name_mode;
    int directories_as_files;
    int recursive;
    int unsorted;
    int classify;
    int show_inode;
    int long_format;
    int numeric_ids;
    int show_blocks;
    int reverse;
} Options;

#endif
