/*
#############################################################
# Author: Arek Gebka                                        #
# Major: Computer Science                                   #
# Creation Date: October 30, 2025                           #
# Last Updated: December 8, 2025                            #
# Due Date: December 10, 2025                               #
# Course: CPSC 552 — Advanced Unix Programming              #
# Professor Name: Dr. Dylan Schwesinger                     #
# Assignment: Project 6 — Threaded Server with Logging      #
# Filename: server.c                                        #
# Purpose:                                                   #
#     Implements a multithreaded TCP server that manages    #
#     Pokémon and Trainer records using binary files.       #
#     Supports concurrent CRUD operations via pthreads,    #
#     validates all Pokémon IDs, logs every client request  #
#     with timestamp and addressing metadata, and performs  #
#     graceful shutdown using SIGINT.                       #
#############################################################
# Citations:                                                #
# [1] Beej’s Guide to Network Programming, Concurrent I/O   #
#     https://beej.us/guide/bgnet/                          #
# [2] Linux Man Pages: socket(2), bind(2), listen(2),       #
#     accept(2), pthread_create(3), pthread_mutex_lock(3), #
#     signal(2), open(2), read(2), write(2), close(2)       #
#     https://man7.org/linux/man-pages/                     #
# [3] POSIX Threads Programming Guide                       #
#     https://man7.org/linux/man-pages/man7/pthreads.7.html#
# [4] ISO/IEC 9899:2018 (C11 Standard)                      #
# [5] Dr. Dylan Schwesinger, CPSC 552 — Project 6 Notes     #
# [6] MaJerle C Code Style Guide                            #
#     https://github.com/MaJerle/c-code-style               #
#############################################################
*/

#include <stdio.h>      /* printf, fprintf */
#include <stdlib.h>     /* malloc, free, atoi */
#include <string.h>     /* strcmp, strncpy, memset, strtok */
#include <unistd.h>     /* close */
#include <signal.h>     /* signal, SIGINT, SIGPIPE */
#include <errno.h>      /* errno, EINTR */
#include <pthread.h>   /* pthread_* */
#include <fcntl.h>     /* open flags */
#include <time.h>      /* time, localtime, strftime */
#include <arpa/inet.h> /* inet_ntop */
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"
#include "protocol.h"
#include "pokemon.h"
#include "trainer.h"

/* ========================================================================== */
/* =============================== Global State ============================= */
/* ========================================================================== */

/*!< Server run flag (cleared by SIGINT handler). */
static volatile int running = 1;

/*!< Listening socket descriptor (closed to unblock accept()). */
static int listenfd = -1;

/*!< Mutex protecting trainer database file access. */
static pthread_mutex_t trainer_mutex = PTHREAD_MUTEX_INITIALIZER;

/*!< Mutex protecting log file access. */
static pthread_mutex_t log_mutex     = PTHREAD_MUTEX_INITIALIZER;

/*!< Runtime file paths passed via command line. */
static char pokemon_path[256];
static char trainer_path[256];
static char log_path[256];

/* ========================================================================== */
/* ============================== Signal Handling =========================== */
/* ========================================================================== */

/**
 * \brief           Handle SIGINT for graceful server shutdown.
 *
 * \param[in]       sig     Signal number (SIGINT).
 *
 * \note            Clears the global run flag and closes the listening socket
 *                  to safely unblock accept().
 */
static void server_sigint_handler(int sig) {
    (void)sig;
    running = 0;
    
    /* Closing the listening socket unblocks accept() */
    if (listenfd >= 0)
        close(listenfd);   /* Unblocks accept() */
    printf("\n[Server] SIGINT received. Shutting down...\n");
}

/* ========================================================================== */
/* ============================ Utility Helpers ============================= */
/* ========================================================================== */

/**
 * \brief Print command-line usage instructions.
 */
static void print_usage(void) {
    printf("Usage: server -p <port> -m <pokemon_file> "
           "-t <trainer_file> -l <logfile>\n");
}

/**
 * \brief Open a binary file with standard permissions and error reporting.
 */
static int open_binary_file(const char *path, int flags) {
    int fd = open(path, flags, 0644);
    if (fd < 0) perror("[Server] open()");
    return fd;
}

/* ========================================================================== */
/* ==================== Pokémon / Trainer Helper Functions ================== */
/* ========================================================================== */

/**
 * \brief Locate a Pokémon record by ID.
 */
static int get_pokemon_by_id(int fd, int id, Pokemon *r) {
    Pokemon p;
    /* Always seek to file beginning before scanning */
    lseek(fd, 0, SEEK_SET);
    while (safe_read(fd, &p, sizeof(Pokemon)) == sizeof(Pokemon)) {
        if (p.id == id) { *r = p; return 1; }
    }
    return 0;
}

/**
 * \brief Locate a Trainer record by ID.
 */
static int get_trainer_by_id(int fd, int id, Trainer *r) {
    Trainer t;
    lseek(fd, 0, SEEK_SET);
    while (safe_read(fd, &t, sizeof(Trainer)) == sizeof(Trainer)) {
        if (t.id == id) { *r = t; return 1; }
    }
    return 0;
}

/**
 * \brief Validate that all Pokémon IDs exist in the Pokémon database.
 */
static int validate_pokemon_ids(const char *pokemon_file, int *ids, int count) {
    int fd = open(pokemon_file, O_RDONLY);
    if (fd < 0) {
        perror("[Server] Failed to open Pokémon file");
        return 0;
    }

    Pokemon p;
    for (int i = 0; i < count; i++) {
        if (!get_pokemon_by_id(fd, ids[i], &p)) {
            close(fd);
            return 0;
        }
    }
    close(fd);
    return 1;
}

/**
 * \brief Add a new trainer after validating Pokémon IDs.
 */
static int add_trainer_with_validation(const char *pokemon_file, int trainer_fd,
                                       const char *name, int *ids, int count) {
    if (count <= 0 || count > MAX_POKEMON) return -1;
    if (!validate_pokemon_ids(pokemon_file, ids, count)) return -1;

    Trainer t, tmp;
    int max_id = 0;

    lseek(trainer_fd, 0, SEEK_SET);
    
    /* Determine next available trainer ID */
    while (safe_read(trainer_fd, &tmp, sizeof(Trainer)) == sizeof(Trainer)) {
        if (tmp.id > max_id) max_id = tmp.id;
    }

    memset(&t, 0, sizeof(t));
    t.id = max_id + 1;
    strncpy(t.name, name, sizeof(t.name) - 1);
    for (int i = 0; i < count; i++) t.pokemon_ids[i] = ids[i];
    t.count = count;

    if (safe_write(trainer_fd, &t, sizeof(t)) != sizeof(t)) return -1;
    return t.id;
}

/**
 * \brief Update an existing trainer record with validation.
 */
static int update_trainer_with_validation(const char *pokemon_file, int trainer_fd,
                                          int id, int *ids, int count) {
    if (count <= 0 || count > MAX_POKEMON) return 0;
    if (!validate_pokemon_ids(pokemon_file, ids, count)) return 0;

    Trainer t;
    off_t pos = 0;

    lseek(trainer_fd, 0, SEEK_SET);
    while (safe_read(trainer_fd, &t, sizeof(t)) == sizeof(t)) {
        if (t.id == id) {
            for (int i = 0; i < count; i++) t.pokemon_ids[i] = ids[i];
            t.count = count;
            lseek(trainer_fd, pos, SEEK_SET);
            if (safe_write(trainer_fd, &t, sizeof(t)) != sizeof(t)) return 0;
            return 1;
        }
        pos += sizeof(t);
    }
    return 0;
}

/**
 * \brief Delete a trainer record by rewriting the database file.
 */
static int delete_trainer(const char *path, int id) {
    int src = open(path, O_RDONLY);
    int tmp = open("trainers.tmp", O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (src < 0 || tmp < 0) return 0;

    Trainer t;
    int found = 0;

    while (safe_read(src, &t, sizeof(t)) == sizeof(t)) {
        if (t.id != id)
            safe_write(tmp, &t, sizeof(t));
        else
            found = 1;
    }

    close(src);
    close(tmp);
    rename("trainers.tmp", path);
    return found;
}

/* ========================================================================== */
/* ================================ Logging ================================= */
/* ========================================================================== */

/**
 * \brief Log a client request with timestamp and address.
 *
 * Thread-safe via log_mutex.
 */
static void log_request(const char *ip, int port, const char *cmd) {
    pthread_mutex_lock(&log_mutex);

    FILE *fp = fopen(log_path, "a");
    if (!fp) {
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char tbuf[64];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(fp, "[%s] Client %s:%d issued command: %s\n",
            tbuf, ip, port, cmd);

    fclose(fp);
    pthread_mutex_unlock(&log_mutex);
}

/**
 * \brief Return the last N lines from the server log.
 */
static int read_last_n_lines(char *outbuf, size_t outsz, int n) {
    pthread_mutex_lock(&log_mutex);

    FILE *fp = fopen(log_path, "r");
    if (!fp) {
        pthread_mutex_unlock(&log_mutex);
        snprintf(outbuf, outsz, "No log file or cannot open log.");
        return 0;
    }

    int lines = 0;
    char buf[1024];
    
    /* Count total log lines */
    while (fgets(buf, sizeof(buf), fp))
        lines++;

    if (lines == 0) {
        fclose(fp);
        pthread_mutex_unlock(&log_mutex);
        snprintf(outbuf, outsz, "Log file is empty.");
        return 0;
    }

    int skip = (lines > n) ? (lines - n) : 0;
    rewind(fp);
    while (skip-- > 0 && fgets(buf, sizeof(buf), fp));

    outbuf[0] = '\0';
    while (fgets(buf, sizeof(buf), fp)) {
        strncat(outbuf, buf, outsz - strlen(outbuf) - 1);
    }

    fclose(fp);
    pthread_mutex_unlock(&log_mutex);
    return 1;
}

/* ========================================================================== */
/* ============================ Client Thread =============================== */
/* ========================================================================== */

/*!< Struct passed to each client-handling thread. */
typedef struct {
    int connfd;                     /*!< Connected client socket */
    struct sockaddr_in addr;       /*!< Client address info */
} client_args_t;

/**
 * \brief Thread routine servicing a single connected client.
 */
static void *client_thread(void *arg) {
    client_args_t *c = arg;
    int connfd = c->connfd;
    struct sockaddr_in client_addr = c->addr;
    free(c);

    char ip[64];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
    int port = ntohs(client_addr.sin_port);

    printf("[Server] Client connected: %s:%d (thread %lu)\n",
           ip, port, pthread_self());

    char buffer[BUFFER_SIZE];

    while (running) {
        
        /* Read one full line from the client */
        ssize_t r = recv_line(connfd, buffer, sizeof(buffer));
        if (r <= 0)
            break;

        trim_newline(buffer);
        
        /* Log command before processing */
        log_request(ip, port, buffer);

        Request req = {0};
        Response res = {0};
        snprintf(req.command, sizeof(req.command), "%s", buffer);

        /* Tokenize command into argument array */
        char *args[20];
        int argc = 0;
        char *tok = strtok(req.command, " ");
        while (tok && argc < 20) {
            args[argc++] = tok;
            tok = strtok(NULL, " ");
        }

        if (argc == 0) {
            snprintf(res.message, sizeof(res.message), "Empty command.");
        }

        /* ========================== EXIT ========================== */
        else if (strcmp(args[0], "exit") == 0) {
            snprintf(res.message, sizeof(res.message), "Goodbye from server.");
            send_all(connfd, res.message);
            break;
        }

        /* ========================== GET LOG ======================== */
        else if (argc == 3 && strcmp(args[0], "get") == 0 &&
                 strcmp(args[1], "log") == 0) {
            int n = atoi(args[2]);
            if (n <= 0) n = 10;
            char out[4096];
            if (!read_last_n_lines(out, sizeof(out), n))
                snprintf(res.message, sizeof(res.message), "Could not read log file.");
            else
                snprintf(res.message, sizeof(res.message), "%s", out);
        }

        /* ========================== GET TRAINER ==================== */
        else if (argc >= 2 && strcmp(args[0], "get") == 0 &&
                 strcmp(args[1], "trainer") == 0) {
            pthread_mutex_lock(&trainer_mutex);
            int fd = open_binary_file(trainer_path, O_RDONLY);
            if (fd < 0) {
                snprintf(res.message, sizeof(res.message), "Cannot open trainer DB.");
                pthread_mutex_unlock(&trainer_mutex);
                send_all(connfd, res.message);
                continue;
            }

            if (argc == 3) {
                int id = atoi(args[2]);
                Trainer t;
                if (get_trainer_by_id(fd, id, &t)) {
                    int pfd = open_binary_file(pokemon_path, O_RDONLY);
                    if (pfd < 0) {
                        snprintf(res.message, sizeof(res.message), "Cannot open Pokémon DB.");
                    } else {
                        char team[512] = "";
                        for (int i = 0; i < t.count; i++) {
                            Pokemon p;
                            if (get_pokemon_by_id(pfd, t.pokemon_ids[i], &p)) {
                                char entry[128];
                                snprintf(entry, sizeof(entry),
                                         "  - [%d] %s (%s/%s)\n",
                                         p.id, p.name, p.type1,
                                         (strlen(p.type2)?p.type2:"—"));
                                strncat(team, entry, sizeof(team)-strlen(team)-1);
                            }
                        }
                        snprintf(res.message, sizeof(res.message),
                                 "Trainer #%d: %s\nPokémon count: %d\nPokémon Team:\n%s",
                                 t.id, t.name, t.count, team);
                        close(pfd);
                    }
                } else {
                    snprintf(res.message, sizeof(res.message), "Trainer %d not found.", id);
                }
            } else {
                Trainer t;
                char out[1024] = "All Trainers:\n";
                while (safe_read(fd, &t, sizeof(t)) == sizeof(t)) {
                    char line[128];
                    snprintf(line, sizeof(line),
                             "  #%d %s (%d Pokémon)\n",
                             t.id, t.name, t.count);
                    strncat(out, line, sizeof(out)-strlen(out)-1);
                }
                snprintf(res.message, sizeof(res.message), "%s", out);
            }

            close(fd);
            pthread_mutex_unlock(&trainer_mutex);
        }

        /* ========================== POST TRAINER =================== */
        else if (argc >= 4 && strcmp(args[0], "post") == 0 &&
                 strcmp(args[1], "trainer") == 0) {
            int ids[MAX_POKEMON];
            int count = argc - 3;
            if (count > MAX_POKEMON) {
                snprintf(res.message, sizeof(res.message),
                         "Invalid command: Trainer cannot have more than %d Pokémon.", MAX_POKEMON);
            } else {
                for (int i = 0; i < count; i++) ids[i] = atoi(args[i+3]);
                pthread_mutex_lock(&trainer_mutex);
                int fd = open_binary_file(trainer_path, O_RDWR);
                int new_id = add_trainer_with_validation(pokemon_path, fd, args[2], ids, count);
                close(fd);
                pthread_mutex_unlock(&trainer_mutex);

                if (new_id < 0)
                    snprintf(res.message, sizeof(res.message),
                             "Invalid command: Failed validation (check Pokémon IDs).");
                else
                    snprintf(res.message, sizeof(res.message),
                             "Trainer added successfully. ID=%d", new_id);
            }
        }

        /* ========================== PUT TRAINER ==================== */
        else if (argc >= 4 && strcmp(args[0], "put") == 0 &&
                 strcmp(args[1], "trainer") == 0) {
            int id = atoi(args[2]);
            int ids[MAX_POKEMON];
            int count = argc - 3;
            if (count > MAX_POKEMON)
                snprintf(res.message, sizeof(res.message),
                         "Invalid command: Max Pokémon = %d.", MAX_POKEMON);
            else {
                for (int i = 0; i < count; i++) ids[i] = atoi(args[i+3]);
                pthread_mutex_lock(&trainer_mutex);
                int fd = open_binary_file(trainer_path, O_RDWR);
                int ok = update_trainer_with_validation(pokemon_path, fd, id, ids, count);
                close(fd);
                pthread_mutex_unlock(&trainer_mutex);

                if (!ok)
                    snprintf(res.message, sizeof(res.message), "Trainer %d not updated.", id);
                else
                    snprintf(res.message, sizeof(res.message), "Trainer %d updated.", id);
            }
        }

        /* ========================== DELETE TRAINER ================= */
        else if (argc == 3 && strcmp(args[0], "delete") == 0 &&
                 strcmp(args[1], "trainer") == 0) {
            int id = atoi(args[2]);
            pthread_mutex_lock(&trainer_mutex);
            int ok = delete_trainer(trainer_path, id);
            pthread_mutex_unlock(&trainer_mutex);

            if (!ok)
                snprintf(res.message, sizeof(res.message), "Trainer %d not found.", id);
            else
                snprintf(res.message, sizeof(res.message), "Trainer %d deleted.", id);
        }

        /* ========================== INVALID COMMAND ================= */
        else {
            snprintf(res.message, sizeof(res.message), "Invalid command.");
        }

        /* Send response back to the client */
        send_all(connfd, res.message);
    }

    printf("[Server] Client disconnected: %s:%d\n", ip, port);
    close(connfd);
    return NULL;
}

/* ========================================================================== */
/* ================================= main ================================== */
/* ========================================================================== */

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);               /* Prevent abrupt termination when client disconnects */
    signal(SIGINT, server_sigint_handler);  /* Install graceful shutdown handler */

    char port[32] = {0};
    char logname[128] = {0};
    int got_p=0, got_m=0, got_t=0, got_l=0;

    /* Parse command-line arguments (order-independent) */
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i+1 < argc) {
            strncpy(port, argv[++i], 31);
            got_p=1;
        } else if (strcmp(argv[i], "-m") == 0 && i+1 < argc) {
            strncpy(pokemon_path, argv[++i], 255);
            got_m=1;
        } else if (strcmp(argv[i], "-t") == 0 && i+1 < argc) {
            strncpy(trainer_path, argv[++i], 255);
            got_t=1;
        } else if (strcmp(argv[i], "-l") == 0 && i+1 < argc) {
            strncpy(logname, argv[++i], 127);
            got_l=1;
        }
    }

    if (!got_p || !got_m || !got_t || !got_l) {
        print_usage();
        return 1;
    }

    /* Normalize trainer file name */
    if (strcmp(trainer_path, "trainer.bin") == 0)
        strcpy(trainer_path, "trainers.bin");

    /* Resolve log file output path */
    if (strchr(logname, '/'))
        snprintf(log_path, sizeof(log_path), "%s", logname);
    else
        snprintf(log_path, sizeof(log_path), "data/%s", logname);

    /* Validate Pokémon database file */
    int fd = open_binary_file(pokemon_path, O_RDONLY);
    if (fd < 0) return 1;
    close(fd);

    /* Ensure trainer database exists */
    fd = open_binary_file(trainer_path, O_RDWR | O_CREAT);
    if (fd < 0) return 1;
    close(fd);

    /* Create and bind the listening socket */
    listenfd = create_server_socket(port);
    if (listenfd < 0) return 1;

    printf("[Server] Listening on port %s ...\n", port);

    /* ====================== Accept Loop ====================== */
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &len);

        if (connfd < 0) {
            if (!running) break;
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        /* Allocate argument block for thread */
        client_args_t *args = malloc(sizeof(client_args_t));
        args->connfd = connfd;
        args->addr = client_addr;

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, args);
        
        /* Detached thread cleans itself up */
        pthread_detach(tid);
    }

    printf("[Server] Shutdown complete.\n");
    return 0;
}
