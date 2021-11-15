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
#include <sys/stat.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

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

bool has_more_byte(int fd) {
    char byte;
    return read_all(fd, &byte, 1) == 0;
}

// Send the request header to the server.
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

// Read the response code and the error message, if any.
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
        LOG("STATUS_OK");
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

// Copy the content of a file from one fd to another.
size_t copy_file(int src_fd, int dst_fd, size_t size) {
    char buff[256];
    if (size == 0) {
        if (has_more_byte(src_fd)) {
            return 1;
        }
        return 0;
    }

    size_t write_len = 0;

    while (write_len < size) {
        // Read a chunk of data from the input file.
        ssize_t len = read(src_fd, buff, sizeof(buff));
        if (len > (ssize_t) (size - write_len)) {
            print_received_too_much_data();
            write_len += len;
            break;
        }

        if (len == 0 || (len == -1 && errno != EINTR)) {
            // EOF or error.
            break;
        }

        if (len > 0) {
            // Write the chunk of data to the output file.
            if (write_all(dst_fd, buff, len) == -1) {
                break;
            }
            write_len += len;
        }
    }
    return write_len;
}

void handle_get(int sock_fd, char **args) {
    // Send the header.
    if (send_request_header(sock_fd, GET, args[3]) == -1) {
        return;
    }

    shutdown(sock_fd, SHUT_WR);

    LOG("processing response");

    // Read the response
    char *err = NULL;
    if (read_response_code(sock_fd, &err) == -1) {
        print_invalid_response();
        return;
    }
    if (err != NULL) {
        print_error_message(err);
        free(err);
        return;
    }

    // Read the size of the file.
    size_t file_size;
    if (read_size(sock_fd, &file_size) == -1) {
        print_invalid_response();
        return;
    }

    int out_fd = open(args[4], O_CREAT | O_TRUNC | O_WRONLY, 0777);
    if (out_fd == -1) {
        perror(args[4]);
        return;
    }

    size_t received_size = copy_file(sock_fd, out_fd, file_size);
    if (received_size > file_size) {
        print_received_too_much_data();
    } else if (received_size < file_size) {
        print_too_little_data();
    }
    close(out_fd);
}

void handle_put(int sock_fd, char **args) {
    if (send_request_header(sock_fd, PUT, args[3]) == -1) {
        return;
    }

    // Open the file to send
    int in_fd = open(args[4], O_RDONLY);
    if (in_fd == -1) {
        perror("open");
        return;
    }

    // Get the file size using fstat
    struct stat stat;
    if (fstat(in_fd, &stat) == -1) {
        perror("fstat");
        close(in_fd);
        return;
    }

    // Send the file size
    const size_t file_size = stat.st_size;
    if (write_all(sock_fd, &file_size, sizeof(file_size)) == -1) {
        perror("write");
        close(in_fd);
        return;
    }

    // Send the file content
    copy_file(in_fd, sock_fd, file_size);
    close(in_fd);
    shutdown(sock_fd, SHUT_WR);

    // Read the response
    char *err = NULL;
    if (read_response_code(sock_fd, &err) == -1) {
        print_invalid_response();
        return;
    }
    if (err != NULL) {
        print_error_message(err);
        free(err);
        return;
    }

    print_success();
}

void handle_delete(int sock_fd, char **args) {
    if (send_request_header(sock_fd, DELETE, args[3]) == -1) {
        return;
    }

    shutdown(sock_fd, SHUT_WR);

    // Read the response
    char *err = NULL;
    if (read_response_code(sock_fd, &err) == -1) {
        print_invalid_response();
        return;
    }
    if (err != NULL) {
        print_error_message(err);
        free(err);
        return;
    }

    print_success();
}

// Handle the LIST command
void handle_list(int sock_fd, char **args) {
    if (send_request_header(sock_fd, LIST, args[3]) == -1) {
        return;
    }

    shutdown(sock_fd, SHUT_WR);

    // Read the response
    char *err = NULL;
    if (read_response_code(sock_fd, &err) == -1) {
        print_invalid_response();
        return;
    }
    if (err != NULL) {
        print_error_message(err);
        free(err);
        return;
    }

    // Read the size of filenames.
    size_t size;
    if (read_size(sock_fd, &size) == -1) {
        print_invalid_response();
        return;
    }

    size_t received_size = copy_file(sock_fd, STDOUT_FILENO, size);
    if (received_size > size) {
        print_received_too_much_data();
    } else if (received_size < size) {
        print_too_little_data();
    }
}

int main(int argc, char **argv) {
    char **args = parse_args(argc, argv);
    verb command = check_args(args);

    int sock_fd = connect_to_server(args[0], args[1]);

    switch (command) {
    case GET:
        handle_get(sock_fd, args);
        break;
    case PUT:
        handle_put(sock_fd, args);
        break;
    case DELETE:
        handle_delete(sock_fd, args);
        break;
    case LIST:
        handle_list(sock_fd, args);
        break;
    default:
        break;
    }

    close(sock_fd);
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
