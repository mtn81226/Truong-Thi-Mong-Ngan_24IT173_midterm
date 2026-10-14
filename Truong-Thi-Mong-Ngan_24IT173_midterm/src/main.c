#define _NETBSD_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <locale.h>
#include <stdio.h>

#include "directory.h"
#include "options.h"

int main(int argc, char *argv[])
{
    Options options;
    int first_operand;

    (void)setlocale(LC_CTYPE, "");
    (void)setlocale(LC_TIME, "");

    first_operand = parse_options(argc, argv, &options);
    if (first_operand < 0) {
        print_usage(argv[0]);
        return 2;
    }

    return list_operands(argc - first_operand,
                         argv + first_operand,
                         &options);
}
