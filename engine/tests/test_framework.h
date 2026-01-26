/**
 * test_framework.h
 * The Framework provides assert macros that evaluate a condition.
 * If the condition is false, it prints an error, increments the failure counter,
 * and immediately returns from the current test function to prevent cascading errors.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>

// Cool output colors
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

// Global variables
int tests_run = 0;
int tests_failed = 0;

// MACROS
#define ASSERT(condition, message)                      \
    do                                                  \
    {                                                   \
        tests_run++;                                    \
        if (!(condition))                               \
        {                                               \
            printf(RED "[FAIL] %s\n" RESET, message);   \
            tests_failed++;                             \
            return;                                     \
        }                                               \
        else                                            \
        {                                               \
            printf(GREEN "[PASS] %s\n" RESET, message); \
        }                                               \
    } while (0)

#define ASSERT_INT_EQ(expected, actual, message)                                             \
    do                                                                                       \
    {                                                                                        \
        tests_run++;                                                                         \
        if ((expected) != (actual))                                                          \
        {                                                                                    \
            printf(RED "[FAIL] %s: Expected %d, got %d\n" RESET, message, expected, actual); \
            tests_failed++;                                                                  \
            return;                                                                          \
        }                                                                                    \
        else                                                                                 \
        {                                                                                    \
            printf(GREEN "[PASS] %s\n" RESET, message);                                      \
        }                                                                                    \
    } while (0)

#endif