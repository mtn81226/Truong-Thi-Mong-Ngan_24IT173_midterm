#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "ls.h"

/* Lists file and directory operands and returns a nonzero status on errors. */
int list_operands(int operand_count, char *operands[], const Options *options);

#endif
