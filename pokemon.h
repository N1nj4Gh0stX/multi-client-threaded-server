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
# Filename: pokemon.h                                       #
# Purpose:                                                   #
#     Defines the Pokémon record structure used for binary  #
#     serialization and deserialization between the         #
#     threaded server and TCP client. Ensures stable memory #
#     layout across systems using explicit structure        #
#     packing.                                              #
#############################################################
# Citations:                                                #
# [1] ISO/IEC 9899:2018 (C11 Standard), Struct Layout        #
# [2] Linux Man Pages, "Data Alignment and Struct Packing"  #
#     https://man7.org/linux/man-pages/                     #
# [3] Beej’s Guide to Network Programming, Struct Packing   #
#     https://beej.us/guide/bgnet/                          #
# [4] Dr. Dylan Schwesinger, CPSC 552 — Project 6 Notes     #
# [5] MaJerle C Code Style Guide                            #
#     https://github.com/MaJerle/c-code-style               #
# [5] Arek Gebka, “Project4 file: pokemon.h                 #
#############################################################
*/

#ifndef POKEMON_H
#define POKEMON_H

#include <stdio.h>   // printf, FILE
#include <stdlib.h>  // malloc, free
#include <string.h>  // strcpy, strncpy, memset

/* ========================================================================== */
/* =============================== Data Types =============================== */
/* ========================================================================== */

/**
 * \brief           Pokémon binary database record.
 *
 * \details         Stores a single Pokémon entry exactly as written to and
 *                  read from the binary Pokémon database file. The structure
 *                  is explicitly packed to guarantee identical memory layout
 *                  across client and server builds.
 *
 * \note            Any modification to this structure will invalidate
 *                  existing Pokémon binary files.
 */
#pragma pack(push, 1)
typedef struct {
    int     id;                     // Unique Pokémon identifier
    char    name[50];               // Pokémon name
    char    type1[20];              // Primary type (e.g., "Fire")
    char    type2[20];              // Secondary type (may be empty)
    int     total;                  // Total base stat sum
    int     hp;                     // Base HP stat
    int     attack;                 // Base Attack stat
    int     defense;                // Base Defense stat
    int     sp_atk;                 // Base Special Attack stat
    int     sp_def;                 // Base Special Defense stat
    int     speed;                  // Base Speed stat
    int     generation;             // Generation number (1–8)
    int     legendary;              // 0 = false, 1 = true
    char    color[20];              // Primary color classification
    int     hasGender;              // 0 = genderless, 1 = gendered
    float   pr_male;                // Probability of being male (0.0–1.0)
    char    egg_group1[20];         // Primary egg group
    char    egg_group2[20];         // Secondary egg group
    int     hasMegaEvolution;       // 0 = false, 1 = true
    float   height_m;               // Height in meters
    float   weight_kg;              // Weight in kilograms
    int     catch_rate;             // Catch rate (0–255)
    char    body_style[30];         // Body style descriptor
} Pokemon;
#pragma pack(pop)

#endif // POKEMON_H
