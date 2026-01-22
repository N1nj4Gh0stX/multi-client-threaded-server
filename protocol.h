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
# Filename: protocol.h                                      #
# Purpose:                                                   #
#     Defines request and response message structures and   #
#     shared constants used for text-based TCP protocol     #
#     exchange between the threaded server and client.      #
#     Ensures consistent formatting and parsing across     #
#     all Project 6 modules.                                #
#############################################################
# Citations:                                                #
# [1] Beej’s Guide to Network Programming, “Data Encoding,”  #
#     https://beej.us/guide/bgnet/                          #
# [2] Linux Man Pages, “send(2), recv(2) - socket I/O,”     #
#     https://man7.org/linux/man-pages/man2/send.2.html     #
# [3] ISO/IEC 9899:2018 (C11 Standard), Sections 6.7–6.9 on #
#     structure definitions and memory layout.              #
# [4] Arek Gebka, “Project 4 — protocol.h”                  #
#     CPSC 552 Assignment, 2025.                            #
#############################################################
*/

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"   // Use BUFFER_SIZE from a single authoritative source

/* ========================================================================== */
/* ============================ Protocol Constants ========================= */
/* ========================================================================== */

/**
 * @brief Status codes for server responses.
 */
typedef enum {
    STATUS_SUCCESS = 0,   /**< Successful operation. */
    STATUS_FAILURE = 1    /**< Failed operation. */
} StatusCode;

/**
 * @brief Maximum number of Pokémon per trainer (as per assignment spec).
 */
#define MAX_POKEMON 6

/**
 * @brief Delimiter for text-based protocol tokens.
 * Typically used to split command words from arguments.
 */
#define PROTOCOL_DELIM " "

/* ========================================================================== */
/* ============================ Message Structures ========================== */
/* ========================================================================== */

/**
 * @brief Request message sent from client to server.
 *
 * Example commands:
 *   - "get pokemon 25"
 *   - "post trainer Ash 1 4 7"
 *   - "put trainer 3 25 26 133"
 *   - "delete trainer 2"
 *   - "get log 10"
 *   - "exit"
 */
typedef struct {
    char command[BUFFER_SIZE]; /**< Raw text command sent from client. */
} Request;

/**
 * @brief Response message sent from server to client.
 *
 * The response includes a simple integer status code and a message body.
 * The message field can include formatted text, trainer lists, logs, or
 * user-friendly error messages.
 */
typedef struct {
    int  status;                      /**< 0 → success, 1 → failure. */
    char message[BUFFER_SIZE];        /**< Human-readable response text. */
} Response;

#endif /* PROTOCOL_H */
