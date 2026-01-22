/*
#############################################################
# Author: Arek Gebka                                        #
# Major: Computer Science                                   #
# Creation Date: October 30, 2025                           #
# Last Updated: December 2025                               #
# Due Date: December 10, 2025                               #
# Course: CPSC 552 — Advanced Unix Programming              #
# Professor Name: Dr. Dylan Schwesinger                     #
# Assignment: Project 6 — Threaded Server                   #
# Filename: common.h                                        #
# Purpose:                                                   #
#     Declares shared constants, global flags, and utility  #
#     function prototypes for socket setup, communication, #
#     and safe I/O used by both the threaded server and     #
#     client. Provides a consistent TCP networking         #
#     interface across Project 6.                           #
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

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>      /* printf, fprintf */
#include <stdlib.h>     /* exit, atoi */
#include <string.h>     /* strlen, memset, strcspn */
#include <unistd.h>     /* read, write, close */
#include <errno.h>      /* errno, EINTR */

#include <arpa/inet.h>  /* inet_pton, sockaddr_in */
#include <sys/socket.h> /* socket, bind, listen, accept, connect */
#include <sys/types.h>  /* ssize_t */
#include <netdb.h>      /* getaddrinfo */
#include <signal.h>     /* sig_atomic_t, SIGINT */

// Compatibility placeholders
#define safe_read read
#define safe_write write


/* ========================================================================== */
/* ============================== Configuration ============================= */
/* ========================================================================== */

/*!< Maximum protocol buffer size for send/recv operations. */
#define BUFFER_SIZE 8192

/*!< Maximum pending connection backlog for the threaded server. */
#define BACKLOG     10

/* ========================================================================== */
/* ====================== Global Signal-Controlled Flag ===================== */
/* ========================================================================== */

/**
 * \brief Global run-control flag.
 *
 * Set to 0 by the SIGINT handler to request graceful shutdown
 * of server accept loops and worker threads.
 */
extern volatile sig_atomic_t keep_running;

/* ========================================================================== */
/* ============================== Signal Handling =========================== */
/* ========================================================================== */

/**
 * \brief           SIGINT handler for graceful termination.
 *
 * \param[in]       sig         Signal number (SIGINT).
 *
 * \note            This function should only set a flag and perform
 *                  async-signal-safe operations.
 */
void handle_sigint(int sig);

/* ========================================================================== */
/* =========================== Socket Setup (GAI) =========================== */
/* ========================================================================== */

/**
 * \brief           Create and configure a TCP server socket bound to a port.
 *
 * \param[in]       port        Port number as a null-terminated string.
 *
 * \return          Listening socket descriptor on success, -1 on failure.
 *
 * \note            Uses getaddrinfo() for portability and enables
 *                  SO_REUSEADDR for rapid restarts.
 */
int create_server_socket(const char *port);

/**
 * \brief           Establish a TCP client connection.
 *
 * \param[in]       host        Hostname or IPv4 string.
 * \param[in]       port        Port number as a string.
 *
 * \return          Connected socket descriptor on success, -1 on failure.
 */
int connect_to_server(const char *host, const char *port);

/* ========================================================================== */
/* ================================ I/O Utilities =========================== */
/* ========================================================================== */

/**
 * \brief           Send an entire null-terminated buffer across a socket.
 *
 * \param[in]       sockfd      Active socket descriptor.
 * \param[in]       buffer      Null-terminated message buffer.
 *
 * \return          Number of bytes transmitted, -1 on failure.
 *
 * \note            Handles short writes caused by TCP stream buffering.
 */
ssize_t send_all(int sockfd, const char *buffer);

/**
 * \brief           Receive a newline-terminated message from a socket.
 *
 * \param[in]       sockfd      Active socket descriptor.
 * \param[out]      buffer      Destination buffer.
 * \param[in]       maxlen      Maximum buffer capacity.
 *
 * \return          Number of bytes received, 0 on orderly close, -1 on error.
 *
 * \note            Reads one byte at a time to preserve protocol framing.
 */
ssize_t recv_line(int sockfd, char *buffer, size_t maxlen);

/**
 * \brief           Safely write an exact number of bytes to a descriptor.
 *
 * \param[in]       fd          Target file descriptor.
 * \param[in]       buf         Data buffer.
 * \param[in]       size        Number of bytes to write.
 *
 * \return          Number of bytes written, -1 on failure.
 */
ssize_t safe_write(int fd, const void *buf, size_t size);

/**
 * \brief           Safely read an exact number of bytes from a descriptor.
 *
 * \param[in]       fd          Source file descriptor.
 * \param[out]      buf         Destination buffer.
 * \param[in]       size        Number of bytes to read.
 *
 * \return          Number of bytes read, -1 on failure.
 */
ssize_t safe_read(int fd, void *buf, size_t size);

/**
 * \brief           Remove trailing newline and carriage return characters.
 *
 * \param[in,out]   str         Input string modified in-place.
 */
void trim_newline(char *str);

#endif // COMMON_H
