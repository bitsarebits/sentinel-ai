#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

/**
 * @brief Conditional Logging Macro.
 *
 * If the symbol DEBUG is defined (e.g., via compiler flag -DDEBUG), this macro
 * expands to a standard printf call, printing output to stdout.
 *
 * If DEBUG is NOT defined (Release mode), this macro expands to a
 * "no-op" (no operation) instruction. The compiler's optimizer will remove
 * this code entirely, ensuring zero performance cost in the final binary.
 *
 * @note The 'do { } while (0)' loop is a standard C idiom to ensure the macro
 * acts as a single statement, preventing syntax errors in unbraced if-else blocks.
 *
 * Usage example:
 * LOG("[SNIFFER] Captured packet len: %d\n", length);
 */
#ifdef DEBUG
// __VA_ARGS__ is a preprocessor feature that passes variable arguments
// (like %d, %s, values) directly to the printf function.
#define LOG(...) printf(__VA_ARGS__)
#else
// In Release mode, this becomes a safe empty instruction.
// The compiler sees: do { } while (0); which does nothing and is optimized away.
#define LOG(...) \
    do           \
    {            \
    } while (0)
#endif

/**
 * @brief Checks if there is input available on stdin (keyboard) within a timeout.
 * * This function uses the 'select' system call to monitor the standard input
 * file descriptor without blocking the thread indefinitely.
 * This allows the thread to periodically check for shutdown signals (Ctrl+C).
 *
 * @param seconds The maximum time to wait for input in seconds.
 * @return int 1 if input is available (safe to call scanf),
 * 0 if the timeout expired (no input),
 * -1 if an error occurred (e.g., signal interruption).
 */
int wait_for_input(int seconds);

#endif