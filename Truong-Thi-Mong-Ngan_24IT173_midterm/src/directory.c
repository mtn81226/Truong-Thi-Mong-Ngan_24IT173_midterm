#define _NETBSD_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#include "directory.h"

typedef struct {
    char *name;
    char *path;
    struct stat info;
    int has_info;
} Entry;

static const Options *active_sort_options;

static int is_visible(const char *name, const Options *options)
{
    if (options->hidden_mode == SHOW_ALL)
        return 1;
    if (name[0] != '.')
        return 1;
    if (options->hidden_mode == SHOW_ALMOST_ALL &&
        strcmp(name, ".") != 0 && strcmp(name, "..") != 0)
        return 1;
    return 0;
}

static char *join_path(const char *directory, const char *name)
{
    size_t directory_length = strlen(directory);
    size_t name_length = strlen(name);
    int need_separator = directory_length > 0 &&
                         directory[directory_length - 1] != '/';
    size_t total_length;
    char *result;

    if (directory_length > SIZE_MAX - name_length - 2) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    total_length = directory_length + (size_t)need_separator + name_length + 1;
    result = malloc(total_length);
    if (result == NULL)
        return NULL;

    memcpy(result, directory, directory_length);
    if (need_separator)
        result[directory_length++] = '/';
    memcpy(result + directory_length, name, name_length + 1);
    return result;
}

static void free_entries(Entry *entries, size_t count)
{
    size_t index;

    for (index = 0; index < count; index++) {
        free(entries[index].name);
        free(entries[index].path);
    }
    free(entries);
}

static int append_entry(Entry **entries, size_t *count, size_t *capacity,
                        Entry *entry)
{
    Entry *grown;
    size_t new_capacity;

    if (*count == *capacity) {
        new_capacity = (*capacity == 0) ? 16 : *capacity * 2;
        if (new_capacity < *capacity ||
            new_capacity > SIZE_MAX / sizeof(**entries)) {
            errno = ENOMEM;
            return -1;
        }
        grown = realloc(*entries, new_capacity * sizeof(**entries));
        if (grown == NULL)
            return -1;
        *entries = grown;
        *capacity = new_capacity;
    }

    (*entries)[*count] = *entry;
    (*count)++;
    return 0;
}

static time_t selected_time(const struct stat *info, TimeMode mode)
{
    if (mode == TIME_CHANGED)
        return info->st_ctime;
    if (mode == TIME_ACCESSED)
        return info->st_atime;
    return info->st_mtime;
}

static int compare_entries(const void *left_pointer, const void *right_pointer)
{
    const Entry *left = left_pointer;
    const Entry *right = right_pointer;
    int result = 0;

    if (active_sort_options->sort_mode == SORT_BY_SIZE &&
        left->has_info && right->has_info) {
        if (left->info.st_size > right->info.st_size)
            result = -1;
        else if (left->info.st_size < right->info.st_size)
            result = 1;
    } else if (active_sort_options->sort_mode == SORT_BY_TIME &&
               left->has_info && right->has_info) {
        time_t left_time = selected_time(&left->info,
                                         active_sort_options->time_mode);
        time_t right_time = selected_time(&right->info,
                                          active_sort_options->time_mode);
        if (left_time > right_time)
            result = -1;
        else if (left_time < right_time)
            result = 1;
    }

    if (result == 0)
        result = strcmp(left->name, right->name);
    if (active_sort_options->reverse) {
        if (result < 0)
            return 1;
        if (result > 0)
            return -1;
    }
    return result;
}

static void sort_entries(Entry *entries, size_t count, const Options *options)
{
    if (options->unsorted || options->sort_mode == SORT_DISABLED || count < 2)
        return;

    active_sort_options = options;
    qsort(entries, count, sizeof(*entries), compare_entries);
}

static int should_quote_names(const Options *options)
{
    if (options->name_mode == NAMES_QUOTED)
        return 1;
    if (options->name_mode == NAMES_RAW)
        return 0;
    return isatty(STDOUT_FILENO);
}

/* Preserve printable multibyte characters; replace other bytes with '?'. */
static void print_name(const char *name, int quote_nonprintable)
{
    const char *cursor = name;
    size_t remaining = strlen(name);
    mbstate_t state;

    if (!quote_nonprintable) {
        fputs(name, stdout);
        return;
    }

    memset(&state, 0, sizeof(state));
    while (remaining > 0) {
        wchar_t character;
        size_t width = mbrtowc(&character, cursor, remaining, &state);

        if (width == (size_t)-1 || width == (size_t)-2) {
            putchar('?');
            cursor++;
            remaining--;
            memset(&state, 0, sizeof(state));
        } else if (width == 0) {
            break;
        } else {
            if (iswprint(character))
                (void)fwrite(cursor, 1, width, stdout);
            else
                putchar('?');
            cursor += width;
            remaining -= width;
        }
    }
}

static uintmax_t allocated_bytes(const struct stat *info)
{
    if (info->st_blocks <= 0)
        return 0;
    return (uintmax_t)info->st_blocks * 512U;
}

static uintmax_t parse_block_size(void)
{
    const char *value = getenv("BLOCKSIZE");
    char *end;
    unsigned long long number;
    uintmax_t multiplier = 1;

    if (value == NULL || *value == '\0')
        return 512;

    number = strtoull(value, &end, 10);
    if (end == value || number == 0)
        return 512;

    if (*end != '\0') {
        if (end[1] != '\0')
            return 512;
        switch (*end) {
        case 'k': case 'K': multiplier = 1024U; break;
        case 'm': case 'M': multiplier = 1024U * 1024U; break;
        case 'g': case 'G': multiplier = 1024U * 1024U * 1024U; break;
        default: return 512;
        }
    }

    if ((uintmax_t)number > UINTMAX_MAX / multiplier)
        return 512;
    return (uintmax_t)number * multiplier;
}

static uintmax_t display_block_size(const Options *options)
{
    if (options->show_blocks && options->size_mode == SIZE_KILOBYTES)
        return 1024;
    return parse_block_size();
}

static uintmax_t rounded_units(uintmax_t bytes, uintmax_t unit)
{
    return bytes / unit + (bytes % unit != 0);
}

static void format_human_size(uintmax_t bytes, char *buffer, size_t size)
{
    static const char *units[] = {"B", "K", "M", "G", "T", "P", "E"};
    double value = (double)bytes;
    size_t unit = 0;

    while (value >= 1024.0 && unit + 1 < sizeof(units) / sizeof(units[0])) {
        value /= 1024.0;
        unit++;
    }
    if (unit == 0)
        (void)snprintf(buffer, size, "%" PRIuMAX "B", bytes);
    else
        (void)snprintf(buffer, size, "%.1f%s", value, units[unit]);
}

static const char *entry_suffix(const Entry *entry)
{
    struct stat link_info;
    const struct stat *info = &entry->info;

    if (lstat(entry->path, &link_info) == 0)
        info = &link_info;

    if (S_ISDIR(info->st_mode))
        return "/";
    if (S_ISLNK(info->st_mode))
        return "@";
#ifdef S_ISWHT
    if (S_ISWHT(info->st_mode))
        return "%";
#endif
    if (S_ISFIFO(info->st_mode))
        return "|";
    if (S_ISSOCK(info->st_mode))
        return "=";
    if (S_ISREG(info->st_mode) &&
        (info->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0)
        return "*";
    return "";
}

static char file_type(mode_t mode)
{
    if (S_ISDIR(mode)) return 'd';
    if (S_ISLNK(mode)) return 'l';
    if (S_ISBLK(mode)) return 'b';
    if (S_ISCHR(mode)) return 'c';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISSOCK(mode)) return 's';
#ifdef S_ISWHT
    if (S_ISWHT(mode)) return 'w';
#endif
    return '-';
}

static void mode_string(mode_t mode, char output[11])
{
    output[0] = file_type(mode);
    output[1] = (mode & S_IRUSR) ? 'r' : '-';
    output[2] = (mode & S_IWUSR) ? 'w' : '-';
    output[3] = (mode & S_ISUID) ? ((mode & S_IXUSR) ? 's' : 'S') :
                ((mode & S_IXUSR) ? 'x' : '-');
    output[4] = (mode & S_IRGRP) ? 'r' : '-';
    output[5] = (mode & S_IWGRP) ? 'w' : '-';
    output[6] = (mode & S_ISGID) ? ((mode & S_IXGRP) ? 's' : 'S') :
                ((mode & S_IXGRP) ? 'x' : '-');
    output[7] = (mode & S_IROTH) ? 'r' : '-';
    output[8] = (mode & S_IWOTH) ? 'w' : '-';
    if (mode & S_ISVTX)
        output[9] = (mode & S_IXOTH) ? 't' : 'T';
    else
        output[9] = (mode & S_IXOTH) ? 'x' : '-';
    output[10] = '\0';
}

static time_t entry_time(const struct stat *info, const Options *options)
{
    return selected_time(info, options->time_mode);
}

static void format_time(time_t value, char *buffer, size_t size)
{
    struct tm local_time;
    time_t now = time(NULL);
    double difference = difftime(now, value);
    const char *format;

    if (localtime_r(&value, &local_time) == NULL) {
        (void)snprintf(buffer, size, "??? ?? ??:??");
        return;
    }

    /* Recent timestamps use a clock; older or future ones use a year. */
    if (difference > 15552000.0 || difference < -3600.0)
        format = "%b %e  %Y";
    else
        format = "%b %e %H:%M";
    if (strftime(buffer, size, format, &local_time) == 0)
        (void)snprintf(buffer, size, "??? ?? ??:??");
}

static char *read_link_target(const char *path)
{
    size_t capacity = 256;

    while (capacity <= 1048576) {
        char *buffer = malloc(capacity + 1);
        ssize_t length;

        if (buffer == NULL)
            return NULL;
        length = readlink(path, buffer, capacity);
        if (length < 0) {
            free(buffer);
            return NULL;
        }
        if ((size_t)length < capacity) {
            buffer[length] = '\0';
            return buffer;
        }
        free(buffer);
        capacity *= 2;
    }

    errno = ENAMETOOLONG;
    return NULL;
}

static void print_long_entry(const Entry *entry, const Options *options,
                             int quote_names)
{
    char permissions[11];
    char size_buffer[64];
    char date_buffer[64];
    char owner_buffer[64];
    char group_buffer[64];
    const char *owner;
    const char *group;
    struct passwd *password;
    struct group *group_entry;
    const char *suffix = options->classify ? entry_suffix(entry) : "";

    if (!entry->has_info) {
        print_name(entry->name, quote_names);
        puts(suffix);
        return;
    }

    mode_string(entry->info.st_mode, permissions);
    if (options->numeric_ids) {
        (void)snprintf(owner_buffer, sizeof(owner_buffer), "%ju",
                       (uintmax_t)entry->info.st_uid);
        (void)snprintf(group_buffer, sizeof(group_buffer), "%ju",
                       (uintmax_t)entry->info.st_gid);
        owner = owner_buffer;
        group = group_buffer;
    } else {
        password = getpwuid(entry->info.st_uid);
        group_entry = getgrgid(entry->info.st_gid);
        (void)snprintf(owner_buffer, sizeof(owner_buffer), "%s",
                       password != NULL ? password->pw_name : "?");
        (void)snprintf(group_buffer, sizeof(group_buffer), "%s",
                       group_entry != NULL ? group_entry->gr_name : "?");
        owner = owner_buffer;
        group = group_buffer;
    }

    if (options->size_mode == SIZE_HUMAN) {
        format_human_size((uintmax_t)entry->info.st_size,
                          size_buffer, sizeof(size_buffer));
    } else if (S_ISCHR(entry->info.st_mode) || S_ISBLK(entry->info.st_mode)) {
        (void)snprintf(size_buffer, sizeof(size_buffer), "%ju,%ju",
                       (uintmax_t)major(entry->info.st_rdev),
                       (uintmax_t)minor(entry->info.st_rdev));
    } else {
        (void)snprintf(size_buffer, sizeof(size_buffer), "%ju",
                       (uintmax_t)entry->info.st_size);
    }
    format_time(entry_time(&entry->info, options), date_buffer,
                sizeof(date_buffer));

    printf("%s %3ju %-8s %-8s %8s %s ", permissions,
           (uintmax_t)entry->info.st_nlink, owner, group, size_buffer,
           date_buffer);
    print_name(entry->name, quote_names);
    fputs(suffix, stdout);

    if (S_ISLNK(entry->info.st_mode)) {
        char *target = read_link_target(entry->path);
        if (target != NULL) {
            fputs(" -> ", stdout);
            print_name(target, quote_names);
            free(target);
        }
    }
    putchar('\n');
}

static void print_entry(const Entry *entry, const Options *options,
                        int quote_names)
{
    if (options->show_inode) {
        uintmax_t inode = entry->has_info ?
                          (uintmax_t)entry->info.st_ino : 0;
        printf("%ju ", inode);
    }

    if (options->show_blocks) {
        uintmax_t bytes = entry->has_info ? allocated_bytes(&entry->info) : 0;
        if (options->size_mode == SIZE_HUMAN) {
            char human[64];
            format_human_size(bytes, human, sizeof(human));
            printf("%s ", human);
        } else {
            printf("%ju ", rounded_units(bytes, display_block_size(options)));
        }
    }

    if (options->long_format) {
        print_long_entry(entry, options, quote_names);
        return;
    }

    print_name(entry->name, quote_names);
    if (options->classify)
        fputs(entry_suffix(entry), stdout);
    putchar('\n');
}

static void print_directory_total(const Entry *entries, size_t count,
                                  const Options *options)
{
    uintmax_t total_bytes = 0;
    uintmax_t total_blocks = 0;
    uintmax_t unit = display_block_size(options);
    size_t index;

    for (index = 0; index < count; index++) {
        if (entries[index].has_info) {
            uintmax_t bytes = allocated_bytes(&entries[index].info);
            total_bytes += bytes;
            total_blocks += rounded_units(bytes, unit);
        }
    }

    if (options->size_mode == SIZE_HUMAN && options->show_blocks) {
        char human[64];
        format_human_size(total_bytes, human, sizeof(human));
        printf("total %s\n", human);
    } else {
        printf("total %ju\n", total_blocks);
    }
}

static int list_directory(const char *path, const Options *options,
                          int show_header, int blank_before_header)
{
    DIR *directory;
    struct dirent *directory_entry;
    Entry *entries = NULL;
    size_t count = 0;
    size_t capacity = 0;
    size_t index;
    int status = 0;
    int quote_names = should_quote_names(options);

    directory = opendir(path);
    if (directory == NULL) {
        perror(path);
        return 1;
    }

    for (;;) {
        Entry item;

        errno = 0;
        directory_entry = readdir(directory);
        if (directory_entry == NULL) {
            if (errno != 0) {
                perror(path);
                status = 1;
            }
            break;
        }
        if (!is_visible(directory_entry->d_name, options))
            continue;

        memset(&item, 0, sizeof(item));
        item.name = strdup(directory_entry->d_name);
        item.path = join_path(path, directory_entry->d_name);
        if (item.name == NULL || item.path == NULL) {
            free(item.name);
            free(item.path);
            fprintf(stderr, "%s: out of memory\n", path);
            status = 1;
            break;
        }
        if (lstat(item.path, &item.info) == 0) {
            item.has_info = 1;
        } else {
            fprintf(stderr, "%s: %s\n", item.path, strerror(errno));
            status = 1;
        }
        if (append_entry(&entries, &count, &capacity, &item) != 0) {
            free(item.name);
            free(item.path);
            fprintf(stderr, "%s: out of memory\n", path);
            status = 1;
            break;
        }
    }

    if (closedir(directory) != 0) {
        perror(path);
        status = 1;
    }

    sort_entries(entries, count, options);
    if (show_header) {
        if (blank_before_header)
            putchar('\n');
        printf("%s:\n", path);
    }

    if (options->long_format ||
        (options->show_blocks && isatty(STDOUT_FILENO)))
        print_directory_total(entries, count, options);

    for (index = 0; index < count; index++)
        print_entry(&entries[index], options, quote_names);

    if (options->recursive) {
        for (index = 0; index < count; index++) {
            if (!entries[index].has_info ||
                !S_ISDIR(entries[index].info.st_mode) ||
                strcmp(entries[index].name, ".") == 0 ||
                strcmp(entries[index].name, "..") == 0)
                continue;

            if (list_directory(entries[index].path, options, 1, 1) != 0)
                status = 1;
        }
    }

    free_entries(entries, count);
    return status;
}

static void compare_and_sort_operands(Entry *entries, size_t count,
                                      const Options *options)
{
    sort_entries(entries, count, options);
}

int list_operands(int operand_count, char *operands[], const Options *options)
{
    Entry *files;
    Entry *directories;
    size_t file_count = 0;
    size_t directory_count = 0;
    size_t index;
    int status = 0;
    int quote_names = should_quote_names(options);
    char *default_operand[] = {"."};

    if (operand_count == 0) {
        operands = default_operand;
        operand_count = 1;
    }

    files = calloc((size_t)operand_count, sizeof(*files));
    directories = calloc((size_t)operand_count, sizeof(*directories));
    if (files == NULL || directories == NULL) {
        fprintf(stderr, "ls: out of memory\n");
        free(files);
        free(directories);
        return 1;
    }

    for (index = 0; index < (size_t)operand_count; index++) {
        Entry item;
        int result;

        memset(&item, 0, sizeof(item));
        item.name = strdup(operands[index]);
        item.path = strdup(operands[index]);
        if (item.name == NULL || item.path == NULL) {
            free(item.name);
            free(item.path);
            fprintf(stderr, "ls: out of memory\n");
            status = 1;
            break;
        }

        if (options->directories_as_files)
            result = lstat(item.path, &item.info);
        else
            result = stat(item.path, &item.info);
        if (result != 0) {
            perror(item.path);
            free(item.name);
            free(item.path);
            status = 1;
            continue;
        }
        item.has_info = 1;

        if (!options->directories_as_files && S_ISDIR(item.info.st_mode))
            directories[directory_count++] = item;
        else
            files[file_count++] = item;
    }

    compare_and_sort_operands(files, file_count, options);
    compare_and_sort_operands(directories, directory_count, options);

    for (index = 0; index < file_count; index++)
        print_entry(&files[index], options, quote_names);

    for (index = 0; index < directory_count; index++) {
        int show_header = operand_count > 1 || options->recursive;
        int blank_before = (index > 0 || file_count > 0);
        if (list_directory(directories[index].path, options,
                           show_header, blank_before) != 0)
            status = 1;
    }

    free_entries(files, file_count);
    free_entries(directories, directory_count);
    return status;
}
