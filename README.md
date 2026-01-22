# Multi-Client Threaded Server (C)

## Overview
A concurrent client–server system implemented in C using TCP sockets.  
The server supports multiple simultaneous clients via a thread-per-connection model using pthreads.

Clients issue human-readable CRUD commands over TCP to query and modify binary data files. All requests are processed concurrently with proper synchronization and logged for auditing.

## Key Features
- Multi-client TCP server using a thread-per-connection model
- Concurrency implemented with pthreads and mutex-based synchronization
- Safe binary file I/O with full read/write guarantees
- Graceful server shutdown via SIGINT with clean client disconnects
- Text-based protocol with deterministic, human-readable commands
- Thread-safe request logging with timestamped entries
- Portable socket setup using `getaddrinfo()`

## Concurrency Model
- Each client connection is handled by a detached pthread
- Trainer data file access is protected by a mutex
- Log file writes are serialized via a dedicated mutex
- Pokémon database is read-only and safely shared across threads

## Technologies Used
- C
- Linux / POSIX
- TCP/IP sockets
- pthreads
- Mutex synchronization

## Build & Run

### Build
```bash
make
```

### Start the server
```bash
./server -p <port> -m <pokemon_db> -t <trainer_db> -l <log_file>
```

### Start a client
```bash
./client -h <host> -p <port>
```

##Notes
- Designed to emphasize concurrency, synchronization, and robust error handling.
- Detailed protocol description, command set, and design rationale are available in the docs/ directory.
