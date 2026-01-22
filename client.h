/*
#############################################################
# Author: Arek Gebka                                        #
# Major: Computer Science                                   #
# Creation Date: December 2025                              #
# Last Updated: December 2025                               #
# Due Date: December 10, 2025                               #
# Course: CPSC 552 — Advanced Unix Programming              #
# Professor: Dr. Dylan Schwesinger                          #
# Assignment: Project 6 — Threaded Server                   #
# Filename: client.h                                        #
# Purpose:                                                   #
#     Declares the public interface for the TCP client,     #
#     including argument parsing, REPL startup, and         #
#     transmission of commands to the threaded server.      #
#     This header modularizes the client logic for reuse    #
#     and maintainability across Project 6 source files.   #
#############################################################
# Citations:                                                #
# [1] Beej’s Guide to Network Programming, TCP Clients       #
#     https://beej.us/guide/bgnet/                           #
# [2] Linux Man Pages: connect(2), recv(2), send(2),        #
#     signal(2)                                             #
#     https://man7.org/linux/man-pages/                     #
# [3] ISO/IEC 9899:2018 (C11 Standard)                      #
# [4] Dr. Dylan Schwesinger, CPSC 552 — Project 6 Notes     #
# [5] MaJerle C Code Style Guide                            #
#     https://github.com/MaJerle/c-code-style               #
#############################################################
*/

#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"

/* ========================================================================== */
/* ========================== Public Client Interface ====================== */
/* ========================================================================== */

/**
 * \brief           Parse command-line arguments for the client.
 *
 * \param[in]       argc        Argument count.
 * \param[in]       argv        Argument vector.
 * \param[out]      host_out    Output buffer to store the hostname or IP.
 * \param[out]      port_out    Output buffer to store the port number.
 *
 * \return          0 on success, 1 if required arguments are missing.
 *
 * \note            Expects the format: -h <host> -p <port>
 */
int parse_client_arguments(int argc, char *argv[], char *host_out, char *port_out);

/**
 * \brief           Start the interactive Read–Eval–Print Loop (REPL).
 *
 * \param[in]       sockfd      Active socket connected to the server.
 *
 * \note            Reads user input from stdin, sends commands to the server,
 *                  and prints formatted responses until "exit" is issued.
 */
void start_repl(int sockfd);

/**
 * \brief           Send a single command to the server and print its reply.
 *
 * \param[in]       sockfd      Connected server socket.
 * \param[in]       cmd         Null-terminated command string.
 *
 * \return          0 on success, -1 on communication failure.
 *
 * \note            Uses a newline-delimited protocol and waits for
 *                  the server-side [END] message marker.
 */
int send_command(int sockfd, const char *cmd);

#endif /* CLIENT_H */
