#include <stdio.h>
#include <pcap.h>
#include <stdlib.h>
#include <string.h>

#include "sniffer.h"
#include "utils.h"
#include "ringBuffer.h"

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
        if (scanf(" %c", &confirm) != 1)
        {
            confirm = 'n'; // Default to 'no' on read error
        }
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
    PacketSlot slot;
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

            // Prepare the slot on the stack
            slot.ts = pcapHeader->ts;
            slot.length = (pcapHeader->caplen > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : pcapHeader->caplen;
            memcpy(slot.data, packageData, slot.length);

            // Push securely
            ring_buffer_push(rb, &slot);
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