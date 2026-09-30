#include <stdio.h>
#include <stdlib.h>

#include "collector.h"

static FILE *file_handle = NULL;

int collector_init(const char *filename)
{
    // Check if the file already exists
    file_handle = fopen(filename, "r");
    if (file_handle == NULL)
    { // the file doesn't exists
      // Create it and open in append mode
        file_handle = fopen(filename, "a");
        if (file_handle == NULL)
        {
            fprintf(stderr, "[COLLECTOR] Error while opening the file %s\n", filename);
            return -1;
        }

        // Write the header
        fprintf(file_handle, "timestamp,protocol,src_port,dst_port,length,flags\n");
    }
    else
    { // File already exists
        // Close it and open it again in append mode, header is already there
        fclose(file_handle);
        file_handle = fopen(filename, "a");
        if (file_handle == NULL)
        {
            fprintf(stderr, "[COLLECTOR] Error while opening the file %s\n", filename);
            return -1;
        }
    }
    return 0;
}

int collector_record(const PacketFeatures *features)
{
    if (!file_handle)
        return -1;

    // Write in the file
    int result = fprintf(file_handle, "%.6f,%d,%d,%d,%d,%d\n",
                         features->timestamp, features->protocol, features->src_port,
                         features->dest_port, features->packet_len, features->tcp_flags);
    fflush(file_handle);
    return (result > 0) ? 0 : -1;
}

void collector_close(void)
{
    // Close the file
    if (file_handle)
    {
        fclose(file_handle);
        file_handle = NULL;
    }
}