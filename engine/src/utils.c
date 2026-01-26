/*
 * utils.c
 *
 * Helper functions and utilities.
 * Contains generic tools for logging, memory inspection, and error handling.
 *
 * Key functions:
 * - Hex dump of memory buffers (for debugging packet contents).
 * - Formatted logging with timestamps.
 * - Signal handling helpers.
 */

#include <stdio.h>
#include <unistd.h>     // For STDIN_FILENO
#include <sys/select.h> // For select, fd_set, FD_ZERO, FD_SET
#include <sys/time.h>   // For struct timeval
#include "utils.h"

int wait_for_input(int seconds)
{
    fd_set fds;
    struct timeval tv;

    // 1. Initialize the file descriptor set
    FD_ZERO(&fds);

    // 2. Add Standard Input (Keyboard) to the set
    FD_SET(STDIN_FILENO, &fds);

    // 3. Set the timeout duration
    tv.tv_sec = seconds;
    tv.tv_usec = 0;

    // 4. Wait for input or timeout
    // First param is nfds (highest fd + 1). STDIN is usually 0, so 1 is fine.
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}