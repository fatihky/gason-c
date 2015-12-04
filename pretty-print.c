#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#if !defined(_WIN32) && !defined(NDEBUG)
#include <execinfo.h>
#include <signal.h>
#endif

#include "gason-c.c"
#include "util.c"

void printError(const char *filename, int status, char *endptr, char *source, size_t size) {
    char *s = endptr;
    while (s != source && *s != '\n')
        --s;
    if (s != endptr && s != source)
        ++s;

    int lineno = 0;
    for (char *it = s; it != source; --it) {
        if (*it == '\n') {
            ++lineno;
        }
    }

    int column = (int)(endptr - s);

    fprintf(stderr, "%s:%d:%d: %s\n", filename, lineno + 1, column + 1, gason_str_error(status));

    while (s != source + size && *s != '\n') {
        int c = *s++;
        switch (c) {
        case '\b':
            fprintf(stderr, "\\b");
            column += 1;
            break;
        case '\f':
            fprintf(stderr, "\\f");
            column += 1;
            break;
        case '\n':
            fprintf(stderr, "\\n");
            column += 1;
            break;
        case '\r':
            fprintf(stderr, "\\r");
            column += 1;
            break;
        case '\t':
            fprintf(stderr, "%*s", SHIFT_WIDTH, "");
            column += SHIFT_WIDTH - 1;
            break;
        case '\0':
            fprintf(stderr, "\"");
            break;
        default:
            fputc(c, stderr);
        }
    }

    fprintf(stderr, "\n%*s\n", column + 1, "^");
}

void on_abort(int x) {
	void *callstack[64];
	int size = backtrace(callstack, sizeof(callstack)/sizeof(callstack[0]));
	char **strings = backtrace_symbols(callstack, size);
	for (int i = 0; i < size; ++i)
		fprintf(stderr, "%s\n", strings[i]);
	free(strings);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
#if !defined(_WIN32) && !defined(NDEBUG)
    signal(SIGABRT, on_abort);
#endif

    printf("argc: %d\n", argc);

    FILE *fp = (argc > 1 && strcmp(argv[1], "-")) ? fopen(argv[1], "rb") : stdin;
    if (!fp)
    {
        fprintf(stderr, "%s: %s: %s\n", argv[0], argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }
    char *source = NULL;
    size_t sourceSize = 0;
    size_t bufferSize = 0;
    while (!feof(fp)) {
        if (sourceSize + 1 >= bufferSize) {
            bufferSize = bufferSize < BUFSIZ ? BUFSIZ : bufferSize * 2;
            source = (char *)realloc(source, bufferSize);
        }
        sourceSize += fread(source + sourceSize, 1, bufferSize - sourceSize - 1, fp);
    }
    fclose(fp);
    source[sourceSize] = 0;

    char *endptr;
    gason_value_t *value = NULL;
    gason_allocator_t *allocator = gason_allocator_new();
    int status = gason_parse(source, &endptr, &value, &allocator);
    if (status != GASON_OK) {
        printError((argc > 1 && strcmp(argv[1], "-")) ? argv[1] : "-stdin-", status, endptr, source, sourceSize);
        exit(EXIT_FAILURE);
    }
    dump_value(value, 0);
    fprintf(stdout, "\n");

    // cleanup
    free(source);
    gason_allocator_deallocate(allocator);

    return 0;
}