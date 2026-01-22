/*
#############################################################
# Author: Arek Gebka                                        #
# Major: Computer Science                                   #
# Creation Date: October 30, 2025                           #
# Last Updated: December 8, 2025                            #
# Due Date: December 10, 2025                               #
# Course: CPSC 552 — Advanced Unix Programming              #
# Professor Name: Dr. Dylan Schwesinger                     #
# Assignment: Project 6 — Threaded Server                   #
# Filename: common.c                                        #
# Purpose:                                                   #
#     Implements core socket setup, safe I/O utilities,     #
#     signal handling, and TCP helper functions shared      #
#     between the threaded client and server. Provides      #
#     robust send/recv wrappers, TCP setup via              #
#     getaddrinfo(), and graceful shutdown support.         #
#############################################################
# Citations:                                                #
# [1] Beej’s Guide to Network Programming, Using Sockets    #
#     https://beej.us/guide/bgnet/                           #
# [2] Linux Man Pages: socket(2), bind(2), listen(2),       #
#     accept(2), connect(2), send(2), recv(2),              #
#     signal(2), write(2), read(2), getaddrinfo(3)          #
#     https://man7.org/linux/man-pages/                     #
# [3] ISO/IEC 9899:2018 (C11 Standard)                      #
# [4] Dr. Dylan Schwesinger, CPSC 552 — Project 6 Notes     #
# [5] MaJerle C Code Style Guide                            #
#     https://github.com/MaJerle/c-code-style               #
#############################################################
*/

#include <stdio.h>      /* printf, fprintf */
#include <stdlib.h>     /* exit */
#include <string.h>     /* memset, strlen, strcspn */
#include <unistd.h>     /* write, read, close */
#include <errno.h>      /* errno, EINTR, EPIPE, ECONNRESET */
#include <arpa/inet.h>  /* sockaddr_in */
#include <sys/socket.h> /* socket, bind, listen, accept, connect */
#include <sys/types.h>  /* ssize_t */
#include <netinet/in.h> /* IPPROTO_TCP */
#include <signal.h>     /* sig_atomic_t, signal */
#include <netdb.h>      /* getaddrinfo, freeaddrinfo */

#include "common.h"

/* ========================================================================== */
/* ============================== Global Flag =============================== */
/* ========================================================================== */

/**
 * \brief Global run flag toggled by SIGINT.
 *
 * Used by both client and server to terminate gracefully when
 * Ctrl+C is received. Declared volatile sig_atomic_t to ensure
 * safe access between signal handlers and main execution.
 */
volatile sig_atomic_t keep_running = 1;

/* ========================================================================== */
/* ============================ Signal Handler ============================== */
/* ========================================================================== */

/**
 * \brief           SIGINT handler for clean shutdown.
 *
 * \param[in]       sig         Signal number (unused).
 *
 * \note            This function only sets a flag and prints a message.
 *                  Heavy cleanup is performed by the main execution flow.
 */
void handle_sigint(int sig) {
    (void)sig;                  /* Explicitly ignore unused parameter */
    keep_running = 0;           /* Flip the global run flag so loops terminate cleanly */
    /* Async-signal-safe output */
    const char *msg = "\n[System] Caught SIGINT. Shutting down...\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

/* ========================================================================== */
/* =========================== Server Socket (GAI) ========================== */
/* ========================================================================== */

/**
 * \brief           Create and bind a TCP listening socket using getaddrinfo().
 *
 * \param[in]       port        Null-terminated port string.
 *
 * \return          Listening socket descriptor on success, -1 on failure.
 *
 * \note            Uses AI_PASSIVE for wildcard binding and enables
 *                  SO_REUSEADDR for fast restart during development.
 */
int create_server_socket(const char *port) {
    struct addrinfo hints, *res, *p;
    int sockfd = -1;
    int opt = 1;
    int rv;

    /* Zero out hints structure */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;        /* IPv4 */
    hints.ai_socktype = SOCK_STREAM;    /* TCP */
    hints.ai_flags    = AI_PASSIVE;     /* Bind to local host */

    /* Resolve local address for the given port */
    if ((rv = getaddrinfo(NULL, port, &hints, &res)) != 0) {
        fprintf(stderr, "[Server] getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    /* Iterate through all returned addresses and attempt to bind */
    for (p = res; p != NULL; p = p->ai_next) {
        
        /* Create socket for this candidate address */
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0)
            continue;

        /* Allow quick reuse of the same port after shutdown */
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            close(sockfd);
            sockfd = -1;
            continue;
        }

        /* Attempt to bind */
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0)
            break;  /* Successful bind */

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(res);

    if (sockfd < 0)
        return -1;

    /* Begin listening for incoming connections */
    if (listen(sockfd, BACKLOG) < 0) {
        close(sockfd);
        return -1;
    }

    printf("[Server] Listening on port %s...\n", port);
    return sockfd;
}

/* ========================================================================== */
/* ============================ Client Socket =============================== */
/* ========================================================================== */

/**
 * \brief           Connect to a TCP server using hostname and port.
 *
 * \param[in]       host        Hostname or dotted IPv4 string.
 * \param[in]       port        Port number string.
 *
 * \return          Connected socket descriptor on success, -1 on failure.
 */
int connect_to_server(const char *host, const char *port) {
    struct addrinfo hints, *res, *p;
    int sockfd = -1;
    int rv;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    /* Resolve server address */
    if ((rv = getaddrinfo(host, port, &hints, &res)) != 0) {
        fprintf(stderr, "[Client] getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    /* Attempt to connect to one of the resolved addresses */
    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0)
            continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)
            break;  /* Successful connection */

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(res);
    return sockfd;
}

/* ========================================================================== */
/* =============================== send_all ================================= */
/* ========================================================================== */

/**
 * \brief           Reliably send an entire buffer over a socket.
 *
 * \param[in]       sockfd      Connected socket descriptor.
 * \param[in]       buffer      Null-terminated data to send.
 *
 * \return          Number of bytes sent on success, -1 on failure.
 *
 * \note            Handles short writes caused by TCP buffering.
 */
ssize_t send_all(int sockfd, const char *buffer) {
    size_t len = strlen(buffer);
    size_t total_sent = 0;

    while (total_sent < len) {
        /* Attempt to send remaining bytes */
        ssize_t n = send(sockfd, buffer + total_sent, len - total_sent, 0);
        if (n <= 0) {
            /* Connection lost or fatal error */
            if (errno == EPIPE || errno == ECONNRESET)
                return -1;
            return -1;
        }
        total_sent += (size_t)n;
    }
    return (ssize_t)total_sent;
}

/* ========================================================================== */
/* =============================== recv_line ================================ */
/* ========================================================================== */

/**
 * \brief           Receive a single newline-terminated message.
 *
 * \param[in]       sockfd      Active socket descriptor.
 * \param[out]      buffer      Output buffer for received line.
 * \param[in]       maxlen      Maximum buffer size.
 *
 * \return          Number of bytes read, 0 on clean close, -1 on error.
 *
 * \note            Reads one byte at a time to preserve message boundaries.
 */
ssize_t recv_line(int sockfd, char *buffer, size_t maxlen)
{
    size_t pos = 0;

    while (pos < maxlen - 1) {
        char c;

        /* Receive exactly one byte */
        ssize_t n = recv(sockfd, &c, 1, 0);

        if (n == 0) {
            /* Peer closed connection */
            buffer[pos] = '\0';
            return 0;
        }

        if (n < 0) {
            if (errno == EINTR)
                continue;  /* Retry interrupted system call */
            return -1;
        }

        buffer[pos++] = c;

        /* Stop at newline for protocol framing */
        if (c == '\n')
            break;

        /* Prevent buffer overflow */
        if (pos == maxlen - 1) {
            buffer[pos] = '\0';
            return (ssize_t)pos;
        }
    }

    buffer[pos] = '\0';
    return (ssize_t)pos;
}

/* ========================================================================== */
/* ============================ Safe I/O Helpers ============================ */
/* ========================================================================== */

/**
 * \brief           Write exactly \p size bytes to a file descriptor.
 *
 * \param[in]       fd          Target file descriptor.
 * \param[in]       buf         Data buffer to write.
 * \param[in]       size        Number of bytes to write.
 *
 * \return          Number of bytes written, -1 on failure.
 */
ssize_t safe_write(int fd, const void *buf, size_t size) {
    size_t total = 0;
    ssize_t n;
    while (total < size) {
        n = write(fd, (const char*)buf + total, size - total);
        if (n <= 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total += (size_t)n;
    }
    return (ssize_t)total;
}

/**
 * \brief           Read exactly \p size bytes from a file descriptor.
 *
 * \param[in]       fd          Source file descriptor.
 * \param[out]      buf         Output buffer.
 * \param[in]       size        Number of bytes to read.
 *
 * \return          Number of bytes read, -1 on failure.
 */
ssize_t safe_read(int fd, void *buf, size_t size) {
    size_t total = 0;
    ssize_t n;
    while (total < size) {
        n = read(fd, (char*)buf + total, size - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        } else if (n == 0) break;
        total += (size_t)n;
    }
    return (ssize_t)total;
}

/* ========================================================================== */
/* ============================== trim_newline ============================== */
/* ========================================================================== */

/**
 * \brief           Remove trailing CR/LF characters from a string.
 *
 * \param[in,out]   str         Input string to sanitize.
 */
void trim_newline(char *str) {
    if (!str || *str == '\0') return;
    
    /* Replace first newline occurrence with null terminator */
    str[strcspn(str, "\r\n")] = '\0';
}

