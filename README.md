Author: Arek Gebka
Course: CPSC 552 — Advanced Unix Programming
Professor: Dr. Dylan Schwesinger
Assignment: Project 6 — Threaded Server
Due Date: December 10, 2025

______________________________________________________________________________________

Project Overview

This project implements a multi-threaded client–server system in C using TCP sockets 
for bidirectional communication. It is the threaded continuation of the prior 
Client–Server with Sockets project, now using pthreads for concurrency, mutexes for 
file synchronization, and logging for all client requests.

The server concurrently manages multiple clients issuing CRUD commands on two 
binary databases:

	pokemon.bin — static Pokémon data
	trainers.bin — mutable Trainer data

Each client sends text-based commands (e.g., get trainer 1) to query or modify the 
binary records, and receives human-readable responses in real time.
Every command is timestamped and appended to a shared log file for auditing.
______________________________________________________________________________________

Design Decisions

1. Thread-Per-Connection Model

Each accepted client connection spawns a pthread executing client_thread():

- Each thread handles one client session independently.
- Threads share access to Pokémon and Trainer data files.
- Synchronization is enforced via two mutexes:
	- trainer_mutex — protects concurrent reads/writes to trainers.bin
	- log_mutex — serializes writes to the log file

Threads detach immediately after creation to avoid leaks and join overhead.
  
2. Graceful Shutdown

Pressing Ctrl + C (SIGINT) in the server triggers a graceful shutdown:
- The server stops accepting new connections.
- It closes its listening socket to unblock accept().
- The server stops accepting new connections, closes the listening socket to 
unblock accept(), and signals all worker threads to terminate by flipping a shared 
atomic flag. Because all threads are detached, no join() operation is required
- The server exits without leaving open sockets or zombie processes.

Clients detect disconnections and exit gracefully, printing a user-friendly message:

“[Client] Lost connection to server (socket closed). The server may have shut down 
or the network connection was interrupted.”

3. Safe Binary I/O and Data Integrity

Custom safe_read() and safe_write() functions guarantee that full records are read 
or written, handling short I/O and EINTR interrupts correctly.
This ensures binary consistency even under heavy multi-threaded load.

Input validation rules:
- A Trainer can have 1–6 Pokémon. Any more produces an error message and the operation 
is rejected.
- Pokémon IDs are validated against the Pokémon DB. Invalid IDs prevent writes.
- Every error path returns a descriptive reason (e.g., “Invalid command: Failed validation (check Pokémon IDs)”).

4. Enhanced Concurrency and Synchronization

Unlike previous fork-based versions:
- All Trainer file access is mutex-protected, including reads.
- Pokémon file access remains read-only and safe for concurrent threads.
- The log file is appended safely with log_mutex.

This eliminates race conditions and ensures correct, reproducible results regardless of thread count.
	
5. Logging Subsystem

Each client request produces one line in the log:

[YYYY-MM-DD HH:MM:SS] Client <ip>:<port> issued command: <text>

The server implements get log <n> to return the last n entries in real time, safely using a mutex.

6. Socket Setup with getaddrinfo()

All socket operations now use getaddrinfo() instead of legacy inet_addr() calls, ensuring portability and IPv4 compliance per the feedback guidelines.
The create_server_socket() and connect_to_server() functions follow Beej’s socket patterns.

7. Error and Validation Feedback

The server never silently ignores invalid requests.
Each rejection returns clear context, for example:

	Invalid command: Trainer cannot have more than 6 Pokémon.
	Invalid command: Failed validation (check Pokémon IDs).
	Trainer 5 not found.
______________________________________________________________________________________

Protocol Description

The client and server communicate using a newline-delimited text protocol over TCP.
Each message is human-readable, space-separated, and terminated by a single newline (\n).

The design is deterministic and simple to test interactively or through batch redirection (< test.txt).

Transmission Rules
Direction			Description
Client → Server		Sends one command per line, terminated by \n (e.g., get trainer 1\n).
Server → Client		Sends a textual response message terminated by \n. Multi-line responses are supported.
Disconnection		The client sends exit\n to close the session. The server replies Goodbye from server. and terminates that thread.
Error Handling		All invalid commands return a readable explanation rather than closing the socket.
______________________________________________________________________________________

Message Format

	<command> [<subcommand>] [<arguments...>]\n

- Commands are space-delimited tokens.
- The first token defines the action (get, post, put, delete, or exit).
- Responses are human-readable and newline-terminated.
______________________________________________________________________________________

Supported Commands

get pokemon <id> — Retrieve Pokémon stats by ID.
get trainer — List all trainers.
get trainer <id> — Retrieve a specific trainer.
post trainer <name> <p1> [<p2> ...] — Add a trainer.
put trainer <id> <p1> [<p2> ...] — Update an existing trainer.
delete trainer <id> — Remove a trainer record.
exit — Gracefully disconnects from the server.

All requests are textual commands, and all responses are plain text messages 
suitable for console display.
_____________________________________________________________________________________


Logging Integration

Every request, valid or invalid, is recorded in the log file:

[2025-12-08 22:19:40] Client 127.0.0.1:51234 issued command: get trainer 1

The get log <n> command retrieves these entries chronologically.
______________________________________________________________________________________

Protocol Termination

The exit command closes a client session gracefully.
The server shuts down only on SIGINT or administrative stop.
When a socket closes, the client detects EOF and exits cleanly.
______________________________________________________________________________________

Design Rationale

This protocol was chosen for:

- Simplicity and transparency for grading.
- Compatibility with telnet or netcat testing.
- Easy debugging and human-readable logs.
- Deterministic, thread-safe interaction under concurrent clients.
______________________________________________________________________________________

Response Handling

Each response follows the format:

[status message]

The client prints it immediately.
On disconnection or server shutdown, the client automatically exits the REPL.
______________________________________________________________________________________

Binary File I/O

The Pokémon and Trainer data are stored as binary files (.bin) containing fixed-size, 
packed structures:
- Pokémon records: pre-encoded from a CSV file in a previous assignment.
- Trainer records: created and updated at runtime.

To preserve binary compatibility, structures in pokemon.h and trainer.h keep using:
#pragma pack(push, 1)
...
#pragma pack(pop)
This ensures exact byte alignment across systems and compilers.
______________________________________________________________________________________

Concurrency Model
_______________________________________
Component		Synchronization
_______________________________________
Pokémon DB		Read-only (no mutex needed)
Trainer DB		Protected by trainer_mutex
Log File		Protected by log_mutex
Client Threads	Detached; operate independently
______________________________________________________________________________________

Error Handling
- Missing or invalid command-line arguments trigger a usage message.
- Invalid Pokémon or Trainer IDs return descriptive error responses.
- The server logs invalid update attempts to stderr but continues running.
- Clients gracefully handle unexpected disconnects or network failures.
- Over-limit Pokémon count triggers an error.
______________________________________________________________________________________

Assumptions
1. The Pokémon binary file is pre-encoded and valid.
2. Pokémon IDs are sequential integers (1–721).
3. Trainer records are stored in a binary file created or updated at runtime 
(-t argument).
4. All strings in binary structs are fixed-length and null-terminated.
5. The system runs on a Unix-like OS supporting fork(), signals, and sockets. 
Preferably LINUX.
6. Command-line arguments (-p, -m, -t, -l) may appear in any order.
The program validates that all required arguments are present; otherwise, it prints 
a usage message and exits.
7. If the server terminates (via SIGINT or unexpected shutdown), connected clients 
detect the closed socket and exit cleanly upon their next command attemp
______________________________________________________________________________________

Example Usage
Start the Server
./server -p 8080 -m data/pokemon.bin -t data/trainers.bin -l server.log

Run Client Interactively
./client -h 127.0.0.1 -p 8080

Run Client in Batch Mode
./client -h 127.0.0.1 -p 8080 < good.txt
./client -h 127.0.0.1 -p 8080 < bad.txt

Note: Lines beginning with # in test files are now treated properly as comments.
______________________________________________________________________________________

Graceful Shutdown

Press Ctrl + C in the server terminal:

[Server] Caught SIGINT. Shutting down gracefully...
[Server] Shutdown complete.


All connected clients will automatically detect the disconnect and exit cleanly.

______________________________________________________________________________________

Compliance with Instructor Feedback

Now Uses getaddrinfo() for socket setup
Safe I/O (read/write) wrappers for full record completion
Proper thread synchronization for trainer and log files
No SO_REUSEADDR dependency in production notes
Consistent results across multiple threads
Non-normalized data (maintains original binary format)
Clean error reporting and protocol-compliant responses
______________________________________________________________________________________

Special Notes
#pragma pack(push, 1) and #pragma pack(pop)

Although the instructor discouraged #pragma pack, it remains necessary to preserve 
the binary structure layout used in previous projects.
Without explicit packing, Pokémon data alignment mismatched the encoded file 
structure, leading to corruption.

#Use of ChatGPT
I used ChatGPT for:
- Refining error handling and signal safety.
- Reviewing thread synchronization and mutex coverage.
- Generating standardized documentation headers following the MaJerle C-Code style.
- Ensuring Project 6 compliance with feedback from Projects 4 and 5.