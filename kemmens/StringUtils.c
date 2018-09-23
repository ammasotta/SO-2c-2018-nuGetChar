#include "kemmens/StringUtils.h"


char* StringUtils_Format(char* plain, ...)
{
	va_list arguments;
	va_start(arguments, plain);

	//Al ejecutar snprintf con NULL como buffer nos devuelve la cantidad de bytes que necesitamos malloc-ear
	int strlength = (size_t)vsnprintf((void*)0, 0, plain, arguments) + 1; //se queda corto por 1 char debe ser por el string end.

	char* formatted = (char*)malloc(strlength);
	vsnprintf(formatted, strlength, plain, arguments);

	va_end(arguments);

	return formatted;
}

int StringUtils_ArrayContains(char** array, char* needle)
{
	int i = 0;
	while (array[i] != NULL) {
		if(strcmp(array[i], needle) == 0)
			return 1;

		i++;
	}

	return 0;
}

int StringUtils_ArrayIndexOf(char** array, char* needle)
{
	int i = 0;
	while (array[i] != NULL) {
		if(strcmp(array[i], needle) == 0)
			return i;

		i++;
	}

	return -1;
}

int StringUtils_ArraySize(char** array)
{
	int i = 0;
	while (array[i] != NULL) {
		i++;
	}

	return i;
}

void StringUtils_FreeArray(char** array)
{
	int i = 0;
	while (array[i] != NULL) {
		free(array[i]);
		i++;
	}
	free(array);
}


