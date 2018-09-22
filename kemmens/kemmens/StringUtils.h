#ifndef KEMMENS_STRINGUTILS_H_
#define KEMMENS_STRINGUTILS_H_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>


char* StringUtils_Format(char* plain, ...);

int StringUtils_ArrayContains(char** array, char* needle);

int StringUtils_ArraySize(char** array);

#endif /* KEMMENS_STRINGUTILS_H_ */
