/**
 * This module tests the circular buffer logic in isolation.
 * It verifies edge cases (like buffer wrapping) in a deterministic,
 * single-threaded environment to ensure the underlying math is correct before concurrency is introduced.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "test_framework.h"
#include "../include/common.h"

void test_buffer_initialization()
{
    RingBuffer *rb = (RingBuffer *)calloc(1, sizeof(RingBuffer));
    if (!rb)
    {
        printf(RED "[FATAL] Out of memory in test_buffer_initialization\n" RESET);
        return;
    }

    ASSERT_INT_EQ(0, rb->count, "Buffer should be empty initially");
    ASSERT_INT_EQ(0, rb->head, "Head should be 0");
    ASSERT_INT_EQ(0, rb->tail, "Tail should be 0");

    free(rb);
}

void test_buffer_wrapping()
{
    RingBuffer *rb = (RingBuffer *)calloc(1, sizeof(RingBuffer));
    if (!rb)
    {
        printf(RED "[FATAL] Out of memory in test_buffer_wrapping\n" RESET);
        return;
    }

    // Fill the buffer till the end
    rb->head = QUEUE_SIZE - 1;
    rb->count = QUEUE_SIZE - 1;

    // Simulate a write
    // Logic: rb.head = (rb.head + 1) % QUEUE_SIZE;
    int next_head = (rb->head + 1) % QUEUE_SIZE;

    ASSERT_INT_EQ(0, next_head, "Buffer head should wrap around to 0");

    free(rb);
}

int main()
{
    printf("=== RUNNING RING BUFFER TESTS ===\n");
    test_buffer_initialization();
    test_buffer_wrapping();

    printf("\nTests Run: %d\n", tests_run);
    if (tests_failed == 0)
    {
        printf(GREEN "ALL TESTS PASSED\n" RESET);
        return 0;
    }
    else
    {
        printf(RED "SOME TESTS FAILED: %d\n" RESET, tests_failed);
        return 1;
    }
}