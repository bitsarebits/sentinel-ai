#include <stdio.h>
#include <pcap.h>
#include <stdlib.h>
#include <string.h>
#include "sniffer.h"
#include "utils.h"

void *sniffer_thread(void *ring_buffer)
{
    printf("[SNIFFER] Thread started\n");

    // Cast dell'argomento
    RingBuffer *rb = (RingBuffer *)ring_buffer;

    // Initialize pcap to scan the net
    char errbuf[PCAP_ERRBUF_SIZE];
    if (pcap_init(PCAP_CHAR_ENC_UTF_8, errbuf) == PCAP_ERROR)
    {
        fprintf(stderr, "[SNIFFER] CRITICAL: Failed to initialize pcap library: %s\n", errbuf);
        return NULL;
    }
    LOG("[SNIFFER]: Pcap initialized successfully.\n");

    pcap_if_t *alldevsp; // Head of the list
    if (pcap_findalldevs(&alldevsp, errbuf) == PCAP_ERROR)
    {
        fprintf(stderr, "[SNIFFER] CRITICAL: Failed to scan net devices, pcap library: %s\n", errbuf);
        pcap_freealldevs(alldevsp);
        return NULL;
    }

    pcap_if_t *cursor;
    int count = 0;
    printf("[SNIFFER] Available devices:\n");
    for (cursor = alldevsp; cursor != NULL; cursor = cursor->next)
    {
        printf("%d. %s", ++count, cursor->name);
        if (cursor->description)
            printf(" (%s)\n", cursor->description);
        else
            printf(" (No description available)\n");
    }
    if (count == 0)
    {
        fprintf(stderr, "[SNIFFER] CRITICAL: No interfaces found! Make sure you are running as root.\n");
        pcap_freealldevs(alldevsp);
        return NULL;
    }

    // --- SELECTION LOOP ---
    pcap_if_t *device = NULL;
    pcap_t *handle = NULL;
    int selection = 0;
    bool found = false;
    bool print_select = true;
    do
    {
        if (print_select)
            printf("[SNIFFER] Enter interface number (1-%d): ", count);
        fflush(stdout);

        // Check for input availability (Non-blocking wait)
        int input_status = wait_for_input(1);

        if (input_status == 0)
        {
            print_select = false;
            // Timeout expired: no input yet.
            // Check if we received a shutdown signal (Ctrl+C)
            if (!keep_running)
            {
                printf("[SNIFFER] Thread exiting...\n");
                pcap_freealldevs(alldevsp);
                return NULL;
            }
            // Loop again to keep checking
            continue;
        }
        else if (input_status < 0)
        {
            // Error in select (likely interrupted by signal)
            if (!keep_running)
            {
                printf("[SNIFFER] Thread exiting...\n");
                pcap_freealldevs(alldevsp);
                return NULL;
            }
            continue;
        }
        print_select = true;

        // If we are here, input is ready! Safe to call scanf.
        if (scanf("%d", &selection) != 1) // scanf returns the number of element read
        {
            fprintf(stderr, "[SNIFFER] Invalid input! Please enter a number.\n");
            while (getchar() != '\n') // clean the buffer
                ;

            continue;
        }

        // Logic to find device in list
        if (selection < 1 || selection > count)
        {
            printf("[SNIFFER] Invalid number. Choose between 1 and %d.\n", count);
            continue;
        }

        // Retrieve pointer
        cursor = alldevsp;
        for (int i = 1; i < selection; i++)
            cursor = cursor->next;
        device = cursor;

        // Confirm
        printf("[SNIFFER] Selected %s. Confirm [y/n]: ", device->name);
        char confirm;
        scanf(" %c", &confirm);
        if (confirm != 'y' && confirm != 'Y')
        {
            continue; // Retry loop
        }

        LOG("[SNIFFER] Opening device %s\n", device->name);

        handle = pcap_open_live(device->name, 65535, 1, 1000, errbuf);
        if (handle == NULL)
        {
            fprintf(stderr, "[SNIFFER] Error opening %s: %s\n", device->name, errbuf);
            printf("[SNIFFER] Please select another device.\n");
            continue;
        }
        // Check if the link layer header is Ethernet (DLT_EN10MB)
        if (pcap_datalink(handle) != DLT_EN10MB)
        {
            fprintf(stderr, "[SNIFFER] Error: Device %s is not Ethernet (DLT %d).\n",
                    device->name, pcap_datalink(handle));
            printf("[SNIFFER] Please select a valid Ethernet device (e.g., eth0, wlan0).\n");
            pcap_close(handle);
            handle = NULL;
            continue;
        }

        found = true;

    } while (!found);

    pcap_freealldevs(alldevsp);
    while (keep_running)
    {
        struct pcap_pkthdr *pcapHeader;
        const uint8_t *packageData;
        int readPackage = pcap_next_ex(handle, &pcapHeader, &packageData);
        if (readPackage == PCAP_ERROR)
        {
            fprintf(stderr, "[SNIFFER] Failed to read a packet: %s\n", pcap_geterr(handle));
            continue;
        }
        else if (readPackage == 0)
        {
            printf("[SNIFFER] packet buffer timeout expired\n");
            continue;
        }
        else if (readPackage == 1)
        {
            LOG("[SNIFFER] Packet captured! --> Captured Length: [%d] - Real Total Length: [%d]\n", pcapHeader->caplen, pcapHeader->len);

            // Lock the mutex
            pthread_mutex_lock(&rb->mutex);

            // Wait if the buffer is full
            while (rb->count == QUEUE_SIZE && keep_running)
                pthread_cond_wait(&rb->not_full, &rb->mutex);

            // Check for shut down while waiting
            if (!keep_running)
            {

                pthread_mutex_unlock(&rb->mutex);
                break;
            }

            // We use a temporary variable to ensure we don't copy more than the slot can hold
            uint32_t copy_len = pcapHeader->caplen;
            if (copy_len > MAX_PACKET_SIZE)
            {
                copy_len = MAX_PACKET_SIZE;
            }

            // Write in the buffer
            rb->buffer[rb->head].length = (uint16_t)copy_len;
            memcpy(rb->buffer[rb->head].data, packageData, copy_len);

            // Increment the counter and the pointer
            rb->head = (rb->head + 1) % QUEUE_SIZE; // ring
            rb->count++;

            // Signal the Analyzer (consumer) and unlock the mutex
            pthread_cond_signal(&rb->not_empty);
            pthread_mutex_unlock(&rb->mutex);
        }
    }

    // SHUTDOWN SEQUENCE: Wake up the consumer just in case
    pcap_close(handle);
    pthread_mutex_lock(&rb->mutex);
    pthread_cond_broadcast(&rb->not_empty);
    pthread_mutex_unlock(&rb->mutex);
    printf("[SNIFFER] Thread exiting...\n");
    return NULL;
}