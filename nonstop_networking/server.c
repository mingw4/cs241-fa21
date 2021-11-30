/**
 * nonstop_networking
 * CS 241 - Fall 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>

#include "format.h"
#include "common.h"
#include "includes/dictionary.h"
#include "includes/compare.h"

#define BUFFER_SIZE 1024
#define MAX_HEADER_SIZE (MAX_VERB_LEN + 1 + MAX_FILENAME_LEN + 1)
#define TEMP_DIR_TEMPLATE "XXXXXX"
#define NUM_EVENTS 16

typedef struct Connection Connection;
typedef enum {
    RECEIVING_HEADER,
    RECEIVING_SIZE,
    RECEIVING_DATA,
    SENDING_DATA,
} ConnState;

struct Connection {
    verb command;
    int sock_fd;
    int remaining_size;    // Number of remaining bytes need to receive or send.
    uint8_t buffer[BUFFER_SIZE];
    uint8_t *list_buffer;
    int buff_size;
    int buff_offset;
    int fd;                 // Input/output filedescriptor.
    ConnState state;        // Current state of the connection
    char *filename;
};

Connection *allocate_conn();
void release_conn(Connection *conn);

char *copy_str(const char *s);
int prepare_socket(uint16_t port);
int mark_non_block(int fd);
int epoll_ctl_wrapper(int epfd, int fd, int op, uint32_t events);
void handle_event(int epfd, struct epoll_event *e);
bool prepare_send(int epfd, Connection *conn);
void send_error_message(int epfd, Connection *conn, const char *msg);
void shutdown_connection(int epfd, Connection *conn);
void copy_bytes(uint8_t *dst, uint8_t *src, int size);
void get_pathname(char *buff, const char *filename);
bool open_file(int epfd, Connection *conn, const char *filename);
void read_file(int epfd, Connection *conn);
void write_file(int epfd, Connection *conn);
int find_filename(const char *filename);
void add_filename(const char *filename);
void finish_put(int epfd, Connection *conn);

void sign_int_handle(int sig);

// Global variables.
static char dir_name[16];
static int dir_name_len;
static dictionary *fd_map;     // Map file descriptor to Connection structure.
static vector *all_files;      // Vector of the filenames of all uploaded files.

int main(int argc, char **argv) {
    if (argc != 2) {
        print_server_usage();
        return 1;
    }

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = sign_int_handle;
    if (sigaction(SIGINT, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    fd_map = int_to_shallow_dictionary_create();
    all_files = string_vector_create();

    // Create the temporary directory.
    strcpy(dir_name, TEMP_DIR_TEMPLATE);
    if (mkdtemp(dir_name) == NULL) {
        perror("mkdtemp");
        return 1;
    }

    dir_name_len = strlen(dir_name);

    print_temp_directory(dir_name);

    // Create the socket.
    const uint16_t port = atoi(argv[1]);
    int sock_fd = prepare_socket(port);
    if (sock_fd == -1) {
        return 1;
    }

    // Create the epoll instance.
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
        close(sock_fd);
        return 1;
    }

    if (epoll_ctl_wrapper(epfd, sock_fd, EPOLL_CTL_ADD, EPOLLIN) == -1) {
        perror("epoll_ctl");
        return 1;
    }

    while (true) {
        struct epoll_event events[NUM_EVENTS];
        int num_events = NUM_EVENTS;
        if ((num_events = epoll_wait(epfd, events, NUM_EVENTS, -1)) <= 0) {
            if (num_events == -1 && errno == EINTR) {
                break;
            }
            continue;
        }
        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == sock_fd) {
                // This is a new connection.
                int fd = accept(sock_fd, NULL, NULL);
                if (fd == -1) {
                    continue;
                }

                if (mark_non_block(fd) == -1) {
                    perror("fcntl");
                    close(fd);
                    continue;
                }
                if (epoll_ctl_wrapper(epfd, fd, EPOLL_CTL_ADD, EPOLLIN) == -1) {
                    perror("epoll_ctl");
                    close(fd);
                    continue;
                }
                Connection *conn = allocate_conn();
                conn->sock_fd = fd;
                dictionary_set(fd_map, &fd, conn);
                LOG("(fd=%d) new connection", fd);
            } else {
                LOG("(fd=%d) old connection with events %d", events[i].data.fd, events[i].events);
                handle_event(epfd, &events[i]);
            }
        }
    }

    char pathname[MAX_FILENAME_LEN + 32];
    VECTOR_FOR_EACH(all_files, thing, {
        const char *filename = (const char *)thing;
        get_pathname(pathname, filename);
        unlink(pathname);
    });

    rmdir(dir_name);

    close(epfd);
    close(sock_fd);
    vector_destroy(all_files);
    dictionary_destroy(fd_map);

    return 0;
}

char *copy_str(const char *s) {
    char *new_s = malloc(strlen(s) + 1);
    strcpy(new_s, s);
    return new_s;
}

int prepare_socket(uint16_t port) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        return -1;
    }

    // Mark the file descriptor as non-blocking
    if (mark_non_block(sock_fd) == -1) {
        perror("fcntl");
        close(sock_fd);
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock_fd, (const struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    if (listen(sock_fd, 64) == -1) {
        perror("listen");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

Connection *allocate_conn() {
    Connection *conn = malloc(sizeof(*conn));
    conn->sock_fd = -1;
    conn->remaining_size = 0;
    conn->list_buffer = NULL;
    conn->buff_size = 0;
    conn->buff_offset = 0;
    conn->fd = -1;
    conn->state = RECEIVING_HEADER;
    conn->filename = NULL;
    return conn;
}

void release_conn(Connection *conn) {
    if (conn->sock_fd != -1)
        close(conn->sock_fd);
    if (conn->fd != -1)
        close(conn->fd);
    if (conn->filename != NULL)
        free(conn->filename);
    free(conn);
}

// Make the given filedescriptor non-blocking.
int mark_non_block(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        perror("fcntl");
        close(fd);
        return -1;
    }

    // Mark the file descriptor as non-blocking
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        close(fd);
        return -1;
    }
    return 0;
}

int epoll_ctl_wrapper(int epfd, int fd, int op, uint32_t events) {
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    return epoll_ctl(epfd, op, fd, &event);
}

void get_pathname(char *buff, const char *filename) {
    strcpy(buff, dir_name);
    buff[dir_name_len] = '/';
    strcpy(buff + dir_name_len + 1, filename);
}

bool open_file(int epfd, Connection *conn, const char *filename) {
    (void) epfd;
    char pathname[MAX_FILENAME_LEN + 32];
    get_pathname(pathname, filename);

    int fd;
    if (conn->command == PUT) {
        fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } else {
        fd = open(pathname, O_RDONLY);
    }

    if (fd == -1) {
        LOG("cannot open the file %s", filename);
        perror(pathname);
        return false;
    }

    conn->fd = fd;

    return true;
}

// PUT
// The file size has been received. Prepare reading the content of the uploaded file.
void prepare_receive_file(int epfd, Connection *conn) {
    size_t content_size = *(size_t *)(conn->buffer);
    conn->remaining_size = (int)content_size - (conn->buff_size - (int)sizeof(size_t));
    conn->state = RECEIVING_DATA;

    LOG("(fd=%d) ready to receive file, total size = %lu, received size = %d, remaining size = %d",
            conn->sock_fd,
            content_size,
            conn->buff_size - (int)sizeof(size_t),
            conn->remaining_size);

    // The header and size has just been received. Open the file for reading.
    if (!open_file(epfd, conn, conn->filename)) {
        shutdown_connection(epfd, conn);
        return;
    }

    LOG("(fd=%d) the file '%s' has been opened", conn->sock_fd, conn->filename);
    write(conn->fd, conn->buffer + sizeof(size_t), conn->buff_size - (int)sizeof(size_t));
    conn->buff_size = 0;
    conn->buff_offset = 0;

    if (conn->remaining_size <= 0) {
        finish_put(epfd, conn);
    }
}

// PUT or DELETE
void prepare_send_ok(int epfd, Connection *conn) {
    if (!prepare_send(epfd, conn))
        return;
    conn->buff_offset = 0;
    conn->buff_size = 3;
    conn->remaining_size = 3;
    conn->state = SENDING_DATA;
    strcpy((char *)conn->buffer, "OK\n");
}

// PUT
void receive_size(int epfd, Connection *conn) {
    int len = read(conn->sock_fd, conn->buffer + conn->buff_size, BUFFER_SIZE - conn->buff_size);
    if (len == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // There is an error.
            perror("read");
            shutdown_connection(epfd, conn);
        }
        return;
    }
    if (len == 0) {
        // Unexpected EOF.
        send_error_message(epfd, conn, err_bad_request);
        return;
    }
    conn->buff_size += len;
    if (conn->buff_size >= (int)sizeof(size_t)) {
        LOG("(f=%d) file size of '%s' has been received", conn->sock_fd, conn->filename);
        prepare_receive_file(epfd, conn);
    }
}

// PUT
void receive_data(int epfd, Connection *conn) {
    int len = read(conn->sock_fd, conn->buffer, BUFFER_SIZE);
    if (len == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // There is an error.
            perror("read");
            shutdown_connection(epfd, conn);
        }
        return;
    }

    if (len == 0) {
        // Unexpected EOF. The file is too small.
        if (conn->remaining_size > 0) {
            send_error_message(epfd, conn, err_bad_file_size);
            return;
        }
    } else {
        write(conn->fd, conn->buffer, len);
        conn->remaining_size -= len;
    }

    if (conn->remaining_size < 0) {
        // The file is too large.
        send_error_message(epfd, conn, err_bad_file_size);
    } else if (conn->remaining_size == 0) {
        finish_put(epfd, conn);
    }
}

void send_data(int epfd, Connection *conn) {
    uint8_t *buff = conn->buffer;
    if (conn->list_buffer != NULL)
        buff = conn->list_buffer;

    if (conn->buff_offset >= conn->buff_size && conn->command == GET && conn->remaining_size > 0) {
        // Read data from the file.
        int len = read(conn->fd, conn->buffer, BUFFER_SIZE);
        if (len <= 0) {
            // An error occurred.
            shutdown_connection(epfd, conn);
            return;
        }
        conn->buff_offset = 0;
        conn->buff_size = len;
    }

    int len = write(conn->sock_fd, buff + conn->buff_offset, conn->buff_size - conn->buff_offset);
    if (len == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("write");
            shutdown_connection(epfd, conn);
        }
        return;
    }
    if (len == 0)
        return;
    conn->buff_offset += len;
    conn->remaining_size -= len;
    if (conn->remaining_size <= 0) {
        // All data has been sent to the client. It's time to shutdown the connection.
        shutdown_connection(epfd, conn);
    }
}

// Handle data receiving/sending data on the socket when the command is "PUT".
void prepare_put(int epfd, Connection *conn, const char *filename) {
    conn->filename = copy_str(filename);

    if (conn->buff_size < (int)sizeof(size_t)) {
        conn->state = RECEIVING_SIZE;
    } else {
        // The size has been received with the header.
        prepare_receive_file(epfd, conn);
    }
}

void prepare_list(int epfd, Connection *conn) {
    if (!prepare_send(epfd, conn))
        return;

    size_t content_size = 0;
    VECTOR_FOR_EACH(all_files, thing, {
        content_size += strlen((const char *)thing);
    });

    const int num_files = vector_size(all_files);
    if (num_files > 0)
        content_size += num_files - 1;
    conn->remaining_size = 3 + sizeof(size_t) + content_size;

    uint8_t *buff = conn->buffer;
    // Allocate a buffer if necessary.
    if (conn->remaining_size > BUFFER_SIZE) {
        conn->list_buffer = malloc(conn->remaining_size);
        buff = conn->list_buffer;
    }

    // Copy the content to be sent to the buffer.
    strcpy((char *)buff, "OK\n");       // Copy "OK\n"
    *(size_t*)(buff + 3) = content_size;// Copy the content size.
    int len = 3 + sizeof(size_t);
    // Copy filename of each file.
    int i = 0;
    VECTOR_FOR_EACH(all_files, thing, {
        if (i > 0) {
            buff[len] = '\n';
            ++len;
        }
        // Copy the filename.
        const char *filename = (const char *)thing;
        strcpy((char *)(buff + len), filename);
        len += strlen(filename);
        ++i;
    });

    conn->buff_size = conn->remaining_size;
    conn->buff_offset = 0;
    conn->state = SENDING_DATA;
}

void prepare_delete(int epfd, Connection *conn, const char *filename) {
    // Find the filename.
    int idx = find_filename(filename);
    if (idx == -1) {
        send_error_message(epfd, conn, err_no_such_file);
        return;
    }

    vector_erase(all_files, idx);
    char pathname[MAX_FILENAME_LEN + 32];
    get_pathname(pathname, filename);
    unlink(pathname);

    prepare_send_ok(epfd, conn);
}

void prepare_get(int epfd, Connection *conn, const char *filename) {
    // Find the filename.
    int idx = find_filename(filename);
    if (idx == -1) {
        // The file does not exist.
        send_error_message(epfd, conn, err_no_such_file);
        return;
    }

    if (!open_file(epfd, conn, filename)) {
        // Cannot open the file.
        shutdown_connection(epfd, conn);
        return;
    }

    // Get the size of the file.
    struct stat stat;
    if (fstat(conn->fd, &stat) == -1) {
        perror("fstat");
        shutdown_connection(epfd, conn);
        return;
    }

    if (!prepare_send(epfd, conn))
        return;

    size_t content_size = stat.st_size;
    conn->buff_offset = 0;
    // The number of bytes need to read from the file
    conn->buff_size = 3 + sizeof(size_t);           // The size of the header.
    // Total number of bytes need to send to the client.
    conn->remaining_size = 3 + sizeof(size_t) + content_size;
    conn->state = SENDING_DATA;
    strcpy((char *)conn->buffer, "OK\n");
    *(size_t *) (conn->buffer + 3) = content_size;
}

// Parse the request header.
void parse_header(int epfd, Connection *conn, int header_size) {
    char header[MAX_HEADER_SIZE + 1];
    memcpy(header, conn->buffer, header_size);
    header[header_size - 1] = '\0';     // Remove the trailing '\n'

    // Remove the header from the buffer.
    copy_bytes(conn->buffer, conn->buffer + header_size, conn->buff_size - header_size);
    conn->buff_size -= header_size;

    // Required by PUT, GET, and DELETE command.
    const char *filename = NULL;

    LOG("(fd=%d) new connection with header '%s'", conn->sock_fd, header);

    if (strcmp(header, "LIST") == 0) {
        conn->command = LIST;
    } else {
        // There must be a space after the command.
        char *space_pos = strchr(header, ' ');
        if (space_pos == NULL) {
            send_error_message(epfd, conn, err_bad_request);
            return;
        }

        *space_pos = '\0';
        if (strcmp(header, "GET") == 0) {
            conn->command = GET;
        } else if (strcmp(header, "PUT") == 0) {
            conn->command = PUT;
        } else if (strcmp(header, "LIST") == 0) {
        } else if (strcmp(header, "DELETE") == 0) {
            conn->command = DELETE;
        } else {
            send_error_message(epfd, conn, err_bad_request);
            return;
        }

        filename = space_pos + 1;
        if (filename[0] == '\0') {
            // The filename is empty.
            send_error_message(epfd, conn, err_bad_request);
            return;
        }
    }

    if (conn->command == PUT) {
        prepare_put(epfd, conn, filename);
    } else {
        if (conn->buff_size > 0) {
            // For commands except PUT, there shouldn't be any data after the
            // header.
            send_error_message(epfd, conn, err_bad_request);
            return;
        }
        switch (conn->command) {
        case GET:
            prepare_get(epfd, conn, filename);
            break;
        case LIST:
            prepare_list(epfd, conn);
            break;
        case DELETE:
            prepare_delete(epfd, conn, filename);
            break;
        default:
            break;
        }
    }
}

bool prepare_send(int epfd, Connection *conn) {
    shutdown(conn->sock_fd, SHUT_RD);   // Close the reading half of the socket.
    if (epoll_ctl_wrapper(epfd, conn->sock_fd, EPOLL_CTL_MOD, EPOLLOUT) == -1) {
        perror("epoll_ctl");
        shutdown_connection(epfd, conn);
        return false;
    }
    return true;
}

void send_error_message(int epfd, Connection *conn, const char *msg) {
    if (!prepare_send(epfd, conn)) {
        return;
    }
    strcpy((char *)conn->buffer, "ERROR\n");
    strcpy((char *)conn->buffer + 6, msg);
    const int msg_len = strlen(msg);
    conn->buff_offset = 0;
    conn->buff_size = 6 + msg_len;
    conn->remaining_size = conn->buff_size;
    conn->state = SENDING_DATA;
}

void shutdown_connection(int epfd, Connection *conn) {
    (void) epfd;
    close(conn->sock_fd);
    if (dictionary_contains(fd_map, &conn->sock_fd))
        dictionary_remove(fd_map, &conn->sock_fd);
    if (conn->fd != -1)
        close(conn->fd);
    release_conn(conn);
}

void read_header(int epfd, Connection *conn) {
    int len = read(conn->sock_fd, conn->buffer + conn->buff_size, MAX_HEADER_SIZE - conn->buff_size);
    if (len == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // An error occurred.
            perror("read");
            shutdown_connection(epfd, conn);
        }
        return;
    }

    LOG("(fd=%d) received %d bytes of header, total received = %d",
            conn->sock_fd, len, conn->buff_size);

    if (len == 0) {
        // Unexpected eof.
        LOG("(fd=%d) unexpected EOF while reading header. received size = %d",
                conn->sock_fd, conn->buff_size);
        send_error_message(epfd, conn, err_bad_request);
        return;
    }

    int header_size = -1;
    for (int i = 0; i < len; ++i) {
        if (conn->buffer[conn->buff_size + i] == '\n') {
            header_size = conn->buff_size + i + 1;
            break;
        }
    }
    conn->buff_size += len;
    if (header_size != -1) {
        LOG("(fd=%d) header has been received", conn->sock_fd);
        // Parse the header.
        parse_header(epfd, conn, header_size);
    } else if (conn->buff_size >= MAX_HEADER_SIZE) {
        // The header is too long.
        LOG("(fd=%d) header too long, received size = %d", conn->sock_fd, conn->buff_size);
        send_error_message(epfd, conn, err_bad_file_size);
    }
}

void handle_event(int epfd, struct epoll_event *e) {
    int fd = e->data.fd;
    Connection *conn = dictionary_get(fd_map, &fd);
    if (conn == NULL) {
        return;
    }

    switch (conn->state) {
    case RECEIVING_HEADER:
        read_header(epfd, conn);
        break;
    case RECEIVING_SIZE:
        receive_size(epfd, conn);
        break;
    case RECEIVING_DATA:
        assert(conn->command == PUT);
        receive_data(epfd, conn);
        break;
    case SENDING_DATA:
        send_data(epfd, conn);
        break;
    default:
        break;
    }
}

void copy_bytes(uint8_t *dst, uint8_t *src, int size) {
    for (int i = 0; i < size; ++i)
        dst[i] = src[i];
}

int find_filename(const char *filename) {
    int i = 0;
    VECTOR_FOR_EACH(all_files, thing, {
        if (strcmp(filename, (const char *)thing) == 0) {
            break;
        }
        ++i;
    });

    if (i >= (int) vector_size(all_files))
        return -1;
    return i;
}

void add_filename(const char *filename) {
    int idx = find_filename(filename);
    if (idx >= 0) {
        vector_set(all_files, idx, (void *)filename);
    } else {
        vector_push_back(all_files, (void *)filename);
    }
}

void finish_put(int epfd, Connection *conn) {
    add_filename(conn->filename);
    prepare_send_ok(epfd, conn);
}

void sign_int_handle(int sig) {
    (void) sig;
};
