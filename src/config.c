#include "config.h"
#include "util.h"

#include <ctype.h>
#include <stdio.h>

#define SECTION_GENERAL "General"
#define SECTION_PEER "Peer"

enum parse_state_t {
    PARSE_NONE = 0,
    PARSE_GENERAL = 1,
    PARSE_PEER = 2,
};

enum line_type_t {
    LINE_UNKNOWN = 0,
    LINE_COMMENT = 1,
    LINE_SECTION = 2,
    LINE_CONFIG = 3,
};

static enum line_type_t line_type(char *line, int len) {
    for (int i = 0; i < len; i++) {
        if (!isspace(line[i])) {
            switch(line[i]) {
                case '#':
                    return LINE_COMMENT;
                case '[':
                    return LINE_SECTION;
                default:
                    if (isalpha(line[i]))
                        return LINE_CONFIG;
                    else
                        return LINE_UNKNOWN;
            }
        }
    }

    // All spaces
    return LINE_COMMENT;
}

static int parse_section(enum parse_state_t *state, char *line) {
    char *start = strchr(line, '[');
    char *end = strchr(line, ']');
    int len;

    if (start == NULL || end == NULL) {
        return -1;
    }

    start++;
    len = end - start;

#define SECTION_MATCHES(str) (!strncmp(start, str, min(len, sizeof(str) - 1)))
    if (SECTION_MATCHES(SECTION_GENERAL))
        *state = PARSE_GENERAL;
    else if (SECTION_MATCHES(SECTION_PEER))
        *state = PARSE_PEER;
    else
        return -1;

    return 0;
}

int config_parse(struct config_t *config, char *filename) {
    enum parse_state_t state = PARSE_NONE;
    FILE* file = fopen(filename, "r");
    char buf[1024];

    if (file == NULL) {
        perror("error opening config file");
        return -1;
    }

    while (fgets(buf, 1024, file) != NULL) {
        int len = strlen(buf);
        enum line_type_t type = line_type(buf, len);

        switch (type) {
            case LINE_UNKNOWN:
                goto parse_err;
            case LINE_COMMENT:
                continue;
            case LINE_SECTION:
                if (parse_section(&state, buf) == -1)
                    goto parse_err;
                break;
            case LINE_CONFIG:
                printf("parse config line: %s\n", buf);
                break;
        }
    }

    if (!feof(file)) {
        perror("error reading from file");
        return -1;
    }

    return 0;

parse_err:
    fprintf(stderr, "error occured while parsing line %s\n", buf);
    return -1;
}
