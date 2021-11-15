/**
 * nonstop_networking
 * CS 241 - Fall 2021
 */
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include "common.h"

char **parse_args(int argc, char **argv);
verb check_args(char **args);

int connect_to_server(const char *host, const char *port) {
    struct addrinfo *addr;
    if (getaddrinfo(host, port, NULL, &addr) != 0) {
        perror("getaddrinfo");
        exit(1);
    }

    int fd = socket(addr->ai_family, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(1);
    }

    if (connect(fd, addr->ai_addr, addr->ai_addrlen) == -1) {
        perror("connect");
        exit(1);
    }

    freeaddrinfo(addr);
    return fd;
}

int send_request_header(int fd, verb command, const char *filename) {
    char buff[MAX_VERB_LEN + 1 + MAX_FILENAME_LEN + 2];
    switch (command)
    {
    case GET:
        sprintf(buff, "GET %s\n", filename);
        break;
    case PUT:
        sprintf(buff, "PUT %s\n", filename);
        break;
    case DELETE:
        sprintf(buff, "DELETE %s\n", filename);
        break;
    case LIST:
        sprintf(buff, "LIST\n");
        break;
    default:
        break;
    }

    int head_len = strlen(buff);
    // Send the request header
    if (write_all(fd, buff, head_len) == -1) {
        perror("write");
        return -1;
    }

    LOG("Sent %d bytes for first line of header", head_len);
    return 0;
}

int read_response_code(int fd, char **p_error) {
    char buff[7];
    if (read_all(fd, buff, 1) == -1) {
        return -1;
    }

    if (buff[0] == 'O') {
        buff[3] = '\0';
        if (read_all(fd, buff + 1, 2) == -1 || strcmp(buff, "OK\n") != 0) {
            return -1;
        }
        LOG("STATUS_ERROR");
    } else {
        buff[6] = '\0';
        if (read_all(fd, buff + 1, 5) == -1 || strcmp(buff, "ERROR\n") != 0) {
            return -1;
        }

        // Read the error message
        int err_cap = 64;
        int err_size = 0;
        char *err = malloc(err_cap);
        while (read_all(fd, err + err_size, 1) == 0) {
            ++err_size;
            err[err_size] = '\0';
            if (err[err_size - 1] == '\n') {
                break;
            }
            if (err_size + 2 > err_cap) {
                err_cap = err_cap * 2;
                err = realloc(err, err_cap);
            }
        }

        if (err[err_size - 1] != '\n') {
            free(err);
            return -1;
        }

        err[err_size - 1] = '\0';
        *p_error = err;
        LOG("STATUS_ERROR");
    }
    return 0;
}

int handle_get(int sock_fd, char **args) {
    if (send_request_header(sock_fd, GET, args[3]) == -1) {
        return -1;
    }

    char buff[256];

    LOG("processing response");

    // Read the response
    char *err = NULL;
    if (read_response_code(sock_fd, &err) == -1) {
        print_invalid_response();
        return -1;
    }
    if (err != NULL) {
        print_error_message(err);
        free(err);
        return -1;
    }


    // Read the size of the file.
    size_t file_size;
    if (read_size(sock_fd, &file_size) == -1) {
        print_invalid_response();
        return -1;
    }

    FILE *out_fp = fopen(args[4], "w");
    if (out_fp) {
        perror(args[4]);
        return -1;
    }

    int return_code = 0;

    while (file_size > 0) {
        ssize_t len = read(sock_fd, buff, sizeof(buff));
        if (len > (ssize_t) file_size) {
            print_received_too_much_data();
            return_code = -1;
            break;
        }

        if (len == -1 && errno != EINTR) {
            print_too_little_data();
            return_code = -1;
            break;
        }
        fwrite(buff, 1, len, out_fp);
        file_size -= len;
    }

    fclose(out_fp);
    return return_code;
}

int handle_put(int sock_fd, char **args) {
    return 0;
}

int handle_delete(int sock_fd, char **args) {
    return 0;
}

int handle_list(int sock_fd, char **args) {
    return 0;
}

int main(int argc, char **argv) {
    char **args = parse_args(argc, argv);
    verb command = check_args(args);

    int sock_fd = connect_to_server(args[0], args[1]);
    int return_code = 0;

    switch (command) {
    case GET:
        return_code = handle_get(sock_fd, args);
        break;
    case PUT:
        return_code = handle_put(sock_fd, args);
        break;
    case DELETE:
        return_code = handle_delete(sock_fd, args);
        break;
    case LIST:
        return_code = handle_list(sock_fd, args);
        break;
    default:
        break;
    }

    close(sock_fd);
    if (return_code == 0) {
        print_connection_closed();
    }
    free(args);
    return 0;
}

/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL) {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp) {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3) {
        args[3] = argv[3];
    }
    if (argc > 4) {
        args[4] = argv[4];
    }

    return args;
}

/**
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args) {
    if (args == NULL) {
        print_client_usage();
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "LIST") == 0) {
        return LIST;
    }

    if (strcmp(command, "GET") == 0) {
        if (args[3] != NULL && args[4] != NULL) {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0) {
        if (args[3] != NULL) {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0) {
        if (args[3] == NULL || args[4] == NULL) {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}
