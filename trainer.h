/*
#############################################################
# Author: Arek Gebka                                        #
# Major: Computer Science                                   #
# Creation Date: October 30, 2025                           #
# Last Updated: November 2, 2025                            #
# Due Date: November 6, 2025                                #
# Course: CPSC 552 — Advanced Unix Programming              #
# Professor Name: Dr. Dylan Schwesinger                     #
# Assignment: Client–Server with Sockets                    #
# Filename: trainer.h                                       #
# Purpose: Defines the Trainer data structure and related   #
#          constants for use by both the client and server  #
#          in performing CRUD operations on trainer records.#
#          Each trainer maintains up to MAX_POKEMON entries #
#          referencing Pokémon IDs stored in the binary DB. #
#############################################################
# Citations:                                                #
# [1] Linux Man Pages, "open(2), read(2), write(2) - File   #
#     I/O System Calls," https://man7.org/linux/man-pages/  #
# [2] ISO/IEC 9899:2018 (C11 Standard), Section 6.7.2 on    #
#     structure definitions and fixed-size arrays.          #
# [3] Beej’s Guide to Network Programming, “Data Structures #
#     and Serialization,” https://beej.us/guide/bgnet/      #
# [4] Arek Gebka, Project4 file,  trainer.h                 #
#     CPSC 552 Assignment, 2025.                            #
#############################################################

*/

#ifndef TRAINER_H
#define TRAINER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * Public constants
 * ================================================================ */

/* Trainers can own between 1 and 6 Pokémon */
#define MAX_POKEMON 6

/* ================================================================
 * Structure definition
 * ================================================================ */

/**
 * \brief Trainer record
 *
 * \note This struct is written directly to a binary file.
 *       Do not change field sizes or ordering, as that would
 *       break binary compatibility between versions.
 */
#pragma pack(push, 1)
typedef struct {
    int id;                        /* Trainer ID (auto-assigned by server) */
    char name[50];                 /* Trainer name */
    int pokemon_ids[MAX_POKEMON];  /* Pokémon numbers (max 6) */
    int count;                     /* Number of Pokémon owned */
} Trainer;
#pragma pack(pop)

#endif /* TRAINER_H */


