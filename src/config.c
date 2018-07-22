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

static enum line_type_t line_type(char *line) {
    for (int i = 0; line[i] != '\0'; i++) {
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

#define SECTION_MATCHES(str) (!strncmp(start, str, sizeof(str) - 1))
    if (SECTION_MATCHES(SECTION_GENERAL))
        *state = PARSE_GENERAL;
    else if (SECTION_MATCHES(SECTION_PEER))
        *state = PARSE_PEER;
    else
        return -1;

    return 0;
}

static int parse_config(struct config_t *config, enum parse_state_t state, char *line) {
    char *start, *end, *value;
    int len, res;

    for (start = line; *start != '\0'; start++) {
        if (!isspace(*start)) {
            break;
        }
    }
    if (*start == '\0')
        return -1;

    end = strchr(line, '=');
    if (end == NULL)
        return -1;
    len = end - start;

    for (value = end; *value != '\0'; value++) {
        if (isalnum(*value)) {
            break;
        }
    }
    if (*value == '\0')
        return -1;

#define KEY_MATCHES(str) (!strncmp(start, str, sizeof(str) - 1))
#define VALUE_MATCHES(str) (!strncmp(value, str, sizeof(str) - 1))
    switch(state) {
        case PARSE_GENERAL:
            if (KEY_MATCHES("Mode")) {
                if (VALUE_MATCHES("server")) {
                    config->mode = CONFIG_MODE_SERVER;
                } else if (VALUE_MATCHES("client")) {
                    config->mode = CONFIG_MODE_CLIENT;
                } else {
                    fprintf(stderr, "invalid mode\n");
                    return -1;
                }
            } else if (KEY_MATCHES("Address")) {
                res = cidr_parse(value, &config->addr);
            } else if (KEY_MATCHES("Listen")){
                char *endptr, *cport = strchr(value, ':');
                uint16_t port;

                if (cport == NULL) {
                    goto listen_err;
                }
                *cport = '\0';
                cport++;

                port = strtol(value, &endptr, 10);
                if (errno != 0 || (*endptr != '\0' && !isspace(*endptr))) {
                    goto listen_err;
                }

                if ((res = uv_ip4_addr(value, port, &config->listen_addr.s4)) < 0) {
                    goto listen_err;
                }

                break;
listen_err:
                fprintf(stderr, "invalid address\n");
                return -1;
            } else if (KEY_MATCHES("Timeout")) {
                char* endptr;
                uint32_t timeout = strtol(value, &endptr, 10);
                if (errno != 0 || (*endptr != '\0' && !isspace(*endptr))) {
                    fprintf(stderr, "invalid timeout specified\n");
                    return -1;
                }
                config->timeout = timeout;
            } else {
                fprintf(stderr, "unknown key");
                return -1;
            }
            break;
        case PARSE_PEER:
            break;
        case PARSE_NONE:
            return -1;
    };
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
        enum line_type_t type = line_type(buf);

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
                if (parse_config(config, state, buf) == -1)
                    goto parse_err;
                break;
        }
    }

    if (!feof(file)) {
        perror("error reading from file");
        goto err;
    }

    fclose(file);
    return 0;

parse_err:
    fprintf(stderr, "error occured while parsing line %s\n", buf);

err:
    fclose(file);
    return -1;
}

