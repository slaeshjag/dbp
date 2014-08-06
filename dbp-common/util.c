#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

char* dbp_string(const char *fmt, ...) {
	assert(fmt);

	char *buffer;
	va_list args;
	size_t len;

	va_start(args, fmt);
	len = vsnprintf(NULL, 0, fmt, args) + 1;
	va_end(args);

	if (!(buffer = calloc(1, len)))
		return NULL;

	va_start(args, fmt);
	vsnprintf(buffer, len, fmt, args);
	va_end(args);
	return buffer;
}