/**
 * test_ringbuffer.c
 * * Tests the RingBuffer implementation (src/ringBuffer.c).
 * Uses single-threaded "Push-then-Pop" logic to verify pointer math
 * without triggering the "Buffer Full" blocking condition.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "test_framework.h"
#include "../include/common.h"
#include "../include/ringBuffer.h"

// Implementation of the extern declared in common.h
volatile bool keep_running = true;

void test_init_and_state()
{
    // Allocate on Heap to avoid Stack Overflow (RingBuffer is ~134MB)
    RingBuffer *rb = (RingBuffer *)calloc(1, sizeof(RingBuffer));
    if (!rb)
    {
        printf(RED "[FATAL] Out of memory\n" RESET);
        return;
    }

    // 1. Test Initialization
    int status = ring_buffer_init(rb);
    ASSERT_INT_EQ(0, status, "RingBuffer init should succeed");

    ASSERT_INT_EQ(0, rb->count, "Count should be 0 after init");
    ASSERT_INT_EQ(0, rb->head, "Head should be 0 after init");
    ASSERT_INT_EQ(0, rb->tail, "Tail should be 0 after init");

    // 2. Test Basic Produce/Consume
    PacketSlot in_slot = {0};
    in_slot.length = 123; // Marker value

    ring_buffer_push(rb, &in_slot);
    ASSERT_INT_EQ(1, rb->count, "Count should be 1 after single push");
    ASSERT_INT_EQ(1, rb->head, "Head should increment to 1");

    PacketSlot out_slot = {0};
    int pop_res = ring_buffer_pop(rb, &out_slot);
    ASSERT_INT_EQ(1, pop_res, "Pop should return success");
    ASSERT_INT_EQ(123, out_slot.length, "Data integrity: Length matches");
    ASSERT_INT_EQ(0, rb->count, "Count should be 0 after empty");
    ASSERT_INT_EQ(1, rb->tail, "Tail should increment to 1");

    // Cleanup
    ring_buffer_cleanup(rb);
    free(rb);
}

void test_wrapping_logic()
{
    RingBuffer *rb = (RingBuffer *)calloc(1, sizeof(RingBuffer));
    ring_buffer_init(rb);

    printf("    [INFO] Walking indices to edge of buffer (Index %d)...\n", QUEUE_SIZE - 1);

    // MOVE INDICES TO THE CLIFF EDGE
    // We Loop QUEUE_SIZE - 1 times.
    // In each iteration, we Push then immediately Pop.
    // Result: head and tail increment, but count stays 0 or 1.
    // This avoids the "Buffer Full" block.
    for (int i = 0; i < QUEUE_SIZE - 1; i++)
    {
        PacketSlot dummy = {0};
        ring_buffer_push(rb, &dummy);
        PacketSlot out;
        ring_buffer_pop(rb, &out);
    }

    // Verification before the wrap
    ASSERT_INT_EQ(QUEUE_SIZE - 1, rb->head, "Head should be at last index");
    ASSERT_INT_EQ(QUEUE_SIZE - 1, rb->tail, "Tail should be at last index");

    // TEST WRAP: PUSH
    // Pushing one more time should wrap Head to 0
    PacketSlot wrap_slot = {0};
    ring_buffer_push(rb, &wrap_slot);

    ASSERT_INT_EQ(0, rb->head, "Head should wrap around to 0");
    ASSERT_INT_EQ(1, rb->count, "Count is 1");

    // TEST WRAP: POP
    // Popping one more time should wrap Tail to 0
    PacketSlot out_slot;
    ring_buffer_pop(rb, &out_slot);

    ASSERT_INT_EQ(0, rb->tail, "Tail should wrap around to 0");
    ASSERT_INT_EQ(0, rb->count, "Count is 0");

    ring_buffer_cleanup(rb);
    free(rb);
}

int main()
{
    printf("\n=== RUNNING RING BUFFER TESTS ===\n");

    test_init_and_state();
    test_wrapping_logic();

    printf("\nTotal Tests Run: %d\n", tests_run);
    if (tests_failed == 0)
    {
        printf(GREEN "ALL TESTS PASSED\n" RESET);
        return 0;
    }
    else
    {
        printf(RED "TESTS FAILED: %d\n" RESET, tests_failed);
        return 1;
    }
}