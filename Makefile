#############################################################
# Author: Arek Gebka                                        #
# Major: Computer Science                                   #
# Creation Date: October 30, 2025                           #
# Last Updated: December 2025                               #
# Due Date: December 10, 2025                               #
# Course: CPSC 552 — Advanced Unix Programming              #
# Professor Name: Dr. Dylan Schwesinger                     #
# Assignment: Project 6 — Threaded Server with Logging      #
# Filename: Makefile                                        #
# Purpose:                                                   #
#     Builds the threaded TCP server and client applications#
#     for Project 6. Supports POSIX threads (-pthread),     #
#     modular header dependencies, and clean rebuilds.      #
#############################################################
# Citations:                                                #
# [1] Arek Gebka, CPSC 552 — Project 5 file: makefile	    #
#############################################################

# ========================================================================== #
# =============================== Toolchain ================================ #
# ==========================================================================

# C compiler
CC = gcc

# Compiler warnings, optimization level, pthread support,
# and local directory include path for project headers
CFLAGS = -Wall -Wextra -O2 -pthread -I.

# ========================================================================== #
# ============================ Project Objects ============================= #
# ==========================================================================

# Object files required to build the final executables
OBJS = server.o client.o common.o

# Shared headers used across multiple translation units
HDRS = common.h protocol.h pokemon.h trainer.h client.h

# ========================================================================== #
# ============================== Default Target ============================ #
# ==========================================================================

# Build both server and client with a single "make"
all: server client

# ========================================================================== #
# ================================ Build Rules ============================= #
# ==========================================================================

# ----------- Threaded Server Build Rule -----------
server: server.o common.o
	$(CC) $(CFLAGS) -o server server.o common.o

# ----------- TCP Client Build Rule ----------------
client: client.o common.o
	$(CC) $(CFLAGS) -o client client.o common.o

# ----------- Server Object Compilation ------------
# Rebuilds if server.c or any shared header changes
server.o: server.c $(HDRS)
	$(CC) $(CFLAGS) -c server.c

# ----------- Client Object Compilation ------------
# Rebuilds if client.c or any shared header changes
client.o: client.c $(HDRS)
	$(CC) $(CFLAGS) -c client.c

# ----------- Shared Common Module Compilation -----
# Provides socket setup, protocol I/O, and signal handling
common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c

# ========================================================================== #
# ============================== Maintenance =============================== #
# ==========================================================================

# Remove all compiled binaries and intermediate object files
clean:
	rm -f server client *.o
	rm -f *.log core

# Fully clean and rebuild the entire project
rebuild: clean all

# Mark targets that are not actual files
.PHONY: all clean rebuild
