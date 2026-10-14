#ifndef OPTIONS_H
#define OPTIONS_H

#include "ls.h"

/* Parses the command-line flags and returns the index of the first operand. */
int parse_options(int argc, char *argv[], Options *options);

/* Prints the accepted command-line syntax. */
void print_usage(const char *program_name);

#endif
