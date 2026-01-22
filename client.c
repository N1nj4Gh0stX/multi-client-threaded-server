/*
#############################################################
# Author: Arek Gebka                                        #
# Major: Computer Science                                   #
# Creation Date: October 30, 2025                           #
# Last Updated: December 2025                               #
# Due Date: December 10, 2025                               #
# Course: CPSC 552 — Advanced Unix Programming              #
# Professor: Dr. Dylan Schwesinger                          #
# Assignment: Project 6 — Threaded Server                   #
# Filename: client.c                                        #
# Purpose:                                                   #
#     Implements the TCP client for Project 6. The client   #
#     connects to the threaded server, sends user commands  #
#     using a newline-delimited protocol, and prints        #
#     formatted server responses using recv_line().         #
#############################################################
# Citations:                                                #
# [1] Beej’s Guide to Network Programming, TCP Clients       #
#     https://beej.us/guide/bgnet/                           #
# [2] Linux Man Pages: socket(2), connect(2), send(2),      #
#     recv(2), signal(2)                                    #
#     https://man7.org/linux/man-pages/                     #
# [3] ISO/IEC 9899:2018 (C11 Standard)                      #
# [4] Dr. Dylan Schwesinger, CPSC 552 Lecture Notes         #
# [5] MaJerle C Code Style Guide                            #
#     https://github.com/MaJerle/c-code-style               #
#############################################################
*/

#include <stdio.h>      /* printf, fprintf, perror */
#include <stdlib.h>     /* exit */
#include <string.h>     /* strcmp, strncpy, strlen, snprintf */
#include <unistd.h>     /* close */
#include <signal.h>     /* signal, SIGPIPE */

#include "common.h"
#include "client.h"

/* ========================================================================== */
/* ============================== Usage Helper ============================== */
/* ========================================================================== */

/**
 * \brief           Print proper command-line usage for the client.
 */
static void print_usage(void) {
    printf("Usage: client -h <host> -p <port>\n");
}

/* ========================================================================== */
/* ========================= Argument Parsing (Exported) ==================== */
/* ========================================================================== */

/**
 * \brief           Parse command-line arguments for the client.
 *
 * \param[in]       argc        Argument count.
 * \param[in]       argv        Argument vector.
 * \param[out]      host        Output buffer for host string.
 * \param[out]      port        Output buffer for port string.
 *
 * \return          0 on success, 1 on error.
 */
int parse_client_arguments(int argc, char *argv[], char *host, char *port)
{
    int got_host = 0, got_port = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            strncpy(host, argv[++i], 255);
            got_host = 1;
        }
        else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            strncpy(port, argv[++i], 31);
            got_port = 1;
        }
    }

    if (!got_host || !got_port) {
        fprintf(stderr, "Error: Missing required arguments.\n");
        print_usage();
        return 1;
    }
    return 0;
}

/* ========================================================================== */
/* ===================== Send Command and Receive Reply ===================== */
/* ========================================================================== */

/**
 * \brief           Send a single command to the server and print its response.
 *
 * \param[in]       sockfd      Connected socket descriptor.
 * \param[in]       command     Command string to send.
 *
 * \return          0 on success, -1 on failure.
 *
 * \note            Uses recv_line() to read until the [END] marker.
 */
int send_command(int sockfd, const char *command)
{
    char sendbuf[BUFFER_SIZE];
    char recvbuf[BUFFER_SIZE];

    /* Append newline for proper recv_line() termination */
    snprintf(sendbuf, sizeof(sendbuf), "%s\n", command);

    if (send_all(sockfd, sendbuf) < 0) {
        perror("[Client] Failed to send command");
        return -1;
    }

    /* Receive until end-of-message marker */
    while (1) {
        ssize_t n = recv_line(sockfd, recvbuf, sizeof(recvbuf));
        if (n <= 0) {
            printf("[Client] Connection closed or error.\n");
            return -1;
        }

        /* End of message marker */
        if (strcmp(recvbuf, "[END]\n") == 0)
            break;

        printf("%s", recvbuf);
    }

    printf("\n");
    return 0;
}

/* ========================================================================== */
/* =============================== REPL Loop ================================ */
/* ========================================================================== */

/**
 * \brief           Start the client-side read–eval–print loop (REPL).
 *
 * \param[in]       sockfd      Active server connection socket.
 *
 * \note            Supports interactive and batch test input.
 */
void start_repl(int sockfd)
{
    char command[BUFFER_SIZE];

    printf("[Client] Type 'exit' to quit.\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(command, sizeof(command), stdin)) {
            printf("\n[Client] End of input.\n");
            break;
        }

        trim_newline(command);

        /* Ignore blank lines */
        if (strlen(command) == 0)
            continue;
        
        /* Ignore comment lines in batch test mode */
        if (command[0] == '#')
            continue;

        if (strcmp(command, "exit") == 0) {
            send_all(sockfd, "exit\n");
            printf("[Client] Exiting.\n");
            break;
        }

        if (send_command(sockfd, command) < 0)
            break;
    }
}

/* ========================================================================== */
/* ================================= main ================================== */
/* ========================================================================== */

/**
 * \brief           Program entry point for the Project 6 client.
 *
 * \param[in]       argc        Argument count.
 * \param[in]       argv        Argument vector.
 *
 * \return          0 on normal termination, 1 on failure.
 */
int main(int argc, char *argv[])
{
    char host[256] = {0};
    char port[32] = {0};

    /* Avoid termination on SIGPIPE when server closes early */
    signal(SIGPIPE, SIG_IGN);

    if (parse_client_arguments(argc, argv, host, port) != 0)
        return 1;

    int sockfd = connect_to_server(host, port);
    if (sockfd < 0) {
        fprintf(stderr, "[Client] Could not connect to %s:%s\n", host, port);
        return 1;
    }

    printf("[Client] Connected to %s:%s (pid=%d)\n", host, port, getpid());
    start_repl(sockfd);

    close(sockfd);
    printf("[Client] Connection closed.\n");

    return 0;
}
